#include "Data/ReEchoCsvDataRegistry.h"
#include "Encounter/ReEchoEncounterRuntime.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoPhase3SpawnPlanTest,
                                 "ReEcho.Encounter.Phase3RepeatedPlan",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoPhase3SpawnPlanTest::RunTest(const FString& Parameters)
{
	const FReEchoCsvLoadResult Load =
	    FReEchoCsvDataRegistry::LoadSnapshotFromDirectory(FReEchoCsvDataRegistry::GetDefaultDataDirectory());
	if (!TestTrue(TEXT("Production snapshot loads"), Load.bSuccess))
	{
		return false;
	}
	FReEchoEncounterWaveScheduler Original, Repeated;
	FString Error;
	TestTrue(TEXT("Original configures"), Original.Configure(*Load.Snapshot, TEXT("Encounter.8"), Error));
	int32 OriginalCount = 0;
	TSet<FName> OriginalWaves;
	for (const FReEchoScheduledSpawnEvent& Event : Original.GetEvents())
	{
		if (Event.EnemyRole != TEXT("Boss") && Event.Type == EReEchoScheduledSpawnEventType::Commit)
		{
			OriginalCount += Event.Count;
			OriginalWaves.Add(Event.WaveId);
		}
	}
	TestTrue(TEXT("Repeated plan configures"),
	         Repeated.ConfigureRepeatedOrdinaryPlan(*Load.Snapshot, TEXT("Encounter.8"), 5, 12.0f, Error));
	int32 RepeatedCount = 0;
	TSet<FName> RepeatedWaves;
	float PreviousTime = -1.0f;
	for (const FReEchoScheduledSpawnEvent& Event : Repeated.GetEvents())
	{
		TestTrue(TEXT("Plan remains sorted and starts at entry"),
		         Event.EventSeconds >= PreviousTime && Event.EventSeconds >= 12.0f);
		TestTrue(TEXT("No extra Boss"), Event.EnemyRole != TEXT("Boss"));
		PreviousTime = Event.EventSeconds;
		if (Event.Type == EReEchoScheduledSpawnEventType::Commit)
		{
			RepeatedCount += Event.Count;
			RepeatedWaves.Add(Event.WaveId);
		}
	}
	TestEqual(TEXT("Total is x5, never x25"), RepeatedCount, OriginalCount * 5);
	TestEqual(TEXT("Wave count is x5"), RepeatedWaves.Num(), OriginalWaves.Num() * 5);
	Repeated.AdvanceTo(23.0f);
	FReEchoEncounterWaveScheduler Restored;
	Restored.RestoreEvents(Repeated.GetEvents(), Repeated.GetNextEventIndex());
	TestEqual(TEXT("Restore does not replay consumed events"), Restored.AdvanceTo(23.0f).Num(), 0);
	TestEqual(TEXT("Restored remaining events match"),
	          Restored.AdvanceTo(10000.0f).Num(),
	          Repeated.AdvanceTo(10000.0f).Num());
	const int32 CountBeforeInvalid = Repeated.GetEventCount();
	TestFalse(TEXT("Invalid multiplier rejected"),
	          Repeated.ConfigureRepeatedOrdinaryPlan(*Load.Snapshot, TEXT("Encounter.8"), 0, 0.0f, Error));
	TestEqual(TEXT("Rejected plan preserves previous schedule"), Repeated.GetEventCount(), CountBeforeInvalid);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEncounterWaveSchedulerTest,
                                 "ReEcho.Encounter.TableDrivenWaveScheduler",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEncounterWaveSchedulerTest::RunTest(const FString& Parameters)
{
	const FReEchoCsvLoadResult Load =
	    FReEchoCsvDataRegistry::LoadSnapshotFromDirectory(FReEchoCsvDataRegistry::GetDefaultDataDirectory());
	if (!TestTrue(TEXT("Production data loads"), Load.bSuccess) || !Load.Snapshot.IsValid())
	{
		AddError(Load.FormatIssues());
		return false;
	}

	FReEchoEncounterWaveScheduler Scheduler;
	FString Error;
	TestTrue(TEXT("Encounter 1 schedule compiles"), Scheduler.Configure(*Load.Snapshot, TEXT("Encounter.1"), Error));
	TestEqual(TEXT("Three melee and ranged waves create warning and commit events"), Scheduler.GetEventCount(), 12);
	const TArray<FReEchoScheduledSpawnEvent> AtStart = Scheduler.AdvanceTo(0.0f);
	TestEqual(TEXT("Wave one emits only its two role warnings at encounter start"), AtStart.Num(), 2);
	for (const FReEchoScheduledSpawnEvent& Event : AtStart)
	{
		TestEqual(
		    TEXT("Wave one encounter-start event is a warning"), Event.Type, EReEchoScheduledSpawnEventType::Warning);
		TestEqual(TEXT("Wave one warning preserves its full configured lead"), Event.SpawnSeconds, 0.8f);
	}
	TestEqual(TEXT("Wave one does not commit before its warning lead expires"), Scheduler.AdvanceTo(0.79f).Num(), 0);
	const TArray<FReEchoScheduledSpawnEvent> FirstCommit = Scheduler.AdvanceTo(0.8f);
	TestEqual(TEXT("Both wave one role commits fire after the warning lead"), FirstCommit.Num(), 2);
	for (const FReEchoScheduledSpawnEvent& Event : FirstCommit)
	{
		TestEqual(TEXT("Wave one delayed event is a commit"), Event.Type, EReEchoScheduledSpawnEventType::Commit);
	}
	TestEqual(TEXT("No wave two event before its warning lead"), Scheduler.AdvanceTo(9.19f).Num(), 0);
	const TArray<FReEchoScheduledSpawnEvent> Warning = Scheduler.AdvanceTo(9.2f);
	TestEqual(TEXT("Both wave two role warnings fire at 9.2 seconds"), Warning.Num(), 2);
	const TArray<FReEchoScheduledSpawnEvent> Commit = Scheduler.AdvanceTo(10.0f);
	TestEqual(TEXT("Both wave two role commits fire at 10 seconds"), Commit.Num(), 2);

	TestTrue(TEXT("Boss encounter schedule compiles"), Scheduler.Configure(*Load.Snapshot, TEXT("Encounter.8"), Error));
	TestEqual(TEXT("Boss encounter adds a warning and commit for the configured boss"), Scheduler.GetEventCount(), 20);
	const TArray<FReEchoScheduledSpawnEvent> BossStart = Scheduler.AdvanceTo(0.0f);
	TestEqual(TEXT("Boss wave one emits four warned role batches without early commits"), BossStart.Num(), 4);
	const FReEchoScheduledSpawnEvent* BossWarning = BossStart.FindByPredicate(
	    [](const FReEchoScheduledSpawnEvent& Event)
	    {
		    return Event.EnemyRole == TEXT("Boss") && Event.Type == EReEchoScheduledSpawnEventType::Warning;
	    });
	const FReEchoScheduledSpawnEvent* EarlyBossCommit = BossStart.FindByPredicate(
	    [](const FReEchoScheduledSpawnEvent& Event)
	    {
		    return Event.EnemyRole == TEXT("Boss") && Event.Type == EReEchoScheduledSpawnEventType::Commit;
	    });
	TestNotNull(TEXT("Boss warning uses the shared spawn pipeline"), BossWarning);
	TestNull(TEXT("Boss is not committed in the same frame as its warning"), EarlyBossCommit);
	TestEqual(
	    TEXT("Melee and ranged first-wave commits respect their 0.8 second lead"), Scheduler.AdvanceTo(0.8f).Num(), 2);
	const TArray<FReEchoScheduledSpawnEvent> BossLeadExpiry = Scheduler.AdvanceTo(0.9f);
	const FReEchoScheduledSpawnEvent* BossCommit = BossLeadExpiry.FindByPredicate(
	    [](const FReEchoScheduledSpawnEvent& Event)
	    {
		    return Event.EnemyRole == TEXT("Boss") && Event.Type == EReEchoScheduledSpawnEventType::Commit;
	    });
	TestNotNull(TEXT("Boss commits through the shared spawn pipeline after its warning lead"), BossCommit);
	if (BossWarning && BossCommit)
	{
		TestEqual(TEXT("Boss warning keeps the wave-owned enemy id"), BossWarning->EnemyId, FName(TEXT("M_SHEEP")));
		TestEqual(TEXT("Boss commit keeps the wave-owned enemy id"), BossCommit->EnemyId, FName(TEXT("M_SHEEP")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoSpawnWarningCapacityReservationTest,
                                 "ReEcho.Encounter.SpawnWarningCapacityReservation",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoSpawnWarningCapacityReservationTest::RunTest(const FString& Parameters)
{
	const int32 ActiveUnitLimit = 18;
	const int32 LivingCount = 10;
	const int32 FirstReservation = ReEchoSpawnCapacity::CalculateReservationCount(ActiveUnitLimit, LivingCount, 0, 5);
	const int32 SecondReservation =
	    ReEchoSpawnCapacity::CalculateReservationCount(ActiveUnitLimit, LivingCount, FirstReservation, 5);
	const int32 ThirdReservation = ReEchoSpawnCapacity::CalculateReservationCount(
	    ActiveUnitLimit, LivingCount, FirstReservation + SecondReservation, 2);

	TestEqual(TEXT("First warned role reserves all available requested units"), FirstReservation, 5);
	TestEqual(TEXT("Second warned role reserves only the remaining unit capacity"), SecondReservation, 3);
	TestEqual(TEXT("No warning is emitted after earlier batches reserve the unit limit"), ThirdReservation, 0);
	TestEqual(TEXT("An exempt Boss still receives its promised spawn reservation"),
	          ReEchoSpawnCapacity::CalculateReservationCount(ActiveUnitLimit, ActiveUnitLimit, 0, 1, false),
	          1);
	TestEqual(TEXT("Negative requested counts cannot create reservations"),
	          ReEchoSpawnCapacity::CalculateReservationCount(ActiveUnitLimit, LivingCount, 0, -1),
	          0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoSpawnResolverTest,
                                 "ReEcho.Encounter.DeterministicSpawnResolver",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoSpawnResolverTest::RunTest(const FString& Parameters)
{
	const FReEchoCsvLoadResult Load =
	    FReEchoCsvDataRegistry::LoadSnapshotFromDirectory(FReEchoCsvDataRegistry::GetDefaultDataDirectory());
	if (!Load.bSuccess || !Load.Snapshot.IsValid())
	{
		AddError(Load.FormatIssues());
		return false;
	}
	const FReEchoCsvSpawnProfileRow* Profile = Load.Snapshot->FindSpawnProfileByRole(TEXT("Melee"));
	const FReEchoCsvSpawnPolicyRow* Policy = Load.Snapshot->FindEnabledSpawnPolicy();
	if (!TestNotNull(TEXT("Melee profile exists"), Profile) || !TestNotNull(TEXT("Spawn policy exists"), Policy))
	{
		return false;
	}

	FReEchoSpawnResolveRequest Request;
	Request.PlayerAnchor = FVector::ZeroVector;
	Request.EchoAnchor = FVector(-1000.0f, 0.0f, 0.0f);
	Request.EchoAnchorRatio = 1.0f;
	Request.SpawnWorldBounds = FBox2D(FVector2D(-2500.0f, -3400.0f), FVector2D(3500.0f, 2600.0f));
	Request.SpawnCenterWorldZ = 215.0f;
	Request.Seed = 481337;
	Request.Sequence = 3;
	Request.bHasEchoAnchor = true;
	FReEchoResolvedSpawn First;
	FReEchoResolvedSpawn Second;
	FString FirstError;
	FString SecondError;
	TestTrue(TEXT("First solve succeeds"),
	         FReEchoSpawnResolver::Resolve(*Profile, *Policy, Request, First, FirstError));
	TestTrue(TEXT("Second solve succeeds"),
	         FReEchoSpawnResolver::Resolve(*Profile, *Policy, Request, Second, SecondError));
	TestEqual(TEXT("Stable seed produces the same location"), First.Location, Second.Location);
	TestEqual(TEXT("Resolved location preserves the requested final actor center height"), First.Location.Z, 215.0);
	TestFalse(TEXT("Echo anchor concept is dropped under full-map random placement"), First.bUsedEchoAnchor);
	TestTrue(TEXT("Player exclusion distance is still honored"),
	         FVector::Dist2D(First.Location, Request.PlayerAnchor) >= Policy->MinPlayerDistanceCm);
	TestTrue(TEXT("Resolved location stays inside arena bounds"),
	         Request.SpawnWorldBounds.IsInside(FVector2D(First.Location.X, First.Location.Y)));

	// Full-map random placement never fails closed: even a postage-stamp arena always yields a
	// point inside its bounds, so hundreds of concurrent enemies keep flowing in.
	FReEchoSpawnResolveRequest TinyRequest = Request;
	TinyRequest.SpawnWorldBounds = FBox2D(FVector2D(10000.0f, 10000.0f), FVector2D(10010.0f, 10010.0f));
	TinyRequest.ExistingLocations = {FVector(10005.0f, 10005.0f, 0.0f)};
	FReEchoResolvedSpawn TinySpawn;
	FString TinyError;
	TestTrue(TEXT("Full-map random placement always yields a location"),
	         FReEchoSpawnResolver::Resolve(*Profile, *Policy, TinyRequest, TinySpawn, TinyError));
	TestTrue(TEXT("Full-map placement lands inside the tiny arena bounds"),
	         TinyRequest.SpawnWorldBounds.IsInside(FVector2D(TinySpawn.Location.X, TinySpawn.Location.Y)));

	const FReEchoCsvSpawnProfileRow* BossProfile = Load.Snapshot->FindSpawnProfileByRole(TEXT("Boss"));
	if (!TestNotNull(TEXT("Boss has a dedicated spawn profile"), BossProfile))
	{
		return false;
	}
	Request.EchoAnchorRatio = 0.0f;
	Request.SpawnCenterWorldZ = 220.0f;
	Request.Sequence = 9;
	FReEchoResolvedSpawn BossSpawn;
	FString BossError;
	TestTrue(TEXT("Boss spawn resolves through the shared full-map solver"),
	         FReEchoSpawnResolver::Resolve(*BossProfile, *Policy, Request, BossSpawn, BossError));
	TestTrue(TEXT("Boss spawn stays inside arena bounds"),
	         Request.SpawnWorldBounds.IsInside(FVector2D(BossSpawn.Location.X, BossSpawn.Location.Y)));
	TestFalse(TEXT("Boss spawn no longer uses the former fixed world coordinate"),
	          BossSpawn.Location.Equals(FVector(800.0f, 0.0f, 50.0f)));
	return true;
}

#endif
