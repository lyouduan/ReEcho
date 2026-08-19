#include "Encounter/ReEchoEncounterRuntime.h"

namespace
{
void AddRoleEvents(const FReEchoCsvDataSnapshot& Snapshot,
                   const FReEchoCsvEncounterWaveRow& Wave,
                   const FName EnemyRole,
                   const int32 Count,
                   TArray<FReEchoScheduledSpawnEvent>& OutEvents,
                   FString& OutError)
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
	Warning.EnemyId = Profile->EnemyId;
	Warning.Count = Count;
	Warning.EventSeconds = FMath::Max(0.0f, Wave.TriggerSeconds - Profile->WarningLeadSeconds);
	Warning.SpawnSeconds = Wave.TriggerSeconds;
	OutEvents.Add(Warning);

	FReEchoScheduledSpawnEvent Commit = Warning;
	Commit.Type = EReEchoScheduledSpawnEventType::Commit;
	Commit.EventSeconds = Wave.TriggerSeconds;
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
			FReEchoScheduledSpawnEvent Boss;
			Boss.Type = EReEchoScheduledSpawnEventType::Commit;
			Boss.WaveId = Wave.Id;
			Boss.EnemyRole = TEXT("Boss");
			Boss.EnemyId = Wave.BossEnemyId;
			Boss.Count = 1;
			Boss.EventSeconds = Wave.TriggerSeconds;
			Boss.SpawnSeconds = Wave.TriggerSeconds;
			Events.Add(Boss);
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
	if (Request.ArenaHalfX <= 0.0f || Request.ArenaHalfY <= 0.0f ||
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
		Candidate.X = FMath::Clamp(Candidate.X, -Request.ArenaHalfX, Request.ArenaHalfX);
		Candidate.Y = FMath::Clamp(Candidate.Y, -Request.ArenaHalfY, Request.ArenaHalfY);
		Candidate.Z = 50.0f;

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

	FVector Fallback = Anchor + FVector(Profile.MaxAnchorDistanceCm, 0.0f, 0.0f);
	Fallback.X = FMath::Clamp(Fallback.X, -Request.ArenaHalfX, Request.ArenaHalfX);
	Fallback.Y = FMath::Clamp(Fallback.Y, -Request.ArenaHalfY, Request.ArenaHalfY);
	Fallback.Z = 50.0f;
	if (Distance2D(Fallback, Request.PlayerAnchor) < Policy.MinPlayerDistanceCm)
	{
		OutError = TEXT("No deterministic spawn candidate satisfies the minimum player distance.");
		return false;
	}
	OutSpawn.Location = Fallback;
	OutSpawn.bUsedEchoAnchor = bUseEcho;
	OutSpawn.bUsedDeterministicFallback = true;
	return true;
}
