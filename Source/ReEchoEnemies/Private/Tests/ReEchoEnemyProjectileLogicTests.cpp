#include "Enemies/ReEchoEnemyProjectileLogic.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEnemyProjectilePathTest,
                                 "ReEcho.Enemies.Boss.Projectile.PathAndExpiry",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEnemyProjectilePathTest::RunTest(const FString& Parameters)
{
	FReEchoEnemyProjectileDefinition Definition;
	Definition.InitialLocation = FVector(10.0f, 20.0f, 30.0f);
	Definition.Direction = FVector(2.0f, 0.0f, 0.0f);
	Definition.SpeedCmPerSecond = 100.0f;
	Definition.MaxRangeCm = 25.0f;

	FReEchoEnemyProjectileSnapshot Snapshot;
	TestTrue(TEXT("Valid projectile initializes"),
	         FReEchoEnemyProjectileLogic::Initialize(Definition, Snapshot));
	const FReEchoEnemyProjectileAdvanceResult First =
	    FReEchoEnemyProjectileLogic::Advance(Definition, 0.1f, Snapshot);
	TestEqual(TEXT("First segment starts at current point"), First.PreviousLocation, Definition.InitialLocation);
	TestEqual(TEXT("First segment moves by speed times time"),
	          First.NewLocation,
	          FVector(20.0f, 20.0f, 30.0f));
	TestFalse(TEXT("First segment remains active"), First.bExpiredByRange);

	const FReEchoEnemyProjectileAdvanceResult Last =
	    FReEchoEnemyProjectileLogic::Advance(Definition, 1.0f, Snapshot);
	TestEqual(TEXT("Final segment clamps exactly to remaining range"), Last.SegmentDistanceCm, 15.0f);
	TestEqual(TEXT("Final point is exact range endpoint"),
	          Last.NewLocation,
	          FVector(35.0f, 20.0f, 30.0f));
	TestTrue(TEXT("Range expiry is explicit"), Last.bExpiredByRange);
	TestFalse(TEXT("Expired state is inactive"), Snapshot.bActive);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEnemyProjectileSnapshotTest,
                                 "ReEcho.Enemies.Boss.Projectile.SnapshotRestore",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEnemyProjectileSnapshotTest::RunTest(const FString& Parameters)
{
	FReEchoEnemyProjectileDefinition Definition;
	Definition.Direction = FVector(1.0f, 1.0f, 0.0f);
	Definition.SpeedCmPerSecond = 60.0f;
	Definition.MaxRangeCm = 300.0f;

	FReEchoEnemyProjectileSnapshot Original;
	FReEchoEnemyProjectileLogic::Initialize(Definition, Original);
	FReEchoEnemyProjectileLogic::Advance(Definition, 0.125f, Original);
	FReEchoEnemyProjectileSnapshot Restored;
	TestTrue(TEXT("Valid snapshot restores"),
	         FReEchoEnemyProjectileLogic::RestoreSnapshot(Definition, Original, Restored));

	const FReEchoEnemyProjectileAdvanceResult OriginalNext =
	    FReEchoEnemyProjectileLogic::Advance(Definition, 0.2f, Original);
	const FReEchoEnemyProjectileAdvanceResult RestoredNext =
	    FReEchoEnemyProjectileLogic::Advance(Definition, 0.2f, Restored);
	TestEqual(TEXT("Restored previous point matches"),
	          RestoredNext.PreviousLocation,
	          OriginalNext.PreviousLocation);
	TestEqual(TEXT("Restored new point matches"), RestoredNext.NewLocation, OriginalNext.NewLocation);
	TestEqual(TEXT("Restored distance matches"),
	          Restored.DistanceTravelledCm,
	          Original.DistanceTravelledCm);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
