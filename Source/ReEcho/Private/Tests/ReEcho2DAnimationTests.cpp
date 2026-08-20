#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/SceneComponent.h"
#include "Graybox/ReEchoEchoActor.h"
#include "Graybox/ReEchoEnemyActor.h"
#include "PaperFlipbook.h"
#include "PaperSprite.h"
#include "PhysicsEngine/BodySetup.h"
#include "Presentation/Animation2D/ReEcho2DAnimationComponent.h"
#include "Presentation/Animation2D/ReEcho2DAnimationTags.h"
#include "Presentation/Animation2D/ReEcho2DAnimationStateMachineAsset.h"
#include "Presentation/Animation2D/ReEcho2DCharacterPresentationProfile.h"
#include "Presentation/Animation2D/ReEcho2DFrameCollisionDriver.h"
#include "Presentation/Animation2D/ReEcho2DFrameCollisionTrack.h"
#include "Presentation/Animation2D/ReEcho2DPresentationCatalog.h"
#include "Presentation/Animation2D/ReEcho2DPresentationController.h"
#include "Presentation/Enemy/ReEchoEnemyGameplayClassRegistry.h"
#include "Presentation/Enemy/ReEchoEnemyPresentationComponent.h"
#include "Player/ReEchoPlayerPawn.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEcho2DFootpointAlignmentTest,
                                 "ReEcho.Presentation.Animation2D.FootpointAlignment",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEcho2DFootpointAlignmentTest::RunTest(const FString& Parameters)
{
	const FBoxSphereBounds Bounds(FVector(6.0f, -4.0f, 58.0f), FVector(22.0f, 18.0f, 42.0f), 64.0f);
	const FTransform RendererToRoot(FRotator::ZeroRotator, FVector(3.0f, 5.0f, 7.0f), FVector(-1.5f, 1.5f, 1.5f));
	const FTransform RootToMotion(FRotator(-55.0f, 90.0f, 0.0f), FVector(11.0f, -9.0f, 13.0f));
	const FVector AuthoredMotionLocation(17.0f, -8.0f, 21.0f);
	const FVector FootpointOffset(2.0f, 3.0f, 4.0f);
	const FVector Alignment = UReEcho2DAnimationComponent::CalculateFootAlignmentOffset(
	    Bounds, RendererToRoot, RootToMotion, AuthoredMotionLocation, FootpointOffset);
	const FVector LocalBottom(Bounds.Origin.X, Bounds.Origin.Y, Bounds.Origin.Z - Bounds.BoxExtent.Z);
	const FVector AlignedBottom = AuthoredMotionLocation + Alignment +
	                              RootToMotion.TransformPosition(RendererToRoot.TransformPosition(LocalBottom));
	TestTrue(TEXT("Mirrored, scaled and camera-tilted Flipbook bottom center meets authored footpoint"),
	         AlignedBottom.Equals(FootpointOffset, KINDA_SMALL_NUMBER));
	const float PresentationWidth =
	    UReEcho2DAnimationComponent::CalculateFlipbookPresentationWidth(Bounds, RendererToRoot, RootToMotion);
	const FVector LocalLeft(Bounds.Origin.X - Bounds.BoxExtent.X, Bounds.Origin.Y, Bounds.Origin.Z);
	const FVector LocalRight(Bounds.Origin.X + Bounds.BoxExtent.X, Bounds.Origin.Y, Bounds.Origin.Z);
	const float ExpectedWidth =
	    FVector::Distance(RootToMotion.TransformPosition(RendererToRoot.TransformPosition(LocalLeft)),
	                      RootToMotion.TransformPosition(RendererToRoot.TransformPosition(LocalRight)));
	TestEqual(TEXT("Ground shadow width source includes Flipbook mirror, scale and camera-facing transform"),
	          PresentationWidth,
	          ExpectedWidth);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEcho2DAnimationAssetProfilesTest,
                                 "ReEcho.Presentation.Animation2D.AssetProfiles",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEcho2DAnimationAssetProfilesTest::RunTest(const FString& Parameters)
{
	const FRotator TestCameraRotation(-55.0f, 0.0f, 0.0f);
	const FRotator SpriteRotation = UReEcho2DAnimationComponent::CalculateCameraFacingRotation(TestCameraRotation);
	const FRotationMatrix SpriteMatrix(SpriteRotation);
	const FRotationMatrix CameraMatrix(TestCameraRotation);
	TestTrue(TEXT("Animated sprite normal faces the active camera"),
	         FVector::DotProduct(SpriteMatrix.GetUnitAxis(EAxis::Y), -CameraMatrix.GetUnitAxis(EAxis::X)) > 0.999f);
	TestTrue(TEXT("Animated sprite vertical axis follows screen up"),
	         FVector::DotProduct(SpriteMatrix.GetUnitAxis(EAxis::Z), CameraMatrix.GetUnitAxis(EAxis::Z)) > 0.999f);

	UPaperFlipbook* WalkFlipbook =
	    LoadObject<UPaperFlipbook>(nullptr, TEXT("/Game/ReEcho/Art/Animation2D/Players/Spade/Flipbooks/Walk.Walk"));
	UPaperFlipbook* PlayerFlipbook = WalkFlipbook;
	UPaperFlipbook* GruntFlipbook = LoadObject<UPaperFlipbook>(
	    nullptr, TEXT("/Game/ReEcho/Art/Animation2D/Enemies/Grunt/Flipbooks/Default.Default"));
	UPaperFlipbook* StaffAttackFlipbook =
	    LoadObject<UPaperFlipbook>(nullptr, TEXT("/Game/ReEcho/Art/Animation2D/Players/Spade/Flipbooks/Attack.Attack"));
	UPaperFlipbook* RabbitFlipbook = LoadObject<UPaperFlipbook>(
	    nullptr, TEXT("/Game/ReEcho/Art/Animation2D/Enemies/Rabbit/Flipbooks/Default.Default"));
	UPaperFlipbook* RabbitAttackFlipbook =
	    LoadObject<UPaperFlipbook>(nullptr, TEXT("/Game/ReEcho/Art/Animation2D/Enemies/Rabbit/Flipbooks/NRA.NRA"));
	UPaperFlipbook* GoatWalkFlipbook =
	    LoadObject<UPaperFlipbook>(nullptr, TEXT("/Game/ReEcho/Art/Animation2D/Enemies/Goat/Flipbooks/Walk0.Walk0"));
	UPaperFlipbook* GoatAttackFlipbook = LoadObject<UPaperFlipbook>(
	    nullptr, TEXT("/Game/ReEcho/Art/Animation2D/Enemies/Goat/Flipbooks/Attack0.Attack0"));
	TestNotNull(TEXT("J_SPADE reusable renderer Flipbook is loadable"), PlayerFlipbook);
	TestNotNull(TEXT("J_SPADE walk Flipbook is loadable"), WalkFlipbook);
	TestNotNull(TEXT("Grunt default Flipbook is loadable"), GruntFlipbook);
	TestNotNull(TEXT("Moon Staff attack Flipbook is loadable"), StaffAttackFlipbook);
	TestNotNull(TEXT("Rabbit Doll Flipbook is loadable"), RabbitFlipbook);
	TestNotNull(TEXT("Rabbit Doll attack Flipbook is loadable"), RabbitAttackFlipbook);
	TestNotNull(TEXT("Goat Priest walk Flipbook is loadable"), GoatWalkFlipbook);
	TestNotNull(TEXT("Goat Priest attack Flipbook is loadable"), GoatAttackFlipbook);
	TestTrue(TEXT("J_SPADE Flipbook has non-empty render bounds"),
	         PlayerFlipbook && PlayerFlipbook->GetRenderBounds().BoxExtent.Z > 0.0f);
	TestTrue(TEXT("Grunt Flipbook has non-empty render bounds"),
	         GruntFlipbook && GruntFlipbook->GetRenderBounds().BoxExtent.Z > 0.0f);
	TestTrue(TEXT("Walk Flipbook uses authored EachFrame collision"),
	         WalkFlipbook && WalkFlipbook->GetCollisionSource() == EFlipbookCollisionMode::EachFrameCollision);
	TestTrue(TEXT("Attack Flipbook uses authored EachFrame collision"),
	         StaffAttackFlipbook &&
	             StaffAttackFlipbook->GetCollisionSource() == EFlipbookCollisionMode::EachFrameCollision);
	TestTrue(TEXT("Grunt Flipbook uses authored EachFrame collision"),
	         GruntFlipbook && GruntFlipbook->GetCollisionSource() == EFlipbookCollisionMode::EachFrameCollision);
	TestTrue(TEXT("Rabbit Flipbook uses authored EachFrame collision"),
	         RabbitFlipbook && RabbitFlipbook->GetCollisionSource() == EFlipbookCollisionMode::EachFrameCollision);
	TestTrue(TEXT("Goat walk Flipbook uses authored EachFrame collision"),
	         GoatWalkFlipbook && GoatWalkFlipbook->GetCollisionSource() == EFlipbookCollisionMode::EachFrameCollision);
	TestTrue(TEXT("Goat attack Flipbook uses authored EachFrame collision"),
	         GoatAttackFlipbook &&
	             GoatAttackFlipbook->GetCollisionSource() == EFlipbookCollisionMode::EachFrameCollision);
	auto TestEveryKeyFrameHasCollision = [this](const TCHAR* Label, const UPaperFlipbook* Flipbook)
	{
		bool bEveryFrameHasCollision = Flipbook && Flipbook->GetNumKeyFrames() > 0;
		if (Flipbook)
		{
			for (int32 Index = 0; Index < Flipbook->GetNumKeyFrames(); ++Index)
			{
				const UPaperSprite* Sprite = Flipbook->GetKeyFrameChecked(Index).Sprite;
				bEveryFrameHasCollision &=
				    Sprite && Sprite->BodySetup && Sprite->BodySetup->AggGeom.GetElementCount() > 0;
			}
		}
		TestTrue(Label, bEveryFrameHasCollision);
	};
	TestEveryKeyFrameHasCollision(TEXT("Walk has collision geometry on every key frame"), WalkFlipbook);
	TestEveryKeyFrameHasCollision(TEXT("Attack has collision geometry on every key frame"), StaffAttackFlipbook);
	TestEveryKeyFrameHasCollision(TEXT("Grunt has collision geometry on every key frame"), GruntFlipbook);
	TestEveryKeyFrameHasCollision(TEXT("Rabbit has collision geometry on every key frame"), RabbitFlipbook);
	TestEveryKeyFrameHasCollision(TEXT("Goat walk has collision geometry on every key frame"), GoatWalkFlipbook);
	TestEveryKeyFrameHasCollision(TEXT("Goat attack has collision geometry on every key frame"), GoatAttackFlipbook);
	UReEcho2DPresentationCatalog* AuthoredCatalog = LoadObject<UReEcho2DPresentationCatalog>(
	    nullptr, TEXT("/Game/ReEcho/Animation2D/DA_PresentationCatalog.DA_PresentationCatalog"));
	UReEchoEnemyGameplayClassRegistry* GameplayClassRegistry = LoadObject<UReEchoEnemyGameplayClassRegistry>(
	    nullptr,
	    TEXT("/Game/ReEcho/Gameplay/CharacterPrefabs/DA_EnemyGameplayClassRegistry.DA_EnemyGameplayClassRegistry"));
	UReEcho2DCharacterPresentationProfile* AuthoredGrunt = LoadObject<UReEcho2DCharacterPresentationProfile>(
	    nullptr, TEXT("/Game/ReEcho/Animation2D/DA_Enemy_Grunt.DA_Enemy_Grunt"));
	UReEcho2DCharacterPresentationProfile* AuthoredRabbit = LoadObject<UReEcho2DCharacterPresentationProfile>(
	    nullptr, TEXT("/Game/ReEcho/Animation2D/DA_Enemy_RabbitDoll.DA_Enemy_RabbitDoll"));
	UReEcho2DCharacterPresentationProfile* AuthoredGoat = LoadObject<UReEcho2DCharacterPresentationProfile>(
	    nullptr, TEXT("/Game/ReEcho/Animation2D/DA_Enemy_GoatPriest.DA_Enemy_GoatPriest"));
	UReEcho2DCharacterPresentationProfile* AuthoredFox = LoadObject<UReEcho2DCharacterPresentationProfile>(
	    nullptr, TEXT("/Game/ReEcho/Animation2D/DA_Enemy_Fox.DA_Enemy_Fox"));
	TestNotNull(TEXT("Cook-visible presentation catalog is loadable"), AuthoredCatalog);
	TestNotNull(TEXT("Gameplay-owned enemy class registry is loadable"), GameplayClassRegistry);
	const UReEcho2DCharacterPresentationProfile* AuthoredSpade =
	    AuthoredCatalog ? AuthoredCatalog->ResolveProfile(TEXT("J_SPADE")) : nullptr;
	TestNotNull(TEXT("Authored catalog resolves J_SPADE AppearanceId"), AuthoredSpade);
	const TArray<FName> PlayerPresentationIds = {TEXT("J_SPADE"), TEXT("J_DIAMOND"), TEXT("J_CLOVER"), TEXT("J_HEART")};
	for (const FName PresentationId : PlayerPresentationIds)
	{
		TestNotNull(*FString::Printf(TEXT("Catalog resolves player profile %s"), *PresentationId.ToString()),
		            AuthoredCatalog ? AuthoredCatalog->ResolveProfile(PresentationId) : nullptr);
	}
	const TArray<FName> EnemyPresentationIds = {TEXT("Enemy.Grunt"),
	                                            TEXT("Enemy.Shield"),
	                                            TEXT("Enemy.Bomber"),
	                                            TEXT("Enemy.Slime"),
	                                            TEXT("Enemy.Rabbit"),
	                                            TEXT("Enemy.Fox"),
	                                            TEXT("Enemy.TimeGuard")};
	for (const FName PresentationId : EnemyPresentationIds)
	{
		TestNotNull(*FString::Printf(TEXT("Catalog resolves enemy profile %s"), *PresentationId.ToString()),
		            AuthoredCatalog ? AuthoredCatalog->ResolveProfile(PresentationId) : nullptr);
		TestTrue(*FString::Printf(TEXT("Gameplay registry resolves enemy Blueprint %s"), *PresentationId.ToString()),
		         GameplayClassRegistry && GameplayClassRegistry->ResolveGameplayClass(PresentationId));
	}
	TestNull(TEXT("Removed J_CAT profile is absent from the production catalog"),
	         AuthoredCatalog ? AuthoredCatalog->ResolveProfile(TEXT("J_CAT")) : nullptr);
	UClass* PlayerGameplayClass = LoadClass<AReEchoPlayerPawn>(
	    nullptr, TEXT("/Game/ReEcho/Gameplay/CharacterPrefabs/BP_PlayerGameplay.BP_PlayerGameplay_C"));
	UClass* GruntGameplayClass = LoadClass<AReEchoEnemyActor>(
	    nullptr, TEXT("/Game/ReEcho/Gameplay/CharacterPrefabs/BP_EnemyGameplay_Grunt.BP_EnemyGameplay_Grunt_C"));
	TestTrue(TEXT("Player Gameplay Blueprint owns the runtime character scene"),
	         PlayerGameplayClass && PlayerGameplayClass->IsChildOf(AReEchoPlayerPawn::StaticClass()));
	TestTrue(TEXT("Enemy Gameplay Blueprint owns the runtime character scene"),
	         GruntGameplayClass && GruntGameplayClass->IsChildOf(AReEchoEnemyActor::StaticClass()));
	auto TestGameplaySceneTree = [this](const TCHAR* Label, UClass* GameplayClass, const bool bStableEnemyGround)
	{
		AActor* DefaultActor = GameplayClass ? Cast<AActor>(GameplayClass->GetDefaultObject()) : nullptr;
		USceneComponent* Collision = DefaultActor ? DefaultActor->GetRootComponent() : nullptr;
		USceneComponent* Presentation =
		    DefaultActor ? Cast<USceneComponent>(DefaultActor->GetDefaultSubobjectByName(TEXT("PresentationRoot")))
		                 : nullptr;
		USceneComponent* FootRoot =
		    DefaultActor ? Cast<USceneComponent>(DefaultActor->GetDefaultSubobjectByName(TEXT("FootRoot"))) : nullptr;
		USceneComponent* MotionRoot =
		    DefaultActor
		        ? Cast<USceneComponent>(DefaultActor->GetDefaultSubobjectByName(TEXT("PresentationMotionRoot")))
		        : nullptr;
		USceneComponent* FlipbookRoot =
		    DefaultActor ? Cast<USceneComponent>(DefaultActor->GetDefaultSubobjectByName(TEXT("FlipbookRoot")))
		                 : nullptr;
		USceneComponent* GroundRoot =
		    DefaultActor ? Cast<USceneComponent>(DefaultActor->GetDefaultSubobjectByName(TEXT("GroundRoot"))) : nullptr;
		USceneComponent* EffectsRoot =
		    DefaultActor ? Cast<USceneComponent>(DefaultActor->GetDefaultSubobjectByName(TEXT("EffectsRoot")))
		                 : nullptr;
		USceneComponent* AttackVfxRoot =
		    DefaultActor ? Cast<USceneComponent>(DefaultActor->GetDefaultSubobjectByName(TEXT("AttackVfxRoot")))
		                 : nullptr;
		USceneComponent* HurtVfxRoot =
		    DefaultActor ? Cast<USceneComponent>(DefaultActor->GetDefaultSubobjectByName(TEXT("HurtVfxRoot")))
		                 : nullptr;
		USceneComponent* Renderer =
		    DefaultActor ? Cast<USceneComponent>(DefaultActor->GetDefaultSubobjectByName(TEXT("FlipbookRenderer")))
		                 : nullptr;
		TestTrue(Label,
		         Collision && Presentation && FootRoot && MotionRoot && FlipbookRoot && GroundRoot && EffectsRoot &&
		             AttackVfxRoot && HurtVfxRoot && Renderer && Presentation->GetAttachParent() == Collision &&
		             FootRoot->GetAttachParent() == Presentation && MotionRoot->GetAttachParent() == FootRoot &&
		             FlipbookRoot->GetAttachParent() == MotionRoot &&
		             GroundRoot->GetAttachParent() == (bStableEnemyGround ? FootRoot : MotionRoot) &&
		             EffectsRoot->GetAttachParent() == MotionRoot && AttackVfxRoot->GetAttachParent() == EffectsRoot &&
		             HurtVfxRoot->GetAttachParent() == EffectsRoot && Renderer->GetAttachParent() == FlipbookRoot &&
		             !FlipbookRoot->IsUsingAbsoluteRotation() && !GroundRoot->IsUsingAbsoluteRotation() &&
		             !Renderer->GetRelativeRotation().ContainsNaN() && !Renderer->GetRelativeLocation().ContainsNaN() &&
		             !Renderer->GetRelativeScale3D().ContainsNaN() &&
		             Renderer->GetRelativeScale3D().GetAbsMin() > UE_SMALL_NUMBER && !Renderer->bHiddenInGame);
	};
	TestGameplaySceneTree(
	    TEXT("Player Blueprint exposes collision, Flipbook, ground and effects roots"), PlayerGameplayClass, true);
	TestGameplaySceneTree(
	    TEXT("Enemy Blueprint exposes collision, Flipbook, ground and effects roots"), GruntGameplayClass, true);
	AReEchoEchoActor* EchoDefault = GetMutableDefault<AReEchoEchoActor>();
	USceneComponent* EchoRoot = EchoDefault ? EchoDefault->GetRootComponent() : nullptr;
	USceneComponent* EchoEffectsRoot =
	    EchoDefault ? Cast<USceneComponent>(EchoDefault->GetDefaultSubobjectByName(TEXT("EffectsRoot"))) : nullptr;
	USceneComponent* EchoAttackVfxRoot =
	    EchoDefault ? Cast<USceneComponent>(EchoDefault->GetDefaultSubobjectByName(TEXT("AttackVfxRoot"))) : nullptr;
	USceneComponent* EchoHurtVfxRoot =
	    EchoDefault ? Cast<USceneComponent>(EchoDefault->GetDefaultSubobjectByName(TEXT("HurtVfxRoot"))) : nullptr;
	TestTrue(TEXT("Echo exposes separate attack and hurt VFX roots"),
	         EchoRoot && EchoEffectsRoot && EchoAttackVfxRoot && EchoHurtVfxRoot &&
	             EchoEffectsRoot->GetAttachParent() == EchoRoot &&
	             EchoAttackVfxRoot->GetAttachParent() == EchoEffectsRoot &&
	             EchoHurtVfxRoot->GetAttachParent() == EchoEffectsRoot);
	TestTrue(TEXT("Authored Spade default set owns looping Move"),
	         AuthoredSpade && AuthoredSpade->ResolveClip(NAME_None, ReEcho2DAnimationTags::Move) &&
	             AuthoredSpade->ResolveClip(NAME_None, ReEcho2DAnimationTags::Move)->Flipbook == WalkFlipbook &&
	             AuthoredSpade->ResolveClip(NAME_None, ReEcho2DAnimationTags::Move)->bLooping);
	TestTrue(TEXT("Authored Spade MoonStaff set owns one-shot BasicAttack"),
	         AuthoredSpade && AuthoredSpade->ResolveClip(TEXT("MoonStaff"), ReEcho2DAnimationTags::Attack_Basic) &&
	             AuthoredSpade->ResolveClip(TEXT("MoonStaff"), ReEcho2DAnimationTags::Attack_Basic)->Flipbook ==
	                 StaffAttackFlipbook &&
	             !AuthoredSpade->ResolveClip(TEXT("MoonStaff"), ReEcho2DAnimationTags::Attack_Basic)->bLooping);
	TestTrue(TEXT("Authored Grunt profile owns its looping Idle clip"),
	         AuthoredGrunt && AuthoredGrunt->ResolveClip(NAME_None, ReEcho2DAnimationTags::Idle) &&
	             AuthoredGrunt->ResolveClip(NAME_None, ReEcho2DAnimationTags::Idle)->Flipbook == GruntFlipbook &&
	             AuthoredGrunt->ResolveClip(NAME_None, ReEcho2DAnimationTags::Idle)->bLooping);
	auto TestEnemyProfile = [this](const TCHAR* Label,
	                               const UReEcho2DCharacterPresentationProfile* Profile,
	                               const UPaperFlipbook* ExpectedFlipbook)
	{
		const FReEcho2DAnimationClip* IdleClip =
		    Profile ? Profile->ResolveClip(NAME_None, ReEcho2DAnimationTags::Idle) : nullptr;
		const FReEcho2DAnimationClip* MoveClip =
		    Profile ? Profile->ResolveClip(NAME_None, ReEcho2DAnimationTags::Move) : nullptr;
		const FReEcho2DAnimationClip* AttackClip =
		    Profile ? Profile->ResolveClip(NAME_None, ReEcho2DAnimationTags::Attack_Basic) : nullptr;
		const FReEcho2DAnimationClip* HitClip =
		    Profile ? Profile->ResolveClip(NAME_None, ReEcho2DAnimationTags::Hit) : nullptr;
		TestTrue(Label,
		         Profile && IdleClip && MoveClip && AttackClip && HitClip && IdleClip->Flipbook == ExpectedFlipbook &&
		             MoveClip->Flipbook == ExpectedFlipbook && AttackClip->Flipbook == ExpectedFlipbook &&
		             HitClip->Flipbook == ExpectedFlipbook && IdleClip->bLooping && MoveClip->bLooping &&
		             !AttackClip->bLooping && !HitClip->bLooping);
	};
	TestEnemyProfile(
	    TEXT("Grunt exposes four semantic slots through its single authored Flipbook"), AuthoredGrunt, GruntFlipbook);
	const FReEcho2DAnimationClip* RabbitIdleClip =
	    AuthoredRabbit ? AuthoredRabbit->ResolveClip(NAME_None, ReEcho2DAnimationTags::Idle) : nullptr;
	const FReEcho2DAnimationClip* RabbitMoveClip =
	    AuthoredRabbit ? AuthoredRabbit->ResolveClip(NAME_None, ReEcho2DAnimationTags::Move) : nullptr;
	const FReEcho2DAnimationClip* RabbitAttackClip =
	    AuthoredRabbit ? AuthoredRabbit->ResolveClip(NAME_None, ReEcho2DAnimationTags::Attack_Basic) : nullptr;
	const FReEcho2DAnimationClip* RabbitHitClip =
	    AuthoredRabbit ? AuthoredRabbit->ResolveClip(NAME_None, ReEcho2DAnimationTags::Hit) : nullptr;
	TestTrue(TEXT("Rabbit uses Default for locomotion and NRA for attack"),
	         AuthoredRabbit && RabbitIdleClip && RabbitMoveClip && RabbitAttackClip && RabbitHitClip &&
	             RabbitIdleClip->Flipbook == RabbitFlipbook && RabbitMoveClip->Flipbook == RabbitFlipbook &&
	             RabbitAttackClip->Flipbook == RabbitAttackFlipbook && RabbitHitClip->Flipbook == RabbitFlipbook &&
	             RabbitIdleClip->bLooping && RabbitMoveClip->bLooping && !RabbitAttackClip->bLooping &&
	             RabbitAttackClip->bRestartOnRequest && !RabbitHitClip->bLooping);
	TestTrue(TEXT("Rabbit semantic clips all use shared Profile.WorldHeight normalization"),
	         RabbitIdleClip && RabbitMoveClip && RabbitAttackClip && RabbitHitClip &&
	             !RabbitIdleClip->bUseNativeScale && !RabbitMoveClip->bUseNativeScale &&
	             !RabbitAttackClip->bUseNativeScale && !RabbitHitClip->bUseNativeScale);
	const FReEcho2DAnimationClip* GoatIdleClip =
	    AuthoredGoat ? AuthoredGoat->ResolveClip(NAME_None, ReEcho2DAnimationTags::Idle) : nullptr;
	const FReEcho2DAnimationClip* GoatAttackClip =
	    AuthoredGoat ? AuthoredGoat->ResolveClip(NAME_None, ReEcho2DAnimationTags::Attack_Basic) : nullptr;
	TestTrue(TEXT("Goat owns looping Walk and one-shot Attack"),
	         AuthoredGoat && GoatIdleClip && GoatAttackClip && GoatIdleClip->Flipbook == GoatWalkFlipbook &&
	             GoatIdleClip->bLooping && GoatAttackClip->Flipbook == GoatAttackFlipbook &&
	             !GoatAttackClip->bLooping && GoatAttackClip->bRestartOnRequest);
	const FReEcho2DAnimationClip* FoxWalkClip =
	    AuthoredFox ? AuthoredFox->ResolveClip(NAME_None, ReEcho2DAnimationTags::Idle) : nullptr;
	const FReEcho2DAnimationClip* FoxAttackClip =
	    AuthoredFox ? AuthoredFox->ResolveClip(NAME_None, ReEcho2DAnimationTags::Attack_Basic) : nullptr;
	TestTrue(TEXT("Fox owns looping Walk and one-shot Attack"),
	         AuthoredFox && FoxWalkClip && FoxAttackClip && FoxWalkClip->Flipbook && FoxWalkClip->bLooping &&
	             FoxAttackClip->Flipbook && !FoxAttackClip->bLooping && FoxAttackClip->bRestartOnRequest);
	TestTrue(TEXT("Fox Walk uses authored EachFrame collision"),
	         FoxWalkClip && FoxWalkClip->Flipbook &&
	             FoxWalkClip->Flipbook->GetCollisionSource() == EFlipbookCollisionMode::EachFrameCollision);
	TestTrue(TEXT("Fox Attack uses authored EachFrame collision"),
	         FoxAttackClip && FoxAttackClip->Flipbook &&
	             FoxAttackClip->Flipbook->GetCollisionSource() == EFlipbookCollisionMode::EachFrameCollision);
	TestEveryKeyFrameHasCollision(TEXT("Fox Walk has collision geometry on every key frame"),
	                              FoxWalkClip ? FoxWalkClip->Flipbook : nullptr);
	TestEveryKeyFrameHasCollision(TEXT("Fox Attack has collision geometry on every key frame"),
	                              FoxAttackClip ? FoxAttackClip->Flipbook : nullptr);

	UReEcho2DAnimationComponent* Component = NewObject<UReEcho2DAnimationComponent>();
	TestNotNull(TEXT("Animation renderer owns an alpha-capable sprite material override"), Component->GetMaterial(0));
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
	TestEqual(TEXT("Idle collision follows the authored per-frame query policy"),
	          Component->GetCollisionEnabled(),
	          PlayerFlipbook->GetCollisionSource() == EFlipbookCollisionMode::EachFrameCollision
	              ? ECollisionEnabled::QueryOnly
	              : ECollisionEnabled::NoCollision);
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
	TestTrue(TEXT("Walk EachFrame collision is enabled as Query-only"),
	         Component->IsUsingEachFrameCollision() &&
	             Component->GetCollisionEnabled() == ECollisionEnabled::QueryOnly);
	TestEqual(TEXT("EachFrame collision never blocks Pawn movement"),
	          Component->GetCollisionResponseToChannel(ECC_Pawn),
	          ECollisionResponse::ECR_Overlap);
	TestFalse(TEXT("EachFrame collision does not emit automatic overlap events"),
	          Component->GetGenerateOverlapEvents());
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

	UReEcho2DCharacterPresentationProfile* PresentationProfile = NewObject<UReEcho2DCharacterPresentationProfile>();
	PresentationProfile->AppearanceId = TEXT("Appearance.Spade");
	FReEcho2DCompositeAnimationSet& DefaultSet = PresentationProfile->AnimationSets.AddDefaulted_GetRef();
	DefaultSet.Clips.Add(ReEcho2DAnimationTags::Idle, AuthoredClip);
	FReEcho2DCompositeAnimationSet& StaffSet = PresentationProfile->AnimationSets.AddDefaulted_GetRef();
	StaffSet.WeaponVisualSetId = TEXT("MoonStaff");
	FReEcho2DAnimationClip StaffMoveClip = AuthoredClip;
	StaffMoveClip.Flipbook = WalkFlipbook;
	StaffSet.Clips.Add(ReEcho2DAnimationTags::Move, StaffMoveClip);
	const FReEcho2DAnimationClip* ResolvedMove =
	    PresentationProfile->ResolveClip(TEXT("MoonStaff"), ReEcho2DAnimationTags::Move);
	TestTrue(TEXT("Exact weapon visual set resolves its authored semantic clip"),
	         ResolvedMove && ResolvedMove->Flipbook == WalkFlipbook);
	const FReEcho2DAnimationClip* ResolvedIdle =
	    PresentationProfile->ResolveClip(TEXT("MoonStaff"), ReEcho2DAnimationTags::Idle);
	TestTrue(TEXT("Missing weapon semantic falls back to the character default set"),
	         ResolvedIdle && ResolvedIdle->Flipbook == StaffAttackFlipbook);
	TestNull(TEXT("Unknown semantic returns no clip instead of guessing an asset"),
	         PresentationProfile->ResolveClip(TEXT("MoonStaff"), ReEcho2DAnimationTags::Death));

	UReEcho2DPresentationCatalog* Catalog = NewObject<UReEcho2DPresentationCatalog>();
	Catalog->CharacterProfiles.Add(PresentationProfile);
	TestTrue(TEXT("Catalog resolves profiles only through AppearanceId"),
	         Catalog->ResolveProfile(TEXT("Appearance.Spade")) == PresentationProfile);
	TestNull(TEXT("Catalog safely rejects an unknown AppearanceId"), Catalog->ResolveProfile(TEXT("J_SPADE")));

	// Match the authored Spade policy for controller behavior: static Idle, default Move, MoonStaff attack.
	PresentationProfile->AnimationSets[0].Clips.Remove(ReEcho2DAnimationTags::Idle);
	PresentationProfile->AnimationSets[0].Clips.Add(ReEcho2DAnimationTags::Move, StaffMoveClip);
	PresentationProfile->AnimationSets[1].Clips.Add(ReEcho2DAnimationTags::Attack_Basic, AuthoredClip);
	PresentationProfile->AnimationSets[1].Clips.Add(ReEcho2DAnimationTags::Hit, AuthoredClip);
	UReEcho2DAnimationStateMachineAsset* StateMachine = NewObject<UReEcho2DAnimationStateMachineAsset>();
	auto AddState = [StateMachine](const FGameplayTag Tag, const int32 Priority, const bool bLock, const bool bTerminal)
	{
		FReEcho2DAnimationStateDefinition& State = StateMachine->States.AddDefaulted_GetRef();
		State.StateTag = Tag;
		State.SemanticKey = Tag;
		State.InterruptPriority = Priority;
		State.bLockUntilPlaybackComplete = bLock;
		State.bTerminal = bTerminal;
	};
	AddState(ReEcho2DAnimationTags::Idle, 0, false, false);
	AddState(ReEcho2DAnimationTags::Move, 10, false, false);
	AddState(ReEcho2DAnimationTags::Attack_Basic, 40, true, false);
	AddState(ReEcho2DAnimationTags::Hit, 60, true, false);
	PresentationProfile->StateMachine = StateMachine;
	UReEcho2DAnimationComponent* ControlledRenderer = NewObject<UReEcho2DAnimationComponent>();
	UReEcho2DPresentationController* Controller = NewObject<UReEcho2DPresentationController>();
	ControlledRenderer->SetFlipbook(WalkFlipbook);
	Controller->Configure(ControlledRenderer, nullptr, NAME_None);
	TestTrue(TEXT("Missing Profile retains the Gameplay Blueprint fallback Flipbook"),
	         ControlledRenderer->IsAnimationActive() && ControlledRenderer->GetFlipbook() == WalkFlipbook &&
	             ControlledRenderer->IsVisible() && !ControlledRenderer->bHiddenInGame);
	Controller->Configure(ControlledRenderer, PresentationProfile, TEXT("MoonStaff"));
	PresentationProfile->WorldHeight = 100.0f;
	Controller->Configure(ControlledRenderer, PresentationProfile, TEXT("MoonStaff"));
	TestTrue(TEXT("Missing Idle clip retains the Gameplay Blueprint fallback Flipbook"),
	         ControlledRenderer->IsAnimationActive() && ControlledRenderer->GetFlipbook() == WalkFlipbook &&
	             ControlledRenderer->IsVisible() && !ControlledRenderer->bHiddenInGame);
	Controller->SetMoving(true);
	const float ControlledNativeHeight = ControlledRenderer->GetFlipbook()->GetRenderBounds().BoxExtent.Z * 2.0f;
	TestTrue(TEXT("Profile world height normalizes the active animation renderer"),
	         ControlledNativeHeight > 0.0f &&
	             FMath::IsNearlyEqual(ControlledRenderer->GetRelativeScale3D().Z, 100.0f / ControlledNativeHeight));
	TestTrue(TEXT("Move intent selects the authored weapon-set clip"),
	         ControlledRenderer->IsAnimationActive() && ControlledRenderer->GetFlipbook() == WalkFlipbook);
	TestTrue(TEXT("Authored basic action overrides Move"),
	         Controller->PlayAction(ReEcho2DAnimationTags::Attack_Basic) &&
	             ControlledRenderer->GetFlipbook() == StaffAttackFlipbook && !ControlledRenderer->IsLooping());
	TestTrue(TEXT("Higher-priority Hit interrupts a locked Attack state"),
	         Controller->PlayAction(ReEcho2DAnimationTags::Hit));
	TestFalse(TEXT("Lower-priority Attack cannot interrupt a locked Hit state"),
	          Controller->PlayAction(ReEcho2DAnimationTags::Attack_Basic));
	ControlledRenderer->Stop();
	Controller->UpdatePlaybackCompletion();
	TestTrue(TEXT("Completed one-shot returns to the current Move base state"),
	         Controller->GetActiveSemanticKey() == ReEcho2DAnimationTags::Move &&
	             ControlledRenderer->GetFlipbook() == WalkFlipbook);

	FReEcho2DCompositeAnimationSet& Phase2Set = PresentationProfile->AnimationSets.AddDefaulted_GetRef();
	Phase2Set.WeaponVisualSetId = TEXT("Phase2");
	FReEcho2DAnimationClip Phase2TransformClip = AuthoredClip;
	Phase2TransformClip.Flipbook = StaffAttackFlipbook;
	Phase2Set.Clips.Add(ReEcho2DAnimationTags::Transform_Phase2, Phase2TransformClip);
	FReEcho2DAnimationClip Phase2MoveClip = StaffMoveClip;
	Phase2MoveClip.Flipbook = WalkFlipbook;
	Phase2Set.Clips.Add(ReEcho2DAnimationTags::Move, Phase2MoveClip);
	TestTrue(TEXT("Animation-set transition resolves Transform from the target set before base-state playback"),
	         Controller->BeginAnimationSetTransition(TEXT("Phase2"), ReEcho2DAnimationTags::Transform_Phase2) &&
	             Controller->GetActiveSemanticKey() == ReEcho2DAnimationTags::Transform_Phase2 &&
	             ControlledRenderer->GetFlipbook() == StaffAttackFlipbook);
	Controller->CompleteAnimationSetTransition(TEXT("Phase2"));
	TestTrue(TEXT("Completing an animation-set transition enters the target set's current base state"),
	         Controller->GetActiveSemanticKey() == ReEcho2DAnimationTags::Move &&
	             ControlledRenderer->GetFlipbook() == WalkFlipbook);

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

	CollisionTrack->Frames.SetNum(WalkFlipbook->GetNumFrames());
	CollisionTrack->PixelsPerUnrealUnit = 2.0f;
	CollisionTrack->PivotPixels = FVector2D(10.0f, 20.0f);
	FReEcho2DCollisionPolygon BodyPolygon;
	BodyPolygon.Vertices = {FVector2D(10.0f, 20.0f), FVector2D(14.0f, 20.0f), FVector2D(14.0f, 24.0f)};
	FReEcho2DCollisionPolygon AttackPolygon;
	AttackPolygon.Vertices = {FVector2D(14.0f, 20.0f), FVector2D(18.0f, 20.0f), FVector2D(18.0f, 24.0f)};
	CollisionTrack->Frames[0].BodyHurtboxes.Add(BodyPolygon);
	CollisionTrack->Frames[0].WeaponAttackHitboxes.Add(AttackPolygon);
	CollisionTrack->Frames[0].bAttackActive = true;
	FReEcho2DAnimationClip CollisionClip;
	CollisionClip.Flipbook = WalkFlipbook;
	CollisionClip.CollisionTrack = CollisionTrack;
	CollisionClip.bUseNativeScale = true;
	Component->PlayClip(CollisionClip, true);
	Component->SetPlaybackPosition(0.0f, false);
	UReEcho2DFrameCollisionDriver* CollisionDriver = NewObject<UReEcho2DFrameCollisionDriver>();
	CollisionDriver->BindRenderer(Component);
	TestTrue(TEXT("Driver accepts a source-matched authored track"), CollisionDriver->HasValidTrack());
	TestEqual(TEXT("Driver follows the renderer key frame"), CollisionDriver->GetSnapshot().FrameIndex, 0);
	TestEqual(TEXT("Body Hurtboxes remain queryable without an attack instance"),
	          CollisionDriver->GetSnapshot().BodyHurtboxes.Num(),
	          1);
	TestFalse(TEXT("Authored active frame cannot open AttackHitboxes without a committed attack"),
	          CollisionDriver->GetSnapshot().bAttackActive);
	CollisionDriver->BeginAttackInstance(42);
	TestTrue(TEXT("Committed attack opens authored active-frame AttackHitboxes"),
	         CollisionDriver->GetSnapshot().bAttackActive && CollisionDriver->GetSnapshot().AttackInstanceId == 42 &&
	             CollisionDriver->GetSnapshot().WeaponAttackHitboxes.Num() == 1);
	TestEqual(TEXT("Pixel geometry is pivoted and converted to Unreal units"),
	          CollisionDriver->GetSnapshot().BodyHurtboxes[0].Vertices[1],
	          FVector2D(2.0f, 0.0f));
	TestTrue(TEXT("Body geometry supports Query-only local point tests"),
	         CollisionDriver->IsLocalPointInsideBody(FVector2D(1.5f, 0.5f)));
	TestTrue(TEXT("Attack geometry accepts only its committed attack instance"),
	         CollisionDriver->IsLocalPointInsideActiveAttack(FVector2D(3.0f, 0.5f), 42));
	TestFalse(TEXT("Attack geometry rejects stale attack instances"),
	          CollisionDriver->IsLocalPointInsideActiveAttack(FVector2D(3.0f, 0.5f), 41));
	Component->SetFacingSign(-1.0f);
	CollisionDriver->RefreshSnapshot();
	TestEqual(TEXT("Facing mirrors collision geometry around the authored pivot"),
	          CollisionDriver->GetSnapshot().BodyHurtboxes[0].Vertices[1],
	          FVector2D(-2.0f, 0.0f));
	CollisionDriver->EndAttackInstance(42);
	TestFalse(TEXT("Ending the matching attack instance closes AttackHitboxes"),
	          CollisionDriver->GetSnapshot().bAttackActive);
	CollisionClip.CollisionTrack = nullptr;
	Component->PlayClip(CollisionClip, true);
	CollisionDriver->RefreshSnapshot();
	TestFalse(TEXT("Missing track safely disables authored frame geometry"), CollisionDriver->HasValidTrack());
	TestTrue(TEXT("Missing-track fallback is diagnostic"),
	         CollisionDriver->GetFallbackReason().Contains(TEXT("no collision track")));

	FReEcho2DAnimationProfile MissingProfile;
	TestEqual(TEXT("Missing Flipbook fails safely"),
	          Component->ActivateProfile(MissingProfile),
	          EReEcho2DAnimationActivationResult::MissingFlipbook);
	TestFalse(TEXT("Failed activation leaves animation disabled"), Component->IsAnimationActive());
	TestEqual(TEXT("Disabled animation also disables Paper2D collision"),
	          Component->GetCollisionEnabled(),
	          ECollisionEnabled::NoCollision);
	TestFalse(TEXT("Disabled animation is not playing"), Component->IsPlaying());
	return true;
}

#endif
