#if WITH_DEV_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "Data/ReEchoCsvDataRegistry.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Graybox/ReEchoEchoActor.h"
#include "Misc/AutomationTest.h"
#include "PaperFlipbook.h"
#include "Player/ReEchoPlayerPawn.h"
#include "Presentation/Animation2D/ReEcho2DAnimationComponent.h"
#include "Presentation/Animation2D/ReEcho2DAnimationTags.h"
#include "Presentation/Animation2D/ReEcho2DCharacterPresentationProfile.h"
#include "Presentation/Animation2D/ReEcho2DPresentationCatalog.h"
#include "ReEchoGameMode.h"
#include "UObject/UObjectIterator.h"
#include "Weapons/ReEchoWeaponActor.h"

namespace
{
bool TransformsMatch(const FTransform& Left, const FTransform& Right, const float Tolerance = KINDA_SMALL_NUMBER)
{
	return Left.GetTranslation().Equals(Right.GetTranslation(), Tolerance) &&
	       Left.GetRotation().Equals(Right.GetRotation(), Tolerance) &&
	       Left.GetScale3D().Equals(Right.GetScale3D(), Tolerance);
}

const USceneComponent* FindSceneComponent(const AActor& Actor, const FName Name)
{
	return Cast<USceneComponent>(const_cast<AActor&>(Actor).GetDefaultSubobjectByName(Name));
}

struct FSpatialParityWorldFixture
{
	UWorld* World = nullptr;

	FSpatialParityWorldFixture()
	{
		const FName WorldName = MakeUniqueObjectName(nullptr, UWorld::StaticClass(), TEXT("EchoSpatialParityWorld"));
		FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
		World = UWorld::CreateWorld(EWorldType::Game, false, WorldName, GetTransientPackage());
		World->AddToRoot();
		WorldContext.SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
		World->BeginPlay();
	}

