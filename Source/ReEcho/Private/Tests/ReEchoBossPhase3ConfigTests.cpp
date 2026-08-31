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
	TestEqual(TEXT("Phase3 contains Skill03 and independent GroundTriple"), Definition.Abilities.Num(), 2);
	const FReEchoEnemyAbilityDefinition& Triple = Definition.Abilities[1];
	TestEqual(TEXT("Independent triple identity"), Triple.Id, FName(TEXT("M_SHEEP_GroundTriple")));
	TestTrue(TEXT("Triple is stationary"), Triple.bGroundedSlam);
	TestEqual(TEXT("Triple is phase3 only"), Triple.MinPhaseIndex, 3);
	TestEqual(TEXT("Triple always has three beats"), Triple.ComboMin, 3);
	TestEqual(TEXT("Triple always has three beats maximum"), Triple.ComboMax, 3);
	TestTrue(TEXT("Normal combat triple retains authored damage"), Triple.Damage > 0.0f);
	TestEqual(TEXT("Opening uses independent triple"), Definition.BossPhases[0].OpeningAbilityId, Triple.Id);
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
	TestEqual(TEXT("Phase3 health multiplier comes from programmer DA"),
	          Definition.BossPhases[0].PreviousPhasesHealthMultiplier,
	          5.0f);
	TestEqual(TEXT("Phase3 repeats the ordinary plan five times"), Config->SpawnPlanRepetitions, 5);
	TestEqual(TEXT("Opening forces three strikes"), Definition.BossPhases[0].OpeningStrikeCount, 3);
	TestEqual(TEXT("Opening repulse distance"), Definition.BossPhases[0].OpeningRepulseDistanceCm, 200.0f);
	TestEqual(TEXT("Opening repulse duration"), Definition.BossPhases[0].OpeningRepulseSeconds, 0.3f);
	TestFalse(TEXT("The DA never overlays another enemy"), Config->ApplyTo(TEXT("M_FOX"), Other));
	return true;
}

#endif
