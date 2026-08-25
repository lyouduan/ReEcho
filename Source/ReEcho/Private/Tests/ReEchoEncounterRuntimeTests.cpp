#include "Data/ReEchoCsvDataRegistry.h"
#include "Encounter/ReEchoEncounterRuntime.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

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
	TestEqual(TEXT("Wave one warnings and commits are deterministic at encounter start"), AtStart.Num(), 4);
	TestEqual(
	    TEXT("Warning sorts before commit at the same time"), AtStart[0].Type, EReEchoScheduledSpawnEventType::Warning);
	TestEqual(TEXT("No wave two event before its warning lead"), Scheduler.AdvanceTo(9.19f).Num(), 0);
	const TArray<FReEchoScheduledSpawnEvent> Warning = Scheduler.AdvanceTo(9.2f);
	TestEqual(TEXT("Both wave two role warnings fire at 9.2 seconds"), Warning.Num(), 2);
	const TArray<FReEchoScheduledSpawnEvent> Commit = Scheduler.AdvanceTo(10.0f);
	TestEqual(TEXT("Both wave two role commits fire at 10 seconds"), Commit.Num(), 2);
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
	Request.ArenaHalfX = 3000.0f;
	Request.ArenaHalfY = 3000.0f;
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
	TestTrue(TEXT("Ratio one chooses the available echo anchor"), First.bUsedEchoAnchor);
	TestTrue(TEXT("Player exclusion distance is honored"),
	         FVector::Dist2D(First.Location, Request.PlayerAnchor) >= Policy->MinPlayerDistanceCm);
	TestTrue(TEXT("Echo exclusion distance is honored"),
	         FVector::Dist2D(First.Location, Request.EchoAnchor) >= Policy->MinEchoDistanceCm);
	return true;
}

#endif
