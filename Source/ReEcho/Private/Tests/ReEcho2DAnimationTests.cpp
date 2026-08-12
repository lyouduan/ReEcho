#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "PaperFlipbook.h"
#include "Presentation/Animation2D/ReEcho2DAnimationComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEcho2DAnimationAssetProfilesTest,
                                 "ReEcho.Presentation.Animation2D.AssetProfiles",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEcho2DAnimationAssetProfilesTest::RunTest(const FString& Parameters)
{
	UPaperFlipbook* PlayerFlipbook = LoadObject<UPaperFlipbook>(nullptr, TEXT("/Game/2DAnim/Flipbook/Idel.Idel"));
	UPaperFlipbook* GruntFlipbook = LoadObject<UPaperFlipbook>(nullptr, TEXT("/Game/2DAnim/Flipbook/01_2.01_2"));
	TestNotNull(TEXT("J_SPADE idle Flipbook is loadable"), PlayerFlipbook);
	TestNotNull(TEXT("Grunt default Flipbook is loadable"), GruntFlipbook);

	UReEcho2DAnimationComponent* Component = NewObject<UReEcho2DAnimationComponent>();
	FReEcho2DAnimationProfile Profile;
	Profile.DefaultFlipbook = PlayerFlipbook;
	Profile.WorldHeight = 224.0f;
	TestEqual(TEXT("Valid profile activates"),
	          Component->ActivateProfile(Profile),
	          EReEcho2DAnimationActivationResult::Activated);
	TestTrue(TEXT("Activated component owns the requested Flipbook"),
	         Component->IsAnimationActive() && Component->GetFlipbook() == PlayerFlipbook);
	TestTrue(TEXT("Unassigned gameplay states resolve to the default Flipbook"),
	         Component->SetAnimationState(EReEcho2DAnimationState::Death) &&
	             Component->GetFlipbook() == PlayerFlipbook);

	FReEcho2DAnimationProfile MissingProfile;
	TestEqual(TEXT("Missing Flipbook fails safely"),
	          Component->ActivateProfile(MissingProfile),
	          EReEcho2DAnimationActivationResult::MissingFlipbook);
	TestFalse(TEXT("Failed activation leaves animation disabled"), Component->IsAnimationActive());
	return true;
}

#endif
