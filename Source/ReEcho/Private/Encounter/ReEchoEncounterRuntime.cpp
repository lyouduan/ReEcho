#include "Encounter/ReEchoEncounterRuntime.h"

namespace
{
void AddRoleEvents(const FReEchoCsvDataSnapshot& Snapshot,
                   const FReEchoCsvEncounterWaveRow& Wave,
                   const FName EnemyRole,
                   const int32 Count,
                   TArray<FReEchoScheduledSpawnEvent>& OutEvents,
                   FString& OutError,
                   const FName EnemyIdOverride = NAME_None)
{
	if (Count <= 0)
	{
		return;
	}
	const FReEchoCsvSpawnProfileRow* Profile = Snapshot.FindSpawnProfileByRole(EnemyRole);
	if (!Profile)
	{
		OutError = FString::Printf(TEXT("Encounter wave '%s' has no enabled spawn profile for role '%s'."),
		                           *Wave.Id.ToString(),
		                           *EnemyRole.ToString());
		return;
	}

	FReEchoScheduledSpawnEvent Warning;
	Warning.Type = EReEchoScheduledSpawnEventType::Warning;
	Warning.WaveId = Wave.Id;
	Warning.EnemyRole = EnemyRole;
	Warning.EnemyId = EnemyIdOverride.IsNone() ? Profile->EnemyId : EnemyIdOverride;
	Warning.Count = Count;
	const float WarningLeadSeconds = FMath::Max(0.0f, Profile->WarningLeadSeconds);
	Warning.EventSeconds = FMath::Max(0.0f, Wave.TriggerSeconds - WarningLeadSeconds);
	// A zero-second wave has no negative encounter time in which to show its warning. Keep the warning at
	// encounter start and move only the commit far enough forward to preserve the configured lead time.
	Warning.SpawnSeconds = FMath::Max(Wave.TriggerSeconds, Warning.EventSeconds + WarningLeadSeconds);
	OutEvents.Add(Warning);

	FReEchoScheduledSpawnEvent Commit = Warning;
	Commit.Type = EReEchoScheduledSpawnEventType::Commit;
	Commit.EventSeconds = Warning.SpawnSeconds;
	OutEvents.Add(Commit);
}

float Distance2D(const FVector& Left, const FVector& Right)
{
	return FVector2D(Left.X - Right.X, Left.Y - Right.Y).Size();
}
} // namespace

bool ReEchoStageTransition::Resolve(const FReEchoCsvDataSnapshot& Snapshot,
                                    const int32 CompletedEncounterIndex,
                                    FReEchoStageTransitionDecision& OutDecision,
                                    FString& OutError)
{
	OutDecision = {};
	OutError.Reset();
	if (CompletedEncounterIndex < 0)
	{
		OutError = TEXT("Completed encounter index cannot be negative.");
		return false;
	}

	const FReEchoCsvEncounterRow* NextEncounter = Snapshot.FindEncounterByIndex(CompletedEncounterIndex + 1);
	if (!NextEncounter)
	{
		OutError = FString::Printf(TEXT("No enabled encounter exists at index %d."), CompletedEncounterIndex + 1);
		return false;
	}
	const FReEchoCsvStageRow* NextStage = Snapshot.FindStage(NextEncounter->StageId);
	if (!NextStage)
	{
		OutError = FString::Printf(TEXT("Encounter '%s' references unknown StageId '%s'."),
		                           *NextEncounter->Id.ToString(),
		                           *NextEncounter->StageId.ToString());
		return false;
	}
	OutDecision.NextStageId = NextStage->Id;

	if (CompletedEncounterIndex == 0)
	{
		return true;
	}

	const FReEchoCsvEncounterRow* PreviousEncounter = Snapshot.FindEncounterByIndex(CompletedEncounterIndex);
	if (!PreviousEncounter)
	{
		OutError = FString::Printf(TEXT("No enabled completed encounter exists at index %d."), CompletedEncounterIndex);
		return false;
	}
	const FReEchoCsvStageRow* PreviousStage = Snapshot.FindStage(PreviousEncounter->StageId);
	if (!PreviousStage)
	{
		OutError = FString::Printf(TEXT("Encounter '%s' references unknown StageId '%s'."),
		                           *PreviousEncounter->Id.ToString(),
		                           *PreviousEncounter->StageId.ToString());
		return false;
	}

	OutDecision.bHasPreviousEncounter = true;
	OutDecision.PreviousStageId = PreviousStage->Id;
	OutDecision.bSameStage = PreviousStage->Id == NextStage->Id;
	OutDecision.bPreserveEnemyRoster =
	    OutDecision.bSameStage ? NextStage->bPreserveEnemiesBetweenEncounters : !NextStage->bClearEnemiesOnEnter;
	OutDecision.bPreservePlayerLocation = OutDecision.bSameStage;
	return true;
}

