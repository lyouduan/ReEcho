#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Presentation/Combat/ReEchoCombatPresentationCoordinator.h"
#include "Presentation/Enemy/ReEchoEnemyPresentationComponent.h"
#include "Presentation/Weapon/ReEchoWeaponPresentationCatalog.h"
#include "Presentation/Weapon/ReEchoWeaponPresentationProfile.h"
#include "Weapons/ReEchoWeaponActor.h"
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

	static const FName Keys[] = {TEXT("CrescentBlade"), TEXT("Scythe"), TEXT("Bow"), TEXT("Gun")};
	for (const FName Key : Keys)
	{
		const UReEchoWeaponPresentationProfile* Profile = FReEchoWeaponVisualCatalog::ResolveProfile(Key);
		TestNotNull(*FString::Printf(TEXT("%s has one presentation profile"), *Key.ToString()), Profile);
		if (Profile)
		{
			TestEqual(TEXT("Profile identity matches lookup"), Profile->WeaponVisualKey, Key);
			TestTrue(TEXT("Held weapon ratio remains positive"), Profile->HeldLengthRatio > 0.0f);
			TestTrue(TEXT("Absolute held length override remains positive"), Profile->HeldLengthOverrideCm > 0.0f);
			TestTrue(TEXT("Production weapon size is absolute"), Profile->bOverrideHeldLength);
			const float PlayerLength = AReEchoWeaponActor::ResolveHeldWorldLengthForTests(*Profile, 224.0f, 1.0f);
			const float EchoLength = AReEchoWeaponActor::ResolveHeldWorldLengthForTests(*Profile, 224.0f, 0.75f);
			TestEqual(TEXT("Player and Echo host scales resolve the same weapon length"), PlayerLength, EchoLength);
		}
	}
	const UReEchoWeaponPresentationCatalog* WeaponCatalog = FReEchoWeaponVisualCatalog::ResolveCatalog();
	TestNotNull(TEXT("Weapon catalog owns the shared held layout"), WeaponCatalog);
	if (WeaponCatalog)
	{
		TestTrue(TEXT("Shared right hand anchor is valid"), !WeaponCatalog->RightHandAnchorRatio.ContainsNaN());
		TestTrue(TEXT("Shared left hand anchor is valid"), !WeaponCatalog->LeftHandAnchorRatio.ContainsNaN());
		TestFalse(TEXT("Shared left and right hand anchors are independently authored"),
		          WeaponCatalog->LeftHandAnchorRatio.Equals(WeaponCatalog->RightHandAnchorRatio, KINDA_SMALL_NUMBER));
	}
	const FVector CameraRight = FVector(0.6f, 0.8f, 0.0f).GetSafeNormal();
	const FVector AuthoredHeldOffset(12.0f, 7.0f, 3.0f);
	const FVector RightFacingOffset =
	    AReEchoWeaponActor::ResolveFacingHeldOffsetForTests(AuthoredHeldOffset, 1.0f, CameraRight);
	const FVector LeftFacingOffset =
	    AReEchoWeaponActor::ResolveFacingHeldOffsetForTests(AuthoredHeldOffset, -1.0f, CameraRight);
	TestTrue(TEXT("Right-facing weapon keeps the authored hand-relative offset"),
	         RightFacingOffset.Equals(AuthoredHeldOffset, KINDA_SMALL_NUMBER));
	TestTrue(TEXT("Left-facing weapon mirrors the horizontal hand-relative offset"),
	         FMath::IsNearlyEqual(FVector::DotProduct(LeftFacingOffset, CameraRight),
	                              -FVector::DotProduct(RightFacingOffset, CameraRight),
	                              KINDA_SMALL_NUMBER));
	TestTrue(TEXT("Facing mirror preserves the offset component outside the camera horizontal axis"),
	         (LeftFacingOffset - FVector::DotProduct(LeftFacingOffset, CameraRight) * CameraRight)
	             .Equals(RightFacingOffset - FVector::DotProduct(RightFacingOffset, CameraRight) * CameraRight,
	                     KINDA_SMALL_NUMBER));
	const FVector AuthoredBossRightOffset(1.0f, 20.0f, 3.0f);
	const FVector AuthoredBossLeftOffset(-2.0f, -18.0f, 4.0f);
	TestTrue(TEXT("Boss weapon selects the Staff DA right-facing offset"),
	         UReEchoEnemyPresentationComponent::ResolveBossWeaponFacingOffsetForTests(
	             1.0f, AuthoredBossRightOffset, AuthoredBossLeftOffset)
	             .Equals(AuthoredBossRightOffset, KINDA_SMALL_NUMBER));
	TestTrue(TEXT("Boss weapon selects the independently authored Staff DA left-facing offset"),
	         UReEchoEnemyPresentationComponent::ResolveBossWeaponFacingOffsetForTests(
	             -1.0f, AuthoredBossRightOffset, AuthoredBossLeftOffset)
	             .Equals(AuthoredBossLeftOffset, KINDA_SMALL_NUMBER));
	TestTrue(TEXT("Longsword triple swing starts at upper sixty degrees"),
	         FMath::IsNearlyEqual(
	             AReEchoWeaponActor::ResolveTripleSwingAngleForTests(0.0f, 1.0f), PI / 3.0f, KINDA_SMALL_NUMBER));
	TestTrue(TEXT("Longsword first pass reaches lower sixty degrees"),
	         FMath::IsNearlyEqual(AReEchoWeaponActor::ResolveTripleSwingAngleForTests(1.0f / 3.0f, 1.0f),
	                              -PI / 3.0f,
	                              KINDA_SMALL_NUMBER));
	TestTrue(TEXT("Longsword second pass returns to upper sixty degrees"),
	         FMath::IsNearlyEqual(AReEchoWeaponActor::ResolveTripleSwingAngleForTests(2.0f / 3.0f, 1.0f),
	                              PI / 3.0f,
	                              KINDA_SMALL_NUMBER));
	TestTrue(TEXT("Longsword third pass ends at lower sixty degrees"),
	         FMath::IsNearlyEqual(
	             AReEchoWeaponActor::ResolveTripleSwingAngleForTests(1.0f, 1.0f), -PI / 3.0f, KINDA_SMALL_NUMBER));
	TestNotNull(TEXT("Sage MoonStaff animation helper retains its presentation profile"),
	            FReEchoWeaponVisualCatalog::ResolveProfile(TEXT("MoonStaff")));
	TestFalse(TEXT("Bow has held visual"),
	          FReEchoWeaponVisualCatalog::ResolveProfile(TEXT("Bow"))->HeldTexture.IsNull());
	TestEqual(TEXT("Bow mirrors only while facing left"),
	          FReEchoWeaponVisualCatalog::ResolveProfile(TEXT("Bow"))->HeldMirrorRule,
	          EReEchoHeldWeaponMirrorRule::WhenFacingLeft);
	TestEqual(TEXT("Gun mirrors only while facing right"),
	          FReEchoWeaponVisualCatalog::ResolveProfile(TEXT("Gun"))->HeldMirrorRule,
	          EReEchoHeldWeaponMirrorRule::WhenFacingRight);
	TestEqual(TEXT("Longsword points along the approved upper-right screen direction"),
	          FReEchoWeaponVisualCatalog::ResolveProfile(TEXT("CrescentBlade"))->HeldPlanarAngleOffsetDegrees,
	          90.0f);
	TestTrue(TEXT("Sword uses committed attack VFX"),
	         FReEchoWeaponVisualCatalog::ResolveProfile(TEXT("CrescentBlade"))->AttackCommitted.IsConfigured());
	const UReEchoWeaponPresentationProfile* SwordProfile =
	    FReEchoWeaponVisualCatalog::ResolveProfile(TEXT("CrescentBlade"));
	TestTrue(TEXT("Sword slash preserves its configured world size"), SwordProfile->AttackCommitted.bPreserveWorldSize);
	TestTrue(TEXT("Bow uses travel VFX"),
	         FReEchoWeaponVisualCatalog::ResolveProfile(TEXT("Bow"))->Travel.IsConfigured());
	return true;
}

#endif
