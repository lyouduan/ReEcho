#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "PaperFlipbook.h"
#include "Presentation/Animation2D/ReEcho2DAnimationComponent.h"
#include "Presentation/Animation2D/ReEcho2DFrameCollisionTrack.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEcho2DAnimationAssetProfilesTest,
                                 "ReEcho.Presentation.Animation2D.AssetProfiles",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEcho2DAnimationAssetProfilesTest::RunTest(const FString& Parameters)
{
	UPaperFlipbook* PlayerFlipbook = LoadObject<UPaperFlipbook>(nullptr, TEXT("/Game/2DAnim/Flipbook/Idel.Idel"));
	UPaperFlipbook* WalkFlipbook = LoadObject<UPaperFlipbook>(nullptr, TEXT("/Game/2DAnim/Flipbook/walk.walk"));
	UPaperFlipbook* GruntFlipbook = LoadObject<UPaperFlipbook>(nullptr, TEXT("/Game/2DAnim/Flipbook/01_2.01_2"));
	UPaperFlipbook* StaffAttackFlipbook =
	    LoadObject<UPaperFlipbook>(nullptr, TEXT("/Game/2DAnim/Flipbook/attack.attack"));
	TestNotNull(TEXT("J_SPADE idle Flipbook is loadable"), PlayerFlipbook);
	TestNotNull(TEXT("J_SPADE walk Flipbook is loadable"), WalkFlipbook);
	TestNotNull(TEXT("Grunt default Flipbook is loadable"), GruntFlipbook);
	TestNotNull(TEXT("Moon Staff attack Flipbook is loadable"), StaffAttackFlipbook);
	TestTrue(TEXT("J_SPADE Flipbook has non-empty render bounds"),
	         PlayerFlipbook && PlayerFlipbook->GetRenderBounds().BoxExtent.Z > 0.0f);
	TestTrue(TEXT("Grunt Flipbook has non-empty render bounds"),
	         GruntFlipbook && GruntFlipbook->GetRenderBounds().BoxExtent.Z > 0.0f);

	UReEcho2DAnimationComponent* Component = NewObject<UReEcho2DAnimationComponent>();
	FReEcho2DAnimationProfile Profile;
	Profile.DefaultFlipbook = PlayerFlipbook;
	Profile.StateFlipbooks.Add(EReEcho2DAnimationState::Walk, WalkFlipbook);
	Profile.StateFlipbooks.Add(EReEcho2DAnimationState::Attack, StaffAttackFlipbook);
	Profile.WorldHeight = 224.0f;
	Profile.bUseNativeScale = true;
	TestEqual(TEXT("Valid profile activates"),
	          Component->ActivateProfile(Profile),
	          EReEcho2DAnimationActivationResult::Activated);
	TestTrue(TEXT("Activated component owns the requested Flipbook"),
	         Component->IsAnimationActive() && Component->GetFlipbook() == PlayerFlipbook);
	TestTrue(TEXT("Activated Idel profile loops"), Component->IsLooping());
	TestEqual(
	    TEXT("Native-scale player profile keeps authored scale"), Component->GetRelativeScale3D(), FVector::OneVector);
	TestTrue(TEXT("Activated Flipbook can tick to advance frames"), Component->PrimaryComponentTick.bCanEverTick);
	TestTrue(TEXT("Activated Flipbook is playing"), Component->IsPlaying());
	TestTrue(TEXT("Unassigned gameplay states resolve to the default Flipbook"),
	         Component->SetAnimationState(EReEcho2DAnimationState::Death) &&
	             Component->GetFlipbook() == PlayerFlipbook);
	TestTrue(TEXT("Default fallback remains looping after a state change"), Component->IsLooping());
	TestEqual(TEXT("Stationary player resolves to Idle"),
	          ReEchoResolve2DAnimationState(false, false),
	          EReEcho2DAnimationState::Idle);
	TestEqual(TEXT("Moving player resolves to Walk"),
	          ReEchoResolve2DAnimationState(true, false),
	          EReEcho2DAnimationState::Walk);
	TestEqual(
	    TEXT("Attack overrides movement"), ReEchoResolve2DAnimationState(true, true), EReEcho2DAnimationState::Attack);
	TestTrue(TEXT("Walk state resolves to looping walk Flipbook"),
	         Component->SetAnimationState(EReEcho2DAnimationState::Walk) && Component->GetFlipbook() == WalkFlipbook &&
	             Component->IsLooping());
	TestTrue(TEXT("Moon Staff attack state resolves to attack Flipbook"),
	         Component->SetAnimationState(EReEcho2DAnimationState::Attack, false) &&
	             Component->GetFlipbook() == StaffAttackFlipbook);
	TestFalse(TEXT("Moon Staff attack Flipbook is one-shot"), Component->IsLooping());
	Component->SetPlaybackPosition(Component->GetFlipbookLength(), false);
	TestTrue(TEXT("Repeated attacks restart one-shot playback"),
	         Component->SetAnimationState(EReEcho2DAnimationState::Attack, false, true) &&
	             FMath::IsNearlyZero(Component->GetPlaybackPosition()));

	FReEcho2DAnimationClip AuthoredClip;
	AuthoredClip.Flipbook = StaffAttackFlipbook;
	AuthoredClip.bLooping = false;
	AuthoredClip.PlayRate = 1.25f;
	AuthoredClip.bUseNativeScale = true;
	TestTrue(TEXT("Pure renderer accepts an authored one-shot clip"), Component->PlayClip(AuthoredClip, true));
	TestFalse(TEXT("Pure renderer preserves authored one-shot policy"), Component->IsLooping());
	TestEqual(TEXT("Pure renderer preserves authored play rate"), Component->GetPlayRate(), 1.25f);

	UReEcho2DFrameCollisionTrack* CollisionTrack = NewObject<UReEcho2DFrameCollisionTrack>();
	CollisionTrack->SourceFlipbook = WalkFlipbook;
	CollisionTrack->SourceRevision = TEXT("test-walk-revision");
	CollisionTrack->Frames.SetNum(WalkFlipbook->GetNumFrames());
	FString CollisionError;
	TestTrue(TEXT("Matching frame collision track validates"),
	         CollisionTrack->ValidateForFlipbook(WalkFlipbook, CollisionError));
	CollisionTrack->Frames.RemoveAt(CollisionTrack->Frames.Num() - 1);
	TestFalse(TEXT("Frame-count mismatch is rejected"),
	          CollisionTrack->ValidateForFlipbook(WalkFlipbook, CollisionError));
	TestTrue(TEXT("Frame-count rejection is diagnostic"), CollisionError.Contains(TEXT("frame count")));

	FReEcho2DAnimationProfile MissingProfile;
	TestEqual(TEXT("Missing Flipbook fails safely"),
	          Component->ActivateProfile(MissingProfile),
	          EReEcho2DAnimationActivationResult::MissingFlipbook);
	TestFalse(TEXT("Failed activation leaves animation disabled"), Component->IsAnimationActive());
	TestFalse(TEXT("Disabled animation is not playing"), Component->IsPlaying());
	return true;
}

#endif
