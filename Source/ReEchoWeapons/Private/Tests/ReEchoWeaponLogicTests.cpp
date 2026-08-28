#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "GameFramework/Actor.h"
#include "Weapons/ReEchoWeaponGeometry.h"
#include "Weapons/ReEchoWeaponLogic.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoMeleeSphereGeometryTest,
                                 "ReEcho.Weapons.Geometry.MeleeSphere",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoMeleeSphereGeometryTest::RunTest(const FString& Parameters)
{
	const FVector Origin(100.0f, -50.0f, 25.0f);
	TestTrue(TEXT("Scythe sphere includes a target directly above within radius"),
	         ReEchoWeaponGeometry::IsInsideMeleeSphere(Origin, Origin + FVector(0.0f, 0.0f, 249.0f), 250.0f));
	TestTrue(TEXT("Scythe sphere includes its exact three-dimensional boundary"),
	         ReEchoWeaponGeometry::IsInsideMeleeSphere(Origin, Origin + FVector(150.0f, 0.0f, 200.0f), 250.0f));
	TestFalse(TEXT("Scythe sphere excludes a target outside the combined XYZ radius"),
	          ReEchoWeaponGeometry::IsInsideMeleeSphere(Origin, Origin + FVector(200.0f, 0.0f, 151.0f), 250.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoWeaponLogicCadenceTest,
                                 "ReEcho.Weapons.Logic.SingleCadence",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoWeaponLogicCadenceTest::RunTest(const FString& Parameters)
{
	FReEchoWeaponDefinition Definition;
	Definition.WeaponId = TEXT("TestWeapon");
	Definition.AttackPatternId = TEXT("TestPattern");
	Definition.AttackIntervalSeconds = 0.28f;
	Definition.DamageCoefficient = 1.0f;
	FReEchoWeaponStepDefinition Step;
	Step.StepId = TEXT("Step1");
	Step.DurationSeconds = 0.80f;
	Step.DamageCoefficient = 1.0f;
	Definition.AttackSteps.Add(Step);

	FReEchoWeaponLogic Logic;
	TestTrue(TEXT("Valid weapon definition initializes"), Logic.Initialize(Definition));
	AActor* Source = NewObject<AActor>();
	FReEchoStatBlock Stats;
	Stats.PhysicalAttack = 10.0f;
	Stats.AttackSpeed = 1.0f;
	FReEchoWeaponAttackCommit First;
	TestTrue(TEXT("First attack commits"), Logic.TryCommitBasicAttack(Source, Stats, First));
	TestTrue(TEXT("Attack identity includes its source"), First.Attack.IsValid());
	TestEqual(TEXT("Damage is computed by weapon logic"), First.RawDamage, 10.0f);
	TestEqual(TEXT("Only weapon interval gates readiness"), Logic.GetSnapshot().ReadinessRemainingSeconds, 0.28f);
	TestEqual(
	    TEXT("Step duration remains observable behavior state"), Logic.GetSnapshot().BehaviorRemainingSeconds, 0.80f);
	Logic.RollbackLastCommit();
	TestEqual(TEXT("Rejected world execution restores cadence"), Logic.GetSnapshot().ReadinessRemainingSeconds, 0.0f);
	TestEqual(TEXT("Rejected world execution restores step progression"), Logic.GetSnapshot().SuccessfulAttackCount, 0);
	TestTrue(TEXT("Retry after rollback commits"), Logic.TryCommitBasicAttack(Source, Stats, First));
	Logic.ConfirmLastCommit();

	FReEchoWeaponAttackCommit Rejected;
	TestFalse(TEXT("Busy cadence rejects a second request"), Logic.TryCommitBasicAttack(Source, Stats, Rejected));
	Logic.Tick(0.28f);
	FReEchoWeaponAttackCommit Second;
	TestTrue(TEXT("Attack commits when weapon interval expires"), Logic.TryCommitBasicAttack(Source, Stats, Second));
	TestTrue(TEXT("Whole attack identity changes between commits"), Second.Attack != First.Attack);
	return true;
}

#endif
