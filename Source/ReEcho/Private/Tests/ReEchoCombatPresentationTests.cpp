#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Presentation/Animation2D/ReEcho2DCharacterPresentationProfile.h"
#include "Presentation/Combat/ReEchoCombatPresentationCoordinator.h"
#include "Presentation/Weapon/ReEchoWeaponPresentationProfile.h"
#include "Weapons/ReEchoWeaponVisualCatalog.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoCombatPresentationLifecycleTest,
                                 "ReEcho.Presentation.Combat.Lifecycle",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoCombatPresentationLifecycleTest::RunTest(const FString& Parameters)
{
	UReEchoCombatPresentationCoordinator* Coordinator = NewObject<UReEchoCombatPresentationCoordinator>();
	FReEchoEnemyActionCommittedEvent Basic;
	Basic.Attack.Sequence = 1;
	Coordinator->ConsumeEnemyActionCommittedForTests(Basic);
	TestFalse(TEXT("One-shot basic action does not retain lifecycle ownership"),
	          Coordinator->HasActiveActionForTests());
	FReEchoEnemySpecialActionEvent Windup;
	Windup.AbilityId = TEXT("M_FOX_Dash");
	Windup.Type = EReEchoEnemySpecialActionEventType::WindupStarted;
	Coordinator->ConsumeSpecialActionForTests(Windup);
	Coordinator->ConsumeSpecialActionForTests(Windup);
	TestEqual(TEXT("Duplicate windup publishes once"), Coordinator->GetPublishedPhaseCountForTests(), 2);
	TestEqual(TEXT("Windup owns active action"),
	          Coordinator->GetActivePhaseForTests(),
	          EReEchoPresentationActionPhase::Windup);

	FReEchoEnemySpecialActionEvent Commit = Windup;
	Commit.Type = EReEchoEnemySpecialActionEventType::ActionCommitted;
	Coordinator->ConsumeSpecialActionForTests(Commit);
	TestEqual(TEXT("Commit advances the same action"),
	          Coordinator->GetActivePhaseForTests(),
	          EReEchoPresentationActionPhase::Committed);

	FReEchoEnemySpecialActionEvent End = Windup;
	End.Type = EReEchoEnemySpecialActionEventType::ActionEnded;
	Coordinator->ConsumeSpecialActionForTests(End);
	Coordinator->ConsumeSpecialActionForTests(End);
	TestFalse(TEXT("End releases active action"), Coordinator->HasActiveActionForTests());
	TestEqual(TEXT("Duplicate end is ignored"), Coordinator->GetPublishedPhaseCountForTests(), 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoCombatPresentationCapabilityTest,
                                 "ReEcho.Presentation.Combat.Capabilities",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoCombatPresentationCapabilityTest::RunTest(const FString& Parameters)
{
	TestFalse(TEXT("Ordinary enemy never has weapon track"),
	          UReEchoCombatPresentationCoordinator::IsWeaponTrackEnabled(EReEchoPresentationHostKind::OrdinaryEnemy,
	                                                                     TEXT("CrescentBlade")));
	TestFalse(TEXT("Boss requires explicit weapon presentation"),
	          UReEchoCombatPresentationCoordinator::IsWeaponTrackEnabled(EReEchoPresentationHostKind::Boss, NAME_None));
	TestTrue(TEXT("Configured Boss may have weapon track"),
	         UReEchoCombatPresentationCoordinator::IsWeaponTrackEnabled(EReEchoPresentationHostKind::Boss,
	                                                                    TEXT("CrescentBlade")));
	TestTrue(
	    TEXT("Player equipped weapon enables weapon track"),
	    UReEchoCombatPresentationCoordinator::IsWeaponTrackEnabled(EReEchoPresentationHostKind::Player, TEXT("Bow")));

	static const FName Keys[] = {
	    TEXT("CrescentBlade"), TEXT("Scythe"), TEXT("Whip"), TEXT("Bow"), TEXT("Gun"), TEXT("Staff")};
	for (const FName Key : Keys)
	{
		const UReEchoWeaponPresentationProfile* Profile = FReEchoWeaponVisualCatalog::ResolveProfile(Key);
		TestNotNull(*FString::Printf(TEXT("%s has one presentation profile"), *Key.ToString()), Profile);
		if (Profile)
		{
			TestEqual(TEXT("Profile identity matches lookup"), Profile->WeaponVisualKey, Key);
			TestTrue(TEXT("Held weapon ratio remains positive"), Profile->HeldLengthRatio > 0.0f);
			TestTrue(TEXT("Absolute held length override remains positive"), Profile->HeldLengthOverrideCm > 0.0f);
		}
	}
	const UReEcho2DCharacterPresentationProfile* DefaultCharacterProfile =
	    GetDefault<UReEcho2DCharacterPresentationProfile>();
	TestTrue(TEXT("Character profile exposes a normalized weapon anchor"),
	         !DefaultCharacterProfile->WeaponAnchorRatio.ContainsNaN());
	TestTrue(TEXT("Missing legacy Whip hand art remains an explicit empty optional field"),
	         FReEchoWeaponVisualCatalog::ResolveProfile(TEXT("Whip"))->HeldTexture.IsNull());
	TestFalse(TEXT("Bow has held visual"),
	          FReEchoWeaponVisualCatalog::ResolveProfile(TEXT("Bow"))->HeldTexture.IsNull());
	TestTrue(TEXT("Longsword owns full-spin weapon motion"),
	         FReEchoWeaponVisualCatalog::ResolveProfile(TEXT("CrescentBlade"))->MotionMode ==
	             EReEchoWeaponMotionMode::FullSpin);
	TestTrue(TEXT("Sword uses committed attack VFX"),
	         FReEchoWeaponVisualCatalog::ResolveProfile(TEXT("CrescentBlade"))->AttackCommitted.IsConfigured());
	TestTrue(TEXT("Bow uses travel VFX"),
	         FReEchoWeaponVisualCatalog::ResolveProfile(TEXT("Bow"))->Travel.IsConfigured());
	return true;
}

#endif