int32 ReEchoSpawnCapacity::CalculateReservationCount(const int32 ActiveUnitLimit,
                                                     const int32 LivingCount,
                                                     const int32 ReservedCount,
                                                     const int32 RequestedCount,
                                                     const bool bCountsTowardUnitLimit)
{
	if (!bCountsTowardUnitLimit)
	{
		return FMath::Max(0, RequestedCount);
	}
	const int32 AvailableCount =
	    FMath::Max(0, ActiveUnitLimit - FMath::Max(0, LivingCount) - FMath::Max(0, ReservedCount));
	return FMath::Clamp(RequestedCount, 0, AvailableCount);
}

bool FReEchoEncounterWaveScheduler::Configure(const FReEchoCsvDataSnapshot& Snapshot,
                                              const FName EncounterId,
                                              FString& OutError)
{
	Reset();
	OutError.Reset();
	if (!Snapshot.FindEncounter(EncounterId))
	{
		OutError = FString::Printf(TEXT("Unknown EncounterId '%s'."), *EncounterId.ToString());
		return false;
	}

	for (const FReEchoCsvEncounterWaveRow& Wave : Snapshot.GetEncounterWaves(EncounterId))
	{
		AddRoleEvents(Snapshot, Wave, TEXT("Melee"), Wave.MeleeCount, Events, OutError);
		AddRoleEvents(Snapshot, Wave, TEXT("Ranged"), Wave.RangedCount, Events, OutError);
		AddRoleEvents(Snapshot, Wave, TEXT("Elite"), Wave.EliteCount, Events, OutError);
		if (!OutError.IsEmpty())
		{
			Reset();
			return false;
		}
		if (!Wave.BossEnemyId.IsNone())
		{
			AddRoleEvents(Snapshot, Wave, TEXT("Boss"), 1, Events, OutError, Wave.BossEnemyId);
			if (!OutError.IsEmpty())
			{
				Reset();
				return false;
			}
		}
	}
	Events.Sort(
	    [](const FReEchoScheduledSpawnEvent& Left, const FReEchoScheduledSpawnEvent& Right)
	    {
		    if (!FMath::IsNearlyEqual(Left.EventSeconds, Right.EventSeconds))
		    {
			    return Left.EventSeconds < Right.EventSeconds;
		    }
		    if (Left.Type != Right.Type)
		    {
			    return Left.Type == EReEchoScheduledSpawnEventType::Warning;
		    }
		    if (Left.WaveId != Right.WaveId)
		    {
			    return Left.WaveId.LexicalLess(Right.WaveId);
		    }
		    return Left.EnemyRole.LexicalLess(Right.EnemyRole);
	    });
	return true;
}

TArray<FReEchoScheduledSpawnEvent> FReEchoEncounterWaveScheduler::AdvanceTo(const float EncounterSeconds)
{
	TArray<FReEchoScheduledSpawnEvent> Due;
	while (Events.IsValidIndex(NextEventIndex) &&
	       Events[NextEventIndex].EventSeconds <= EncounterSeconds + KINDA_SMALL_NUMBER)
	{
		Due.Add(Events[NextEventIndex++]);
	}
	return Due;
}

bool FReEchoEncounterWaveScheduler::ConfigureRepeatedOrdinaryPlan(const FReEchoCsvDataSnapshot& Snapshot,
                                                                  const FName EncounterId,
                                                                  const int32 Repetitions,
                                                                  const float StartSeconds,
                                                                  FString& OutError)
{
	FReEchoEncounterWaveScheduler Template;
	if (Repetitions < 1 || Repetitions > 20 || !FMath::IsFinite(StartSeconds) || StartSeconds < 0.0f)
	{
		OutError = TEXT("Invalid repeated spawn plan parameters.");
		return false;
	}
	if (!Template.Configure(Snapshot, EncounterId, OutError))
	{
		return false;
	}
	Template.Events.RemoveAll(
	    [](const FReEchoScheduledSpawnEvent& Event)
	    {
		    return Event.EnemyRole == TEXT("Boss");
	    });
	float Period = FMath::Max(1.0f, Snapshot.FindEncounter(EncounterId)->DurationSeconds);
	for (const FReEchoScheduledSpawnEvent& Event : Template.Events)
	{
		Period = FMath::Max(Period, Event.SpawnSeconds + 1.0f);
	}
	TArray<FReEchoScheduledSpawnEvent> Repeated;
	for (int32 Cycle = 0; Cycle < Repetitions; ++Cycle)
	{
		for (const FReEchoScheduledSpawnEvent& Original : Template.Events)
		{
			FReEchoScheduledSpawnEvent Event = Original;
			Event.WaveId = FName(*FString::Printf(TEXT("Phase3.%d.%s"), Cycle, *Original.WaveId.ToString()));
			Event.EventSeconds += StartSeconds + Cycle * Period;
			Event.SpawnSeconds += StartSeconds + Cycle * Period;
			Repeated.Add(Event);
		}
	}
	RestoreEvents(Repeated, 0);
	return true;
}

void FReEchoEncounterWaveScheduler::RestoreEvents(const TArray<FReEchoScheduledSpawnEvent>& InEvents,
                                                  const int32 InNextEventIndex)
{
	Events = InEvents;
	RestoreNextEventIndex(InNextEventIndex);
}

