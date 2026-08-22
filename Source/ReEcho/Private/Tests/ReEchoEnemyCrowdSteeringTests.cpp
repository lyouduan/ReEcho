#if WITH_DEV_AUTOMATION_TESTS

#include "Graybox/ReEchoEnemyCrowdSteering.h"

#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEnemyCrowdSteeringTest,
                                 "ReEcho.Enemies.Crowd.Steering",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEnemyCrowdSteeringTest::RunTest(const FString& Parameters)
{
	FReEchoEnemyCrowdSteeringInput Input;
	Input.SelfLocation = FVector::ZeroVector;
	Input.TargetLocation = FVector(500.0f, 0.0f, 0.0f);
	Input.DesiredMovementDelta = FVector(10.0f, 0.0f, 0.0f);
	Input.SelfRadiusCm = 25.0f;
	Input.PreferredTargetDistanceCm = 75.0f;
	Input.SpawnIndex = 2;
	FReEchoEnemyCrowdNeighbor& Neighbor = Input.Neighbors.AddDefaulted_GetRef();
	Neighbor.Location = FVector(15.0f, 0.0f, 0.0f);
	Neighbor.RadiusCm = 25.0f;
	Neighbor.SpawnIndex = 3;

	const FVector First = FReEchoEnemyCrowdSteering::ResolveMovement(Input);
	const FVector Second = FReEchoEnemyCrowdSteering::ResolveMovement(Input);
	TestTrue(TEXT("Crowd steering is deterministic"), First.Equals(Second, KINDA_SMALL_NUMBER));
	TestEqual(TEXT("Crowd steering preserves movement budget"), First.Size2D(), 10.0);
	TestTrue(TEXT("A forward neighbor creates a lateral bypass"), FMath::Abs(First.Y) > KINDA_SMALL_NUMBER);

	Input.Neighbors[0].Location = Input.SelfLocation;
	const FVector OverlapRecovery = FReEchoEnemyCrowdSteering::ResolveMovement(Input);
	TestFalse(TEXT("Exact overlap resolves to a finite stable direction"), OverlapRecovery.ContainsNaN());
	TestEqual(TEXT("Exact overlap recovery preserves movement budget"), OverlapRecovery.Size2D(), 10.0);

	Input.Neighbors[0].Location = FVector(15.0f, 0.0f, 0.0f);
	Input.BlockedSeconds = 0.4f;
	const FVector BlockedRecovery = FReEchoEnemyCrowdSteering::ResolveMovement(Input);
	TestFalse(TEXT("Sustained blockage changes the deterministic bypass"),
	          BlockedRecovery.Equals(First, KINDA_SMALL_NUMBER));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEnemyCrowdCollisionPolicyTest,
                                 "ReEcho.Enemies.Crowd.CollisionPolicy",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEnemyCrowdCollisionPolicyTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Two ordinary enemies ignore swept movement collision"),
	         FReEchoEnemyCrowdSteering::ShouldIgnoreMovementCollision(EReEchoEnemyArchetype::Grunt,
	                                                                  EReEchoEnemyArchetype::Ranged));
	TestFalse(TEXT("Ordinary enemy retains hard movement collision against Boss"),
	          FReEchoEnemyCrowdSteering::ShouldIgnoreMovementCollision(EReEchoEnemyArchetype::Grunt,
	                                                                   EReEchoEnemyArchetype::Boss));
	TestFalse(TEXT("Boss retains hard movement collision against ordinary enemy"),
	          FReEchoEnemyCrowdSteering::ShouldIgnoreMovementCollision(EReEchoEnemyArchetype::Boss,
	                                                                   EReEchoEnemyArchetype::Grunt));
	return true;
}

#endif
