#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "GameFramework/Actor.h"
#include "Weapons/ReEchoWeaponLogic.h"

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoWeaponAttackSpeedDurationFormulaTest,
                                 "ReEcho.Weapons.Logic.AttackSpeedDurationFormula",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoWeaponAttackSpeedDurationFormulaTest::RunTest(const FString& Parameters)
{
	auto MakeLogic = [this](const float EquipmentModifier)
	{
		FReEchoWeaponDefinition Definition;
		Definition.WeaponId = TEXT("AttackSpeedTestWeapon");
		Definition.AttackPatternId = TEXT("AttackSpeedTestPattern");
		Definition.AttackIntervalSeconds = 0.3f;
		Definition.AttackSpeedModifier = EquipmentModifier;
		FReEchoWeaponStepDefinition& Step = Definition.AttackSteps.AddDefaulted_GetRef();
		Step.StepId = TEXT("Step1");
		Step.DurationSeconds = 0.8f;

		FReEchoWeaponLogic Logic;
		TestTrue(TEXT("Attack-speed test definition initializes"), Logic.Initialize(Definition));
		return Logic;
	};

	FReEchoStatBlock NeutralStats;
	NeutralStats.AttackSpeed = 1.0f;
	FReEchoWeaponLogic Neutral = MakeLogic(0.0f);
	TestTrue(TEXT("Neutral attack speed keeps the initial 0.3 second duration"),
	         FMath::IsNearlyEqual(Neutral.GetAttackInterval(NeutralStats), 0.3f));

	FReEchoWeaponLogic Haste = MakeLogic(0.6f);
	TestTrue(TEXT("+60% attack speed scales 0.3 seconds to 0.12 seconds"),
	         FMath::IsNearlyEqual(Haste.GetAttackInterval(NeutralStats), 0.12f));

	FReEchoWeaponLogic Slow = MakeLogic(-0.3f);
	TestTrue(TEXT("-30% attack speed scales 0.3 seconds to 0.39 seconds"),
	         FMath::IsNearlyEqual(Slow.GetAttackInterval(NeutralStats), 0.39f, 0.0001f));
	TestTrue(TEXT("Step behavior duration consumes the same -30% duration scale"),
	         FMath::IsNearlyEqual(Slow.GetScaledAttackDuration(0.8f, NeutralStats), 1.04f, 0.0001f));

	FReEchoWeaponLogic ExtremeSlow = MakeLogic(-5.0f);
	TestTrue(TEXT("-500% attack speed scales 0.3 seconds to 1.8 seconds"),
	         FMath::IsNearlyEqual(ExtremeSlow.GetAttackInterval(NeutralStats), 1.8f, 0.0001f));

	FReEchoStatBlock RuntimeLayerStats = NeutralStats;
	RuntimeLayerStats.AttackSpeed = 1.01f;
	TestTrue(TEXT("Static +60% and runtime +1% add before the single duration conversion"),
	         FMath::IsNearlyEqual(Haste.GetAttackInterval(RuntimeLayerStats), 0.117f, 0.0001f));

	FReEchoWeaponLogic Capped = MakeLogic(1.0f);
	TestTrue(TEXT("Zero or negative formula results keep the 0.01 second timer floor"),
	         FMath::IsNearlyEqual(Capped.GetAttackInterval(NeutralStats), 0.01f));
	return true;
}

#endif