void FReEchoEncounterWaveScheduler::RestoreNextEventIndex(const int32 InNextEventIndex)
{
	NextEventIndex = FMath::Clamp(InNextEventIndex, 0, Events.Num());
}

void FReEchoEncounterWaveScheduler::Reset()
{
	Events.Reset();
	NextEventIndex = 0;
}

bool FReEchoSpawnResolver::Resolve(const FReEchoCsvSpawnProfileRow& Profile,
                                   const FReEchoCsvSpawnPolicyRow& Policy,
                                   const FReEchoSpawnResolveRequest& Request,
                                   FReEchoResolvedSpawn& OutSpawn,
                                   FString& OutError)
{
	OutSpawn = {};
	OutError.Reset();
	if (!Request.SpawnWorldBounds.bIsValid || Request.SpawnWorldBounds.Min.X >= Request.SpawnWorldBounds.Max.X ||
	    Request.SpawnWorldBounds.Min.Y >= Request.SpawnWorldBounds.Max.Y ||
	    Profile.MinAnchorDistanceCm > Profile.MaxAnchorDistanceCm || Policy.MaxCandidateAttempts <= 0)
	{
		OutError = TEXT("Spawn resolver received invalid arena, profile, or attempt configuration.");
		return false;
	}

	FRandomStream Random(Request.Seed + Request.Sequence * 7919);
	const bool bUseEcho = Request.bHasEchoAnchor && Random.FRand() < FMath::Clamp(Request.EchoAnchorRatio, 0.0f, 1.0f);
	const FVector Anchor = bUseEcho ? Request.EchoAnchor : Request.PlayerAnchor;
	for (int32 Attempt = 0; Attempt < Policy.MaxCandidateAttempts; ++Attempt)
	{
		const float Angle = Random.FRandRange(0.0f, 2.0f * PI);
		const float MinDistance = FMath::Max(0.0f, Profile.MinAnchorDistanceCm);
		const float MaxDistance = FMath::Max(MinDistance, Profile.MaxAnchorDistanceCm);
		const float Distance = FMath::Sqrt(Random.FRandRange(FMath::Square(MinDistance), FMath::Square(MaxDistance)));
		FVector Candidate = Anchor + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f) * Distance;
		Candidate.Z = Request.SpawnCenterWorldZ;
		if (!Request.SpawnWorldBounds.IsInside(FVector2D(Candidate.X, Candidate.Y)))
		{
			continue;
		}

		if (Distance2D(Candidate, Request.PlayerAnchor) < Policy.MinPlayerDistanceCm ||
		    (Request.bHasEchoAnchor && Distance2D(Candidate, Request.EchoAnchor) < Policy.MinEchoDistanceCm))
		{
			continue;
		}
		bool bSpacingValid = true;
		for (const FVector& Existing : Request.ExistingLocations)
		{
			if (Distance2D(Candidate, Existing) < Profile.MinSpacingCm)
			{
				bSpacingValid = false;
				break;
			}
		}
		if (!bSpacingValid)
		{
			continue;
		}
		OutSpawn.Location = Candidate;
		OutSpawn.bUsedEchoAnchor = bUseEcho;
		return true;
	}

	constexpr int32 GridSide = 11;
	const int32 GridCount = GridSide * GridSide;
	const int32 StartIndex = FMath::Abs(Request.Sequence) % GridCount;
	for (int32 Offset = 0; Offset < GridCount; ++Offset)
	{
		const int32 GridIndex = (StartIndex + Offset) % GridCount;
		const float AlphaX = (static_cast<float>(GridIndex % GridSide) + 0.5f) / GridSide;
		const float AlphaY = (static_cast<float>(GridIndex / GridSide) + 0.5f) / GridSide;
		const FVector Fallback(FMath::Lerp(Request.SpawnWorldBounds.Min.X, Request.SpawnWorldBounds.Max.X, AlphaX),
		                       FMath::Lerp(Request.SpawnWorldBounds.Min.Y, Request.SpawnWorldBounds.Max.Y, AlphaY),
		                       Request.SpawnCenterWorldZ);
		if (Distance2D(Fallback, Request.PlayerAnchor) < Policy.MinPlayerDistanceCm ||
		    (Request.bHasEchoAnchor && Distance2D(Fallback, Request.EchoAnchor) < Policy.MinEchoDistanceCm))
		{
			continue;
		}
		bool bSpacingValid = true;
		for (const FVector& Existing : Request.ExistingLocations)
		{
			if (Distance2D(Fallback, Existing) < Profile.MinSpacingCm)
			{
				bSpacingValid = false;
				break;
			}
		}
		if (!bSpacingValid)
		{
			continue;
		}
		OutSpawn.Location = Fallback;
		OutSpawn.bUsedEchoAnchor = bUseEcho;
		OutSpawn.bUsedDeterministicFallback = true;
		return true;
	}
	OutError = TEXT(
	    "No deterministic spawn candidate inside the wall-derived bounds satisfies distance and spacing constraints.");
	return false;
}
