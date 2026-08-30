#include "Data/ReEchoBossPhase3Config.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoBossPhase3ConfigOverlayTest,
                                 "ReEcho.Enemies.Boss.Phase3.ProgrammerDataAssetOverlay",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoBossPhase3ConfigOverlayTest::RunTest(const FString& Parameters)
{
	UReEchoBossPhase3Config* Config = NewObject<UReEchoBossPhase3Config>();
	// These are DA defaults; the host extends the final timeline for camera push and sacrifice presentation.
	TestEqual(TEXT("Phase2 base transformation lasts 2.0 seconds"), Config->Phase2Presentation.DurationSeconds, 2.0f);
	TestEqual(TEXT("Phase3 base transformation lasts 2.4 seconds"), Config->Phase3Presentation.DurationSeconds, 2.4f);
	TestEqual(TEXT("Phase3 base burst occurs at 1.3 seconds"), Config->Phase3Presentation.BurstSeconds, 1.3f);
	TestEqual(TEXT("Phase2 camera push lasts 1.3 seconds"), Config->Phase2Presentation.CameraPushSeconds, 1.3f);
	TestEqual(TEXT("Phase3 camera push lasts 1.3 seconds"), Config->Phase3Presentation.CameraPushSeconds, 1.3f);
	TestEqual(TEXT("Phase2 camera return stays 0.8 seconds"), Config->Phase2Presentation.CameraMoveSeconds, 0.8f);
	TestEqual(TEXT("Phase3 camera return stays 0.8 seconds"), Config->Phase3Presentation.CameraMoveSeconds, 0.8f);
	TestTrue(TEXT("Phase3 camera push and shake are stronger than Phase2"),
	         Config->Phase3Presentation.CameraWidthRatio < Config->Phase2Presentation.CameraWidthRatio &&
	             Config->Phase3Presentation.ShakeAmplitudeCm > Config->Phase2Presentation.ShakeAmplitudeCm);
	FReEchoEnemyDefinition Definition;
	Definition.Archetype = EReEchoEnemyArchetype::Boss;
	FReEchoEnemyAbilityDefinition& Existing = Definition.Abilities.AddDefaulted_GetRef();
	Existing.Id = TEXT("M_SHEEP_BlinkSlam");
	Existing.BehaviorId = TEXT("Boss.BlinkSlam");
	Existing.SequenceOrder = 4;
	Existing.bEnabled = true;

	TestTrue(TEXT("The Sheep programmer DA overlays the compiled boss definition"),
	         Config->ApplyTo(TEXT("M_SHEEP"), Definition));
	TestEqual(TEXT("Phase3 reuses exactly the existing Skill03 ability"), Definition.Abilities.Num(), 1);
	const FReEchoEnemyAbilityDefinition& Phase3Ability = Definition.Abilities[0];
	TestEqual(TEXT("Phase3 ability identity"), Phase3Ability.Id, FName(TEXT("M_SHEEP_BlinkSlam")));
	TestEqual(TEXT("Phase3 ability behavior"), Phase3Ability.BehaviorId, FName(TEXT("Boss.BlinkSlam")));
	TestEqual(TEXT("Phase3 ability minimum phase"), Phase3Ability.MinPhaseIndex, 1);
	TestEqual(TEXT("Phase3 ability maximum phase"), Phase3Ability.MaxPhaseIndex, 3);
	TestEqual(TEXT("Phase3 combo minimum"), Phase3Ability.ComboMin, 1);
	TestEqual(TEXT("Phase3 combo maximum"), Phase3Ability.ComboMax, 3);
	TestEqual(TEXT("Phase3 definition count"), Definition.BossPhases.Num(), 1);
	TestEqual(
	    TEXT("Phase3 trigger includes the fifteen-second boundary"), Definition.BossPhases[0].TriggerSeconds, 15.0f);
	TestEqual(
	    TEXT("Phase3 refills to its programmer-authored health"), Definition.BossPhases[0].PhaseMaxHealth, 500.0f);

	FReEchoEnemyDefinition Other = Definition;
	TestFalse(TEXT("The DA never overlays another enemy"), Config->ApplyTo(TEXT("M_FOX"), Other));
	return true;
}

#endif