	~FSpatialParityWorldFixture()
	{
		if (World)
		{
			World->DestroyWorld(true);
			GEngine->DestroyWorldContext(World);
			World->RemoveFromRoot();
		}
	}
};
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoDeferredBornRevealTest,
                                 "ReEcho.Presentation.EchoAppearance.DeferredBornReveal",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoDeferredBornRevealTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FSpatialParityWorldFixture Fixture;
	AReEchoEchoActor* Echo = Fixture.World->SpawnActor<AReEchoEchoActor>();
	if (!TestNotNull(TEXT("Echo spawns for deferred reveal"), Echo))
	{
		return false;
	}
	Echo->QueueBornVfxForTests();
	Echo->PrepareDeferredBornReveal(0.0f);
	TestTrue(TEXT("Deferred reveal state is prepared"), Echo->IsDeferredBornRevealPreparedForTests());
	TestTrue(TEXT("Deferred reveal hides the Echo actor"), Echo->IsHidden());
	TestTrue(TEXT("Position priming preserves the pending birth VFX"), Echo->IsBornVfxPendingForTests());
	Echo->CompleteDeferredBornReveal();
	TestFalse(TEXT("Fail-open completion clears deferred reveal state"),
	          Echo->IsDeferredBornRevealPreparedForTests());
	TestFalse(TEXT("Fail-open completion restores Echo visibility"), Echo->IsHidden());
	TestFalse(TEXT("Fail-open completion prevents duplicate birth VFX"), Echo->IsBornVfxPendingForTests());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEchoAppearanceMappingTest,
                                 "ReEcho.Presentation.EchoAppearance.CharacterMappings",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEchoAppearanceMappingTest::RunTest(const FString& Parameters)
{
	const FReEchoCsvLoadResult LoadResult = FReEchoCsvDataRegistry::LoadAndPublishDefault();
	if (!TestTrue(TEXT("Default CSV data loads"), LoadResult.bSuccess))
	{
		AddError(LoadResult.FormatIssues());
		return false;
	}
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();

	struct FExpectedMapping
	{
		FName CharacterId;
		const TCHAR* WalkPath;
		const TCHAR* AttackPath;
		const TCHAR* RangedAttackPath;
	};

	const FExpectedMapping Mappings[] = {
	    {TEXT("J_HEART"),
	     TEXT("/Game/ReEcho/Art/Animation2D/Echos/Heart/Flipbooks/walk.walk"),
	     TEXT("/Game/ReEcho/Art/Animation2D/Echos/Heart/Flipbooks/Attack.Attack"),
	     TEXT("/Game/ReEcho/Art/Animation2D/Echos/Heart/Flipbooks/Attack_Arrow.Attack_Arrow")},
	    {TEXT("J_SPADE"),
	     TEXT("/Game/ReEcho/Art/Animation2D/Echos/Spade/Flipbooks/walk.walk"),
	     TEXT("/Game/ReEcho/Art/Animation2D/Echos/Spade/Flipbooks/Attack.Attack"),
	     TEXT("/Game/ReEcho/Art/Animation2D/Echos/Spade/Flipbooks/Attack_Arrow.Attack_Arrow")},
	    {TEXT("J_CLOVER"),
	     TEXT("/Game/ReEcho/Art/Animation2D/Echos/Clover/Flipbooks/walk.walk"),
	     TEXT("/Game/ReEcho/Art/Animation2D/Echos/Clover/Flipbooks/Attack.Attack"),
	     TEXT("/Game/ReEcho/Art/Animation2D/Echos/Clover/Flipbooks/Attack_Arrow.Attack_Arrow")},
	    {TEXT("J_DIAMOND"),
	     TEXT("/Game/ReEcho/Art/Animation2D/Echos/Diamond/Flipbooks/Walk.Walk"),
	     TEXT("/Game/ReEcho/Art/Animation2D/Echos/Diamond/Flipbooks/Attack.Attack"),
	     TEXT("/Game/ReEcho/Art/Animation2D/Echos/Diamond/Flipbooks/Attack_Arrow.Attack_Arrow")},
	};
	UReEcho2DPresentationCatalog* Catalog = LoadObject<UReEcho2DPresentationCatalog>(
	    nullptr,
	    TEXT("/Game/ReEcho/DataAsset/Character/Catalogs/DA_EchoPresentationCatalog.DA_EchoPresentationCatalog"));
	if (!TestNotNull(TEXT("Echo presentation catalog loads"), Catalog))
	{
		return false;
	}
	UReEcho2DPresentationCatalog* PlayerCatalog = LoadObject<UReEcho2DPresentationCatalog>(
	    nullptr,
	    TEXT("/Game/ReEcho/DataAsset/Character/Catalogs/DA_CharacterPresentationCatalog."
	         "DA_CharacterPresentationCatalog"));
	if (!TestNotNull(TEXT("Player presentation catalog loads"), PlayerCatalog))
	{
		return false;
	}
	AReEchoEchoActor* Echo = NewObject<AReEchoEchoActor>(GetTransientPackage());
	UReEcho2DAnimationComponent* Animation = Echo->FindComponentByClass<UReEcho2DAnimationComponent>();
	if (!TestNotNull(TEXT("Echo has a Paper2D animation renderer"), Animation))
	{
		return false;
	}
	for (const FExpectedMapping& Mapping : Mappings)
	{
		UPaperFlipbook* Walk = LoadObject<UPaperFlipbook>(nullptr, Mapping.WalkPath);
		UPaperFlipbook* Attack = LoadObject<UPaperFlipbook>(nullptr, Mapping.AttackPath);
		UPaperFlipbook* RangedAttack = LoadObject<UPaperFlipbook>(nullptr, Mapping.RangedAttackPath);
		const UReEcho2DCharacterPresentationProfile* Profile = Catalog->ResolveProfile(Mapping.CharacterId);
		TestNotNull(*FString::Printf(TEXT("%s profile resolves"), *Mapping.CharacterId.ToString()), Profile);
		TestNotNull(*FString::Printf(TEXT("%s walk loads"), *Mapping.CharacterId.ToString()), Walk);
		TestNotNull(*FString::Printf(TEXT("%s attack loads"), *Mapping.CharacterId.ToString()), Attack);
		TestNotNull(*FString::Printf(TEXT("%s ranged attack loads"), *Mapping.CharacterId.ToString()), RangedAttack);
		const FReEcho2DAnimationClip* MeleeClip =
		    Profile ? Profile->ResolveClip(TEXT("CrescentBlade"), ReEcho2DAnimationTags::Attack_Basic) : nullptr;
		const FReEcho2DAnimationClip* RangedClip =
		    Profile ? Profile->ResolveClip(TEXT("Bow"), ReEcho2DAnimationTags::Attack_Basic) : nullptr;
		TestEqual(TEXT("Melee maps to Attack"), MeleeClip ? MeleeClip->Flipbook.Get() : nullptr, Attack);
		TestEqual(TEXT("Bow maps to Attack_Arrow"), RangedClip ? RangedClip->Flipbook.Get() : nullptr, RangedAttack);
		TestTrue(TEXT("Echo configures"), Echo->ConfigureEchoAppearance(Mapping.CharacterId));
		TestEqual(TEXT("Echo uses its own walk"), Animation->GetFlipbook(), Walk);

		const FReEchoCsvCharacterRow* Character =
		    Snapshot.IsValid() ? Snapshot->FindCharacter(Mapping.CharacterId) : nullptr;
		const UReEcho2DCharacterPresentationProfile* PlayerSpatialProfile =
		    Character ? PlayerCatalog->ResolveProfile(Character->AppearanceId) : nullptr;
		const UReEcho2DCharacterPresentationProfile* ComposedProfile = nullptr;
		ForEachObjectWithOuter(
		    Echo,
		    [&ComposedProfile, Profile, Walk](UObject* Object)
		    {
			    const UReEcho2DCharacterPresentationProfile* Candidate =
			        Cast<UReEcho2DCharacterPresentationProfile>(Object);
			    const FReEcho2DAnimationClip* CandidateMove =
			        Candidate ? Candidate->ResolveClip(NAME_None, ReEcho2DAnimationTags::Move) : nullptr;
			    if (Candidate && Candidate != Profile && CandidateMove && CandidateMove->Flipbook == Walk)
			    {
				    ComposedProfile = Candidate;
			    }
		    },
		    EGetObjectsFlags::None);
		TestNotNull(TEXT("Player spatial profile resolves through CSV AppearanceId"), PlayerSpatialProfile);
		TestNotNull(TEXT("Echo composes animation and player spatial policy"), ComposedProfile);
		if (PlayerSpatialProfile && ComposedProfile)
		{
			TestEqual(
			    TEXT("Echo uses player WorldHeight"), ComposedProfile->WorldHeight, PlayerSpatialProfile->WorldHeight);
			TestEqual(TEXT("Echo uses player auto footpoint policy"),
			          ComposedProfile->bAutoAlignFootpoint,
			          PlayerSpatialProfile->bAutoAlignFootpoint);
			TestTrue(
			    TEXT("Echo uses player footpoint offset"),
			    ComposedProfile->FootpointOffset.Equals(PlayerSpatialProfile->FootpointOffset, KINDA_SMALL_NUMBER));
		}
	}
	TestFalse(TEXT("Unknown character id is rejected"), Echo->ConfigureEchoAppearance(TEXT("J_UNKNOWN")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEchoSpatialParityTest,
                                 "ReEcho.Presentation.EchoAppearance.SpatialParity",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEchoSpatialParityTest::RunTest(const FString& Parameters)
{
	const FReEchoCsvLoadResult LoadResult = FReEchoCsvDataRegistry::LoadAndPublishDefault();
	if (!TestTrue(TEXT("Default CSV data loads"), LoadResult.bSuccess))
	{
		AddError(LoadResult.FormatIssues());
		return false;
	}
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
	UClass* PlayerGameplayClass = LoadClass<AReEchoPlayerPawn>(
	    nullptr, TEXT("/Game/ReEcho/Gameplay/CharacterPrefabs/BP_PlayerGameplay.BP_PlayerGameplay_C"));
	AReEchoPlayerPawn* PlayerDefaults =
	    PlayerGameplayClass ? Cast<AReEchoPlayerPawn>(PlayerGameplayClass->GetDefaultObject()) : nullptr;
	if (!TestNotNull(TEXT("Player Gameplay Blueprint CDO loads"), PlayerDefaults))
	{
		return false;
	}
	AReEchoPlayerPawn* PlayerInstance = NewObject<AReEchoPlayerPawn>(GetTransientPackage(), PlayerGameplayClass);
	if (!TestNotNull(TEXT("Player Gameplay Blueprint instance creates"), PlayerInstance))
	{
		return false;
	}
	PlayerInstance->SetActorScale3D(PlayerDefaults->GetActorScale3D());
	USceneComponent* PlayerFlipbookRoot =
	    Cast<USceneComponent>(PlayerInstance->GetDefaultSubobjectByName(TEXT("FlipbookRoot")));
	if (!TestNotNull(TEXT("Live Player FlipbookRoot exists"), PlayerFlipbookRoot))
	{
		return false;
	}
	const FRotator LiveAuthoredFlipbookRotation(13.0f, 27.0f, -9.0f);
	PlayerFlipbookRoot->SetRelativeRotation(LiveAuthoredFlipbookRotation);
	AReEchoEchoActor* Echo = NewObject<AReEchoEchoActor>(GetTransientPackage());
	Echo->ApplyPlayerSpatialAuthoringForTests(*PlayerInstance);
	TestTrue(TEXT("Echo configures Spade"), Echo->ConfigureEchoAppearance(TEXT("J_SPADE")));
	const FReEchoEchoSpatialPresentationSnapshot AuthoredEchoSpatial = Echo->CaptureSpatialPresentationForTests();
	Echo->RefreshSpatialPresentationForTests();
	const FReEchoEchoSpatialPresentationSnapshot EchoSpatial = Echo->CaptureSpatialPresentationForTests();

	auto TestAuthoredTransform =
	    [this, PlayerDefaults](const TCHAR* Label, const FTransform& EchoTransform, const FName ComponentName)
	{
		const USceneComponent* PlayerComponent = FindSceneComponent(*PlayerDefaults, ComponentName);
		TestNotNull(*FString::Printf(TEXT("Player %s exists"), Label), PlayerComponent);
		if (PlayerComponent)
		{
			TestTrue(*FString::Printf(TEXT("Player/Echo %s relative transform matches"), Label),
			         TransformsMatch(EchoTransform, PlayerComponent->GetRelativeTransform()));
		}
	};
	TestAuthoredTransform(TEXT("PresentationRoot"), EchoSpatial.PresentationRootTransform, TEXT("PresentationRoot"));
	TestAuthoredTransform(TEXT("FootRoot"), EchoSpatial.FootRootTransform, TEXT("FootRoot"));
	TestAuthoredTransform(
	    TEXT("PresentationMotionRoot"), AuthoredEchoSpatial.MotionRootTransform, TEXT("PresentationMotionRoot"));
	TestTrue(TEXT("Echo copies the live Player FlipbookRoot relative transform"),
	         TransformsMatch(EchoSpatial.FlipbookRootTransform, PlayerFlipbookRoot->GetRelativeTransform()));
	TestAuthoredTransform(TEXT("EffectsRoot"), EchoSpatial.EffectsRootTransform, TEXT("EffectsRoot"));
	TestAuthoredTransform(TEXT("AttackVfxRoot"), EchoSpatial.AttackVfxRootTransform, TEXT("AttackVfxRoot"));
	TestAuthoredTransform(TEXT("HurtVfxRoot"), EchoSpatial.HurtVfxRootTransform, TEXT("HurtVfxRoot"));
	TestAuthoredTransform(TEXT("GroundRoot"), AuthoredEchoSpatial.GroundRootTransform, TEXT("GroundRoot"));
	TestAuthoredTransform(TEXT("GroundShadow"), AuthoredEchoSpatial.GroundShadowTransform, TEXT("GroundShadow"));
	TestTrue(
	    TEXT("Attack VFX inherits a valid Effects scale"),
	    (EchoSpatial.AttackVfxRootTransform.GetScale3D() * EchoSpatial.EffectsRootTransform.GetScale3D()).GetAbsMin() >
	        UE_SMALL_NUMBER);
	TestTrue(
	    TEXT("Hurt VFX inherits a valid Effects scale"),
	    (EchoSpatial.HurtVfxRootTransform.GetScale3D() * EchoSpatial.EffectsRootTransform.GetScale3D()).GetAbsMin() >
	        UE_SMALL_NUMBER);

	const UStaticMeshComponent* Shadow =
	    Cast<UStaticMeshComponent>(Echo->GetDefaultSubobjectByName(TEXT("GroundShadow")));
	const float ShadowNativeWidth =
	    Shadow && Shadow->GetStaticMesh() ? Shadow->GetStaticMesh()->GetBounds().BoxExtent.Y * 2.0f : 0.0f;
	const FName CharacterIds[] = {TEXT("J_HEART"), TEXT("J_SPADE"), TEXT("J_CLOVER"), TEXT("J_DIAMOND")};
	for (const FName CharacterId : CharacterIds)
	{
		TestTrue(*FString::Printf(TEXT("%s Echo configures for spatial snapshot"), *CharacterId.ToString()),
		         Echo->ConfigureEchoAppearance(CharacterId));
		Echo->RefreshSpatialPresentationForTests();
		const FReEchoEchoSpatialPresentationSnapshot CharacterSpatial = Echo->CaptureSpatialPresentationForTests();
		const UReEcho2DCharacterPresentationProfile* CharacterProfile = Echo->GetSpatialProfileForTests();
		TestNotNull(*FString::Printf(TEXT("%s exposes the Player spatial profile"), *CharacterId.ToString()),
		            CharacterProfile);
		TestTrue(*FString::Printf(TEXT("%s normalized height uses Player WorldHeight"), *CharacterId.ToString()),
		         CharacterProfile && FMath::IsNearlyEqual(CharacterSpatial.NormalizedCharacterHeight,
		                                                  CharacterProfile->WorldHeight *
		                                                      FMath::Abs(PlayerDefaults->GetActorScale3D().Z),
		                                                  KINDA_SMALL_NUMBER));
		const UPaperFlipbook* CharacterFlipbook =
		    Cast<UReEcho2DAnimationComponent>(Echo->GetDefaultSubobjectByName(TEXT("FlipbookRenderer")))->GetFlipbook();
		const float RenderedHeight = CharacterFlipbook
		                                 ? CharacterFlipbook->GetRenderBounds().BoxExtent.Z * 2.0f *
		                                       FMath::Abs(CharacterSpatial.RendererTransform.GetScale3D().Z) *
		                                       FMath::Abs(CharacterSpatial.FlipbookRootTransform.GetScale3D().Z) *
		                                       FMath::Abs(CharacterSpatial.ActorTransform.GetScale3D().Z)
		                                 : 0.0f;
		TestTrue(*FString::Printf(TEXT("%s rendered bounds height matches Player height"), *CharacterId.ToString()),
		         FMath::IsNearlyEqual(RenderedHeight, CharacterSpatial.NormalizedCharacterHeight, 0.01f));
		TestTrue(*FString::Printf(TEXT("%s post-bounds width is valid"), *CharacterId.ToString()),
		         CharacterSpatial.PresentedCharacterWidth > 0.0f);
		const FReEcho2DAnimationClip* PlayerMoveClip =
		    CharacterProfile ? CharacterProfile->ResolveClip(NAME_None, ReEcho2DAnimationTags::Move) : nullptr;
		float ExpectedPlayerShadowWidth = 0.0f;
		if (PlayerMoveClip && PlayerMoveClip->Flipbook)
		{
			const FBoxSphereBounds PlayerBounds = PlayerMoveClip->Flipbook->GetRenderBounds();
			const float NativePlayerHeight = PlayerBounds.BoxExtent.Z * 2.0f;
			if (NativePlayerHeight > UE_SMALL_NUMBER)
			{
				const FTransform PlayerRendererTransform(FQuat::Identity,
				                                         PlayerMoveClip->LocalOffset,
				                                         FVector(CharacterProfile->WorldHeight / NativePlayerHeight));
				ExpectedPlayerShadowWidth =
				    UReEcho2DAnimationComponent::CalculateFlipbookPresentationWidth(
				        PlayerBounds, PlayerRendererTransform, CharacterSpatial.FlipbookRootTransform) *
				    FMath::Abs(CharacterSpatial.ActorTransform.GetScale3D().Z);
			}
		}
		TestTrue(*FString::Printf(TEXT("%s shadow width source matches Player profile"), *CharacterId.ToString()),
		         FMath::IsNearlyEqual(CharacterSpatial.ShadowReferenceWidth, ExpectedPlayerShadowWidth, 0.01f));
		TestFalse(*FString::Printf(TEXT("%s GroundRoot remains finite"), *CharacterId.ToString()),
		          CharacterSpatial.GroundRootTransform.GetTranslation().ContainsNaN());
		TestTrue(*FString::Printf(TEXT("%s final shadow width follows Player reference"), *CharacterId.ToString()),
		         ShadowNativeWidth > UE_SMALL_NUMBER &&
		             FMath::IsNearlyEqual(CharacterSpatial.GroundShadowTransform.GetScale3D().Y * ShadowNativeWidth,
		                                  CharacterSpatial.ShadowReferenceWidth /
		                                      FMath::Abs(CharacterSpatial.ActorTransform.GetScale3D().Z),
		                                  0.01f));
	}
	TestTrue(TEXT("Echo restores Spade for representative weapon layout"),
	         Echo->ConfigureEchoAppearance(TEXT("J_SPADE")));
	const UReEcho2DCharacterPresentationProfile* SpatialProfile = Echo->GetSpatialProfileForTests();
	UReEcho2DAnimationComponent* EchoRenderer =
	    Cast<UReEcho2DAnimationComponent>(Echo->GetDefaultSubobjectByName(TEXT("FlipbookRenderer")));
	EchoRenderer->SetFacingSign(1.0f);
	const FReEchoEchoSpatialPresentationSnapshot RightFacing = Echo->CaptureSpatialPresentationForTests();
	EchoRenderer->SetFacingSign(-1.0f);
	const FReEchoEchoSpatialPresentationSnapshot LeftFacing = Echo->CaptureSpatialPresentationForTests();
	TestTrue(TEXT("Left/right mirror preserves renderer location"),
	         RightFacing.RendererTransform.GetTranslation().Equals(LeftFacing.RendererTransform.GetTranslation(),
	                                                               KINDA_SMALL_NUMBER));
	TestTrue(TEXT("Left/right mirror preserves absolute renderer scale"),
	         RightFacing.RendererTransform.GetScale3D().GetAbs().Equals(
	             LeftFacing.RendererTransform.GetScale3D().GetAbs(), KINDA_SMALL_NUMBER));
	TestTrue(TEXT("Left/right mirror preserves final presentation width"),
	         FMath::IsNearlyEqual(
	             RightFacing.PresentedCharacterWidth, LeftFacing.PresentedCharacterWidth, KINDA_SMALL_NUMBER));
	TestTrue(TEXT("Left/right facing does not alter the authored FlipbookRoot rotation"),
	         RightFacing.FlipbookRootTransform.GetRotation().Equals(LeftFacing.FlipbookRootTransform.GetRotation(),
	                                                                KINDA_SMALL_NUMBER) &&
	             RightFacing.FlipbookRootTransform.GetRotation().Equals(LiveAuthoredFlipbookRotation.Quaternion(),
	                                                                    KINDA_SMALL_NUMBER));

	FSpatialParityWorldFixture Fixture;
	AReEchoPlayerPawn* PlayerOwner = Fixture.World->SpawnActor<AReEchoPlayerPawn>();
	AReEchoEchoActor* EchoOwner = Fixture.World->SpawnActor<AReEchoEchoActor>();
	PlayerOwner->SetActorScale3D(PlayerDefaults->GetActorScale3D());
	EchoOwner->SetActorScale3D(PlayerDefaults->GetActorScale3D());
	const FName WeaponIds[] = {TEXT("W_J_01"), TEXT("W_J_08")};
	const FName VisualComponentNames[] = {TEXT("SwordSprite"), TEXT("BowSprite")};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(WeaponIds); ++Index)
	{
		AReEchoWeaponActor* PlayerWeapon = Fixture.World->SpawnActor<AReEchoWeaponActor>();
		AReEchoWeaponActor* EchoWeapon = Fixture.World->SpawnActor<AReEchoWeaponActor>();
		PlayerWeapon->SetOwner(PlayerOwner);
		EchoWeapon->SetOwner(EchoOwner);
		PlayerWeapon->InitializeWeapon(nullptr, Snapshot);
		EchoWeapon->InitializeWeapon(nullptr, Snapshot);
		TestTrue(TEXT("Player representative weapon selects"), PlayerWeapon->SelectWeaponById(WeaponIds[Index]));
		TestTrue(TEXT("Echo representative weapon selects"), EchoWeapon->SelectWeaponById(WeaponIds[Index]));
		PlayerWeapon->ConfigureHeldPresentation(SpatialProfile);
		EchoWeapon->ConfigureHeldPresentation(Echo->GetSpatialProfileForTests());
		const USceneComponent* PlayerVisual = FindSceneComponent(*PlayerWeapon, VisualComponentNames[Index]);
		const USceneComponent* EchoVisual = FindSceneComponent(*EchoWeapon, VisualComponentNames[Index]);
		TestTrue(TEXT("Player/Echo held weapon anchor matches"),
		         PlayerWeapon->GetActorLocation().Equals(EchoWeapon->GetActorLocation(), KINDA_SMALL_NUMBER));
		TestTrue(TEXT("Player/Echo held weapon final visual transform matches"),
		         PlayerVisual && EchoVisual &&
		             TransformsMatch(PlayerVisual->GetRelativeTransform(), EchoVisual->GetRelativeTransform()));
	}

	UClass* EchoGameplayClass = LoadClass<AReEchoEchoActor>(
	    nullptr, TEXT("/Game/ReEcho/Gameplay/CharacterPrefabs/BP_EchoGameplay.BP_EchoGameplay_C"));
	TestTrue(TEXT("Echo Gameplay Blueprint has the native Echo parent"),
	         EchoGameplayClass && EchoGameplayClass->IsChildOf(AReEchoEchoActor::StaticClass()));
	AReEchoEchoActor* EchoGameplayDefaults =
	    EchoGameplayClass ? Cast<AReEchoEchoActor>(EchoGameplayClass->GetDefaultObject()) : nullptr;
	const FName EditableComponentNames[] = {TEXT("PresentationRoot"),
	                                        TEXT("FlipbookRoot"),
	                                        TEXT("GroundShadow"),
	                                        TEXT("AttackVfxRoot"),
	                                        TEXT("HurtVfxRoot")};
	for (const FName ComponentName : EditableComponentNames)
	{
		const USceneComponent* Component =
		    EchoGameplayDefaults ? FindSceneComponent(*EchoGameplayDefaults, ComponentName) : nullptr;
		TestTrue(*FString::Printf(TEXT("Echo Blueprint exposes editable inherited %s"), *ComponentName.ToString()),
		         Component && Component->bEditableWhenInherited);
	}

	AReEchoGameMode* GameMode = Fixture.World->SpawnActor<AReEchoGameMode>();
	TestEqual(TEXT("GameMode resolves the cooked Echo Gameplay Blueprint class"),
	          GameMode ? GameMode->ResolveEchoClassForTests().Get() : nullptr,
	          EchoGameplayClass);
	AReEchoEchoActor* BlueprintEcho = GameMode ? GameMode->SpawnEchoActorForTests() : nullptr;
	TestTrue(TEXT("Unified Echo spawn entry creates the Gameplay Blueprint class"),
	         BlueprintEcho && BlueprintEcho->GetClass() == EchoGameplayClass);
	if (GameMode)
	{
		GameMode->SetEchoGameplayClassForTests(nullptr);
	}
	TestEqual(TEXT("Missing Echo Blueprint class resolves the native fallback"),
	          GameMode ? GameMode->ResolveEchoClassForTests().Get() : nullptr,
	          AReEchoEchoActor::StaticClass());
	AReEchoEchoActor* NativeEcho = GameMode ? GameMode->SpawnEchoActorForTests() : nullptr;
	TestTrue(TEXT("Unified Echo spawn fallback creates the native class"),
	         NativeEcho && NativeEcho->GetClass() == AReEchoEchoActor::StaticClass());
	return true;
}

#endif
