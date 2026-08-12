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
	UPaperFlipbook* StaffAttackFlipbook = LoadObject<UPaperFlipbook>(nullptr, TEXT("/Game/2DAnim/Flipbook/s.s"));
	TestNotNull(TEXT("J_SPADE idle Flipbook is loadable"), PlayerFlipbook);
	TestNotNull(TEXT("Grunt default Flipbook is loadable"), GruntFlipbook);
	TestNotNull(TEXT("Moon Staff attack Flipbook is loadable"), StaffAttackFlipbook);
	TestTrue(TEXT("J_SPADE Flipbook has non-empty render bounds"),
	         PlayerFlipbook && PlayerFlipbook->GetRenderBounds().BoxExtent.Z > 0.0f);
	TestTrue(TEXT("Grunt Flipbook has non-empty render bounds"),
	         GruntFlipbook && GruntFlipbook->GetRenderBounds().BoxExtent.Z > 0.0f);

	UReEcho2DAnimationComponent* Component = NewObject<UReEcho2DAnimationComponent>();
	FReEcho2DAnimationProfile Profile;
	Profile.DefaultFlipbook = PlayerFlipbook;
	Profile.StateFlipbooks.Add(EReEcho2DAnimationState::Attack, StaffAttackFlipbook);
	Profile.WorldHeight = 224.0f;
	Profile.bUseNativeScale = true;
	TestEqual(TEXT("Valid profile activates"),
	          Component->ActivateProfile(Profile),
	          EReEcho2DAnimationActivationResult::Activated);
	TestTrue(TEXT("Activated component owns the requested Flipbook"),
	         Component->IsAnimationActive() && Component->GetFlipbook() == PlayerFlipbook);
	TestTrue(TEXT("Activated Idel profile loops"), Component->IsLooping());
	TestEqual(TEXT("Native-scale player profile keeps authored scale"),
	          Component->GetRelativeScale3D(),
	          FVector::OneVector);
	TestTrue(TEXT("Activated Flipbook can tick to advance frames"), Component->PrimaryComponentTick.bCanEverTick);
	TestTrue(TEXT("Activated Flipbook is playing"), Component->IsPlaying());
	TestTrue(TEXT("Unassigned gameplay states resolve to the default Flipbook"),
	         Component->SetAnimationState(EReEcho2DAnimationState::Death) &&
	             Component->GetFlipbook() == PlayerFlipbook);
	TestTrue(TEXT("Default fallback remains looping after a state change"), Component->IsLooping());
	TestTrue(TEXT("Moon Staff attack state resolves to s Flipbook"),
	         Component->SetAnimationState(EReEcho2DAnimationState::Attack, false) &&
	             Component->GetFlipbook() == StaffAttackFlipbook);
	TestFalse(TEXT("Moon Staff attack Flipbook is one-shot"), Component->IsLooping());

	FReEcho2DAnimationProfile MissingProfile;
	TestEqual(TEXT("Missing Flipbook fails safely"),
	          Component->ActivateProfile(MissingProfile),
	          EReEcho2DAnimationActivationResult::MissingFlipbook);
	TestFalse(TEXT("Failed activation leaves animation disabled"), Component->IsAnimationActive());
	TestFalse(TEXT("Disabled animation is not playing"), Component->IsPlaying());
	return true;
}

#endif
