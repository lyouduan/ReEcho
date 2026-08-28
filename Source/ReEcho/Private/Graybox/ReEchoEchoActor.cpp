#include "Graybox/ReEchoEchoActor.h"

#include "Combat/ReEchoCombatantComponent.h"
#include "Combat/ReEchoCombatContracts.h"
#include "Combat/ReEchoCombatAudioAdapterComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/BillboardComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "Data/ReEchoCsvDataRegistry.h"
#include "Kismet/GameplayStatics.h"
#include "Graybox/ReEchoEnemyActor.h"
#include "Graybox/ReEchoBillboardDebug.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Math/RotationMatrix.h"
#include "PaperFlipbook.h"
#include "Presentation/Animation2D/ReEcho2DAnimationComponent.h"
#include "Presentation/Animation2D/ReEcho2DAnimationTags.h"
#include "Presentation/Animation2D/ReEcho2DCharacterPresentationProfile.h"
#include "Presentation/Animation2D/ReEcho2DFrameCollisionDriver.h"
#include "Presentation/Animation2D/ReEcho2DPresentationCatalog.h"
#include "Presentation/Animation2D/ReEcho2DPresentationController.h"
#include "Presentation/Scene/ReEcho2DSceneLightingComponent.h"
#include "Recording/ReEchoPlaybackComponent.h"
#include "Run/ReEchoRunSubsystem.h"
#include "Player/ReEchoPlayerPawn.h"
#include "Presentation/VFX/ReEchoCombatVfxComponent.h"
#include "ReEcho.h"
#include "ReEchoAudioEvents.h"
#include "Weapons/ReEchoWeaponActor.h"
#include "UObject/ConstructorHelpers.h"

AReEchoEchoActor::AReEchoEchoActor()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	PresentationRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PresentationRoot"));
	PresentationRoot->SetupAttachment(RootComponent);
	PresentationRoot->bEditableWhenInherited = true;
	FootRoot = CreateDefaultSubobject<USceneComponent>(TEXT("FootRoot"));
	FootRoot->SetupAttachment(PresentationRoot);
	PresentationMotionRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PresentationMotionRoot"));
	PresentationMotionRoot->SetupAttachment(FootRoot);
	FlipbookRoot = CreateDefaultSubobject<USceneComponent>(TEXT("FlipbookRoot"));
	FlipbookRoot->SetupAttachment(PresentationMotionRoot);
	FlipbookRoot->bEditableWhenInherited = true;
	FlipbookRoot->SetRelativeRotation(
	    UReEcho2DAnimationComponent::CalculateCameraFacingRotation(FRotator(-45.0f, 0.0f, 0.0f)));
	GroundRoot = CreateDefaultSubobject<USceneComponent>(TEXT("GroundRoot"));
	GroundRoot->SetupAttachment(FootRoot);
	EffectsRoot = CreateDefaultSubobject<USceneComponent>(TEXT("EffectsRoot"));
	EffectsRoot->SetupAttachment(PresentationMotionRoot);
	AttackVfxRoot = CreateDefaultSubobject<USceneComponent>(TEXT("AttackVfxRoot"));
	AttackVfxRoot->SetupAttachment(EffectsRoot);
	AttackVfxRoot->bEditableWhenInherited = true;
	HurtVfxRoot = CreateDefaultSubobject<USceneComponent>(TEXT("HurtVfxRoot"));
	HurtVfxRoot->SetupAttachment(EffectsRoot);
	HurtVfxRoot->bEditableWhenInherited = true;
	EchoAuraVfxRoot = CreateDefaultSubobject<USceneComponent>(TEXT("EchoAuraVfxRoot"));
	EchoAuraVfxRoot->SetupAttachment(EffectsRoot);
	EchoAuraVfxRoot->bEditableWhenInherited = true;
	GroundShadow = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GroundShadow"));
	GroundShadow->SetupAttachment(GroundRoot);
	GroundShadow->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GroundShadow->SetCastShadow(false);
	GroundShadow->SetTranslucentSortPriority(-10);
	GroundShadow->bEditableWhenInherited = true;
	GroundShadow->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")));
	GroundShadow->SetRelativeLocation(FVector::ZeroVector);
	GroundShadow->SetRelativeScale3D(FVector(0.512f, 0.5376f, 1.0f));
	if (UMaterialInterface* ShadowMaterial = LoadObject<UMaterialInterface>(
	        nullptr, TEXT("/Game/ReEcho/Materials/M_GroundShadow_Procedural.M_GroundShadow_Procedural")))
	{
		GroundShadow->SetMaterial(0, ShadowMaterial);
	}
	CharacterSprite = CreateDefaultSubobject<UBillboardComponent>(TEXT("CharacterSprite"));
	CharacterSprite->SetupAttachment(FlipbookRoot);
	CharacterSprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CharacterSprite->SetHiddenInGame(false);
	CharacterSprite->SetVisibility(true);
	CharacterSprite->SetRelativeLocation(FVector::ZeroVector);
	CharacterSprite->bIsScreenSizeScaled = false;
	EchoAnimation = CreateDefaultSubobject<UReEcho2DAnimationComponent>(TEXT("FlipbookRenderer"));
	EchoAnimation->SetupAttachment(FlipbookRoot);
	PresentationController = CreateDefaultSubobject<UReEcho2DPresentationController>(TEXT("PresentationController"));
	FrameCollisionDriver = CreateDefaultSubobject<UReEcho2DFrameCollisionDriver>(TEXT("FrameCollisionDriver"));
	PresentationController->BindCollisionDriver(FrameCollisionDriver);
	SceneLighting = CreateDefaultSubobject<UReEcho2DSceneLightingComponent>(TEXT("SceneLighting"));
	SceneLighting->Configure(EchoAnimation, GroundShadow);
	static ConstructorHelpers::FObjectFinder<UTexture2D> HeartTextureFinder(
	    TEXT("/Game/ReEcho/Textures/Characters/NewCast/Player_Heart.Player_Heart"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> SpadeTextureFinder(
	    TEXT("/Game/ReEcho/Textures/Characters/NewCast/Player_Cat.Player_Cat"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> CloverTextureFinder(
	    TEXT("/Game/ReEcho/Textures/Characters/NewCast/Player_Clover.Player_Clover"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> DiamondTextureFinder(
	    TEXT("/Game/ReEcho/Textures/Characters/NewCast/Player_Diamond.Player_Diamond"));
	EchoTextures.Add(TEXT("J_HEART"), HeartTextureFinder.Object);
	EchoTextures.Add(TEXT("J_SPADE"), SpadeTextureFinder.Object);
	EchoTextures.Add(TEXT("J_CLOVER"), CloverTextureFinder.Object);
	EchoTextures.Add(TEXT("J_DIAMOND"), DiamondTextureFinder.Object);
	static ConstructorHelpers::FObjectFinder<UReEcho2DPresentationCatalog> EchoCatalogFinder(
	    TEXT("/Game/ReEcho/DataAsset/Character/Catalogs/DA_EchoPresentationCatalog.DA_EchoPresentationCatalog"));
	EchoPresentationCatalog = EchoCatalogFinder.Object;
	static ConstructorHelpers::FObjectFinder<UReEcho2DPresentationCatalog> PlayerCatalogFinder(
	    TEXT("/Game/ReEcho/DataAsset/Character/Catalogs/DA_CharacterPresentationCatalog."
	         "DA_CharacterPresentationCatalog"));
	PlayerPresentationCatalog = PlayerCatalogFinder.Object;
	ConfigureEchoAppearance(TEXT("J_SPADE"));
	BaseVisualLocation = FlipbookRoot->GetRelativeLocation();
	BaseVisualScale = FlipbookRoot->GetRelativeScale3D();
	AuthoredMotionLocation = PresentationMotionRoot->GetRelativeLocation();
	BaseEffectsLocation = EffectsRoot->GetRelativeLocation();
	BaseEffectsScale = EffectsRoot->GetRelativeScale3D();
	AuthoredGroundRootLocation = GroundRoot->GetRelativeLocation();
	AuthoredGroundShadowScale = GroundShadow->GetRelativeScale3D();

	Playback = CreateDefaultSubobject<UReEchoPlaybackComponent>(TEXT("Playback"));
	Combatant = CreateDefaultSubobject<UReEchoCombatantComponent>(TEXT("Combatant"));
	CombatEvents = CreateDefaultSubobject<UReEchoCombatEventsComponent>(TEXT("CombatEvents"));
	CombatAudioAdapter = CreateDefaultSubobject<UReEchoCombatAudioAdapterComponent>(TEXT("CombatAudioAdapter"));
	CombatVfx = CreateDefaultSubobject<UReEchoCombatVfxComponent>(TEXT("CombatVfx"));
	CombatVfx->ConfigureAttachmentRoots(AttackVfxRoot, HurtVfxRoot);
	CombatVfx->ConfigureEchoAuraRoot(EchoAuraVfxRoot);
	CombatAudioAdapter->ConfigureRouting(EReEchoCombatAudioSource::Echo, FReEchoAudioEvents::EchoAttack, NAME_None);
}

bool AReEchoEchoActor::InitializeEcho(const FReEchoRecording& Recording,
                                      const float Efficiency,
                                      TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot)
{
	if (!Snapshot.IsValid() || Recording.BuildSnapshot.WeaponDomainRevision != Snapshot->WeaponDomainRevision)
	{
		UE_LOG(LogReEcho,
		       Error,
		       TEXT("Cannot initialize echo: weapon domain revision mismatch saved=%s current=%s"),
		       *Recording.BuildSnapshot.WeaponDomainRevision,
		       Snapshot.IsValid() ? *Snapshot->WeaponDomainRevision : TEXT("<none>"));
		return false;
	}
	if (const AReEchoPlayerPawn* Player = Cast<AReEchoPlayerPawn>(UGameplayStatics::GetPlayerPawn(this, 0)))
	{
		ApplyPlayerSpatialAuthoring(*Player);
	}
	ConfigureEchoAppearance(Recording.BuildSnapshot.CharacterId);
	DamageEfficiency = Efficiency;
	Playback->LoadRecording(Recording);

	// Plan 64: 世界地面轨迹已迁移至右上角小地图。此处仅缓存平面 XY 轨迹点供小地图绘制。
	RecordedPath.Reset();
	RecordedPath.Reserve(Recording.Positions.Num());
	for (const FReEchoPositionSample& Sample : Recording.Positions)
	{
		RecordedPath.Add(FVector2D(Sample.Position.X, Sample.Position.Y));
	}

	Weapon = GetWorld()->SpawnActor<AReEchoWeaponActor>();
	if (Weapon)
	{
		Weapon->SetOwner(this);
		Weapon->AttachToActor(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		Weapon->SetActorRelativeLocation(FVector::ZeroVector);
		CombatVfx->ConfigureWeaponAttackVfxRoot(Weapon->GetWeaponAttackVfxRoot());
		Weapon->InitializeWeapon(&Recording.BuildSnapshot, Snapshot);
		if (!Weapon->SelectWeaponById(Recording.BuildSnapshot.WeaponId))
		{
			UE_LOG(LogReEcho,
			       Error,
			       TEXT("Cannot initialize echo with WeaponId '%s'"),
			       *Recording.BuildSnapshot.WeaponId.ToString());
			Weapon->Destroy();
			Weapon = nullptr;
			return false;
		}
		if (PresentationController)
		{
			PresentationController->SetWeaponVisualSetId(Weapon->GetEquippedWeaponVisualKey());
		}
		Weapon->ConfigureHeldPresentation(ActivePresentationProfile);
		FReEchoStatBlock EchoStats = Weapon->GetBuildSnapshot().Stats;
		EchoStats.PhysicalAttack = FMath::Max(1.0f, EchoStats.PhysicalAttack * DamageEfficiency);
		EchoStats.ElementalAttack = FMath::Max(1.0f, EchoStats.ElementalAttack * DamageEfficiency);
		Combatant->InitializeFromStats(EchoStats, true);
	}
	if (Weapon && !bAudioLifecycleStarted)
	{
		CombatAudioAdapter->PostConfiguredEvent(FReEchoAudioEvents::EchoSpawn, GetActorLocation());
		bAudioLifecycleStarted = true;
	}
	// Echo creation is shared by normal encounter entry, save restoration and GM stage jumps.
	// Do not arm the one-shot birth presentation here: the Stage 01 -> 02 transition owns
	// the only automatic reveal and explicitly arms it through PrepareDeferredBornReveal().
	bBornVfxPending = false;
	return Weapon != nullptr;
}

bool AReEchoEchoActor::ConfigureEchoAppearance(const FName CharacterId)
{
	const TObjectPtr<UTexture2D>* TextureEntry = EchoTextures.Find(CharacterId);
	if (!TextureEntry || !TextureEntry->Get())
	{
		return false;
	}

	ConfiguredCharacterId = CharacterId;
	RefreshPresentationProfile();
	const bool bHasAnimation = EchoAnimation && EchoAnimation->IsAnimationActive();
	CharacterSprite->SetVisibility(!bHasAnimation);
	CharacterSprite->SetHiddenInGame(bHasAnimation);
	if (!EchoAnimation || !EchoAnimation->IsAnimationActive())
	{
		UTexture2D* Texture = TextureEntry->Get();
		constexpr float CharacterWorldHeight = 224.0f;
		CharacterSprite->SetSprite(Texture);
		CharacterSprite->SetRelativeScale3D(FVector(CharacterWorldHeight / FMath::Max(1, Texture->GetSizeY())));
		CharacterSprite->SetVisibility(true);
		CharacterSprite->SetHiddenInGame(false);
	}
	return true;
}

UTexture2D* AReEchoEchoActor::GetMinimapIconTexture() const
{
	return ActivePresentationProfile ? ActivePresentationProfile->MinimapIcon.Get() : nullptr;
}

void AReEchoEchoActor::RefreshPresentationProfile()
{
	UReEcho2DCharacterPresentationProfile* EchoProfile =
	    EchoPresentationCatalog ? EchoPresentationCatalog->ResolveProfile(ConfiguredCharacterId) : nullptr;
	UReEcho2DCharacterPresentationProfile* PlayerSpatialProfile = nullptr;
	if (PlayerPresentationCatalog)
	{
		const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
		const FReEchoCsvCharacterRow* Character =
		    Snapshot.IsValid() ? Snapshot->FindCharacter(ConfiguredCharacterId) : nullptr;
		PlayerSpatialProfile = Character ? PlayerPresentationCatalog->ResolveProfile(Character->AppearanceId) : nullptr;
	}
	ActiveSpatialProfile = PlayerSpatialProfile;

	ComposedPresentationProfile = nullptr;
	if (EchoProfile && PlayerSpatialProfile)
	{
		ComposedPresentationProfile = DuplicateObject<UReEcho2DCharacterPresentationProfile>(EchoProfile, this);
		ComposedPresentationProfile->WorldHeight = PlayerSpatialProfile->WorldHeight;
		ComposedPresentationProfile->bAutoAlignFootpoint = PlayerSpatialProfile->bAutoAlignFootpoint;
		ComposedPresentationProfile->FootpointOffset = PlayerSpatialProfile->FootpointOffset;
	}
	ActivePresentationProfile = ComposedPresentationProfile ? ComposedPresentationProfile.Get() : EchoProfile;
	if (PresentationController)
	{
		PresentationController->Configure(
		    EchoAnimation, ActivePresentationProfile, Weapon ? Weapon->GetEquippedWeaponVisualKey() : NAME_None);
	}
	if (Weapon)
	{
		Weapon->ConfigureHeldPresentation(ActivePresentationProfile);
	}
	BaseVisualLocation = FlipbookRoot->GetRelativeLocation();
	BaseVisualScale = FlipbookRoot->GetRelativeScale3D();
	AuthoredMotionLocation = PresentationMotionRoot->GetRelativeLocation();
	BaseEffectsLocation = EffectsRoot->GetRelativeLocation();
	BaseEffectsScale = EffectsRoot->GetRelativeScale3D();
}

void AReEchoEchoActor::ApplyPlayerSpatialAuthoring(const AReEchoPlayerPawn& Player)
{
	AReEchoPlayerPawn* PlayerDefaults = Player.GetClass()->GetDefaultObject<AReEchoPlayerPawn>();
	if (!PlayerDefaults)
	{
		return;
	}

	auto CopyRelativeTransform = [](USceneComponent* Target, AReEchoPlayerPawn& SourceActor, const FName SourceName)
	{
		const USceneComponent* Source = Cast<USceneComponent>(SourceActor.GetDefaultSubobjectByName(SourceName));
		if (Target && Source)
		{
			Target->SetRelativeTransform(Source->GetRelativeTransform());
		}
	};
	// Motion, effects and ground nodes carry frame-local feedback at runtime. Seed those from the
	// Blueprint CDO, then read only stable authored roots from the live Player instance.
	CopyRelativeTransform(PresentationMotionRoot, *PlayerDefaults, TEXT("PresentationMotionRoot"));
	CopyRelativeTransform(GroundRoot, *PlayerDefaults, TEXT("GroundRoot"));
	CopyRelativeTransform(EffectsRoot, *PlayerDefaults, TEXT("EffectsRoot"));
	CopyRelativeTransform(GroundShadow, *PlayerDefaults, TEXT("GroundShadow"));
	AReEchoPlayerPawn& LivePlayer = const_cast<AReEchoPlayerPawn&>(Player);
	CopyRelativeTransform(PresentationRoot, LivePlayer, TEXT("PresentationRoot"));
	CopyRelativeTransform(FootRoot, LivePlayer, TEXT("FootRoot"));
	CopyRelativeTransform(FlipbookRoot, LivePlayer, TEXT("FlipbookRoot"));
	CopyRelativeTransform(AttackVfxRoot, LivePlayer, TEXT("AttackVfxRoot"));
	CopyRelativeTransform(HurtVfxRoot, LivePlayer, TEXT("HurtVfxRoot"));
	SetActorScale3D(Player.GetActorScale3D());

	BaseVisualLocation = FlipbookRoot->GetRelativeLocation();
	BaseVisualScale = FlipbookRoot->GetRelativeScale3D();
	AuthoredMotionLocation = PresentationMotionRoot->GetRelativeLocation();
	BaseEffectsLocation = EffectsRoot->GetRelativeLocation();
	BaseEffectsScale = EffectsRoot->GetRelativeScale3D();
	AuthoredGroundRootLocation = GroundRoot->GetRelativeLocation();
	AuthoredGroundShadowScale = GroundShadow->GetRelativeScale3D();
}

#if WITH_DEV_AUTOMATION_TESTS
void AReEchoEchoActor::ApplyPlayerSpatialAuthoringForTests(const AReEchoPlayerPawn& Player)
{
	ApplyPlayerSpatialAuthoring(Player);
}

void AReEchoEchoActor::RefreshSpatialPresentationForTests()
{
	UpdatePresentationState();
}

FReEchoEchoSpatialPresentationSnapshot AReEchoEchoActor::CaptureSpatialPresentationForTests() const
{
	FReEchoEchoSpatialPresentationSnapshot Snapshot;
	Snapshot.ActorTransform = GetActorTransform();
	Snapshot.PresentationRootTransform = PresentationRoot->GetRelativeTransform();
	Snapshot.FootRootTransform = FootRoot->GetRelativeTransform();
	Snapshot.MotionRootTransform = PresentationMotionRoot->GetRelativeTransform();
	Snapshot.FlipbookRootTransform = FlipbookRoot->GetRelativeTransform();
	Snapshot.EffectsRootTransform = EffectsRoot->GetRelativeTransform();
	Snapshot.AttackVfxRootTransform = AttackVfxRoot->GetRelativeTransform();
	Snapshot.HurtVfxRootTransform = HurtVfxRoot->GetRelativeTransform();
	Snapshot.GroundRootTransform = GroundRoot->GetRelativeTransform();
	Snapshot.GroundShadowTransform = GroundShadow->GetRelativeTransform();
	Snapshot.RendererTransform = EchoAnimation->GetRelativeTransform();
	const float ActorScale = FMath::Abs(GetActorScale3D().Z);
	Snapshot.NormalizedCharacterHeight = ActiveSpatialProfile ? ActiveSpatialProfile->WorldHeight * ActorScale : 0.0f;
	if (const UPaperFlipbook* Flipbook = EchoAnimation->GetFlipbook())
	{
		Snapshot.PresentedCharacterWidth =
		    UReEcho2DAnimationComponent::CalculateFlipbookPresentationWidth(Flipbook->GetRenderBounds(),
		                                                                    EchoAnimation->GetRelativeTransform(),
		                                                                    FlipbookRoot->GetRelativeTransform()) *
		    ActorScale;
	}
	Snapshot.ShadowReferenceWidth = CalculateSpatialShadowWidth() * ActorScale;
	return Snapshot;
}

const UReEcho2DCharacterPresentationProfile* AReEchoEchoActor::GetSpatialProfileForTests() const
{
	return ActiveSpatialProfile;
}
#endif

void AReEchoEchoActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bAudioLifecycleStarted && EndPlayReason == EEndPlayReason::Destroyed)
	{
		CombatAudioAdapter->PostConfiguredEvent(FReEchoAudioEvents::EchoEnd, GetActorLocation());
		bAudioLifecycleStarted = false;
	}
	if (Weapon)
	{
		Weapon->Destroy();
		Weapon = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

const FReEchoStatBlock& AReEchoEchoActor::GetCurrentStats() const
{
	return Combatant->Stats;
}

float AReEchoEchoActor::GetCurrentHealth() const
{
	return Combatant->CurrentHealth;
}

void AReEchoEchoActor::ConfigureCardRules(const FReEchoCardRuleSnapshot& Rules, const FReEchoStatBlock& PlayerStats)
{
	bCanAttack = Rules.bEchoesCanAttack;
	if (Combatant && Rules.EchoHealthMultiplier > 1.0f)
	{
		FReEchoStatBlock Stats = Combatant->Stats;
		Stats.HpMax = PlayerStats.HpMax * Rules.EchoHealthMultiplier;
		Stats.HpPoint = Stats.HpMax;
		Combatant->InitializeFromStats(Stats, true);
	}
}

void AReEchoEchoActor::PlayCardAuraPulse(const FReEchoCardRuleSnapshot& Rules)
{
	if (CombatVfx && IsCombatTargetAlive())
	{
		CombatVfx->PlayEchoCardAuraPulse(Rules.bWaterEchoAura, Rules.bGrassEchoAura);
	}
}

bool AReEchoEchoActor::IsCombatTargetAlive() const
{
	return Combatant && Combatant->IsAlive();
}

bool AReEchoEchoActor::IntersectsCombatPath(const FVector& PathStart,
                                            const FVector& PathEnd,
                                            const float CarrierRadius) const
{
	return FMath::PointDistToSegment(GetActorLocation(), PathStart, PathEnd) <= FMath::Max(0.0f, CarrierRadius) + 35.0f;
}

void AReEchoEchoActor::ModifyOutgoingHit(FReEchoHitIntent& Intent) const
{
	UReEchoRunSubsystem* Run = GetGameInstance() ? GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>() : nullptr;
	if (!Run)
	{
		return;
	}
	const AReEchoPlayerPawn* Player = Cast<AReEchoPlayerPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
	const float DistanceCm = Player ? FVector::Dist2D(Player->GetActorLocation(), GetActorLocation()) : 0.0f;
	const UReEchoCombatantComponent* TargetCombatant =
	    Intent.Target ? Intent.Target->FindComponentByClass<UReEchoCombatantComponent>() : nullptr;
	Run->ModifyCardOutgoingHit(Intent,
	                           Combatant ? Combatant->Stats : Run->CurrentBuild.Stats,
	                           DistanceCm,
	                           DistanceCm,
	                           Player != nullptr,
	                           TargetCombatant && TargetCombatant->GetElementState().Attached != EReEchoElement::None);
}

void AReEchoEchoActor::NotifyReactionResolved(const FName ReactionId) const
{
	if (UReEchoRunSubsystem* Run = GetGameInstance() ? GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>() : nullptr)
	{
		Run->NotifyCardReaction(ReactionId, false);
	}
}

float AReEchoEchoActor::GetReactionDamageMultiplier(const FName ReactionId) const
{
	const UReEchoRunSubsystem* Run =
	    GetGameInstance() ? GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>() : nullptr;
	return Run ? Run->GetCardReactionDamageMultiplier(ReactionId) : 1.0f;
}

bool AReEchoEchoActor::HasInfiniteStackingBurn() const
{
	const UReEchoRunSubsystem* Run =
	    GetGameInstance() ? GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>() : nullptr;
	return Run && Run->GetCardRules().bInfiniteStackingBurn;
}

void AReEchoEchoActor::NotifyKillResolved(const FName TargetDefinitionId) const
{
	if (UReEchoRunSubsystem* Run = GetGameInstance() ? GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>() : nullptr)
	{
		Run->NotifyCardKill(true, TargetDefinitionId);
	}
}

void AReEchoEchoActor::NotifyHitResolved(const FReEchoHitResolved& Result) const
{
	if (UReEchoRunSubsystem* Run = GetGameInstance() ? GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>() : nullptr)
	{
		const float Healing = Run->NotifyCardDamageResolved(Result.RawDamage, Result.AppliedDamage, true);
		if (Healing > 0.0f)
		{
			if (AReEchoPlayerPawn* Player = Cast<AReEchoPlayerPawn>(UGameplayStatics::GetPlayerPawn(this, 0)))
			{
				if (Player->Combatant)
				{
					Player->Combatant->ApplyHealing(Healing);
				}
			}
		}
	}
}

void AReEchoEchoActor::NotifyNegativeStatusApplied(const FName StatusId) const
{
	if (UReEchoRunSubsystem* Run = GetGameInstance() ? GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>() : nullptr)
	{
		const float Healing = Run->NotifyCardNegativeStatusApplied(StatusId, true);
		if (Healing > 0.0f)
		{
			if (AReEchoPlayerPawn* Player = Cast<AReEchoPlayerPawn>(UGameplayStatics::GetPlayerPawn(this, 0)))
			{
				if (Player->Combatant)
				{
					Player->Combatant->ApplyHealing(Healing);
				}
			}
		}
	}
}

void AReEchoEchoActor::NotifyDefeated(const EReEchoDamageSource DamageSource) const
{
	if (DamageSource == EReEchoDamageSource::Enemy)
	{
		if (UReEchoRunSubsystem* Run =
		        GetGameInstance() ? GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>() : nullptr)
		{
			Run->NotifyCardEchoDefeated();
			if (AReEchoPlayerPawn* Player = Cast<AReEchoPlayerPawn>(UGameplayStatics::GetPlayerPawn(this, 0)))
			{
				Player->Combatant->InitializeFromStats(Run->CurrentBuild.Stats, false);
			}
		}
	}
}

FString AReEchoEchoActor::GetPinnedWeaponDomainRevision() const
{
	return Weapon ? Weapon->GetPinnedWeaponDomainRevision() : FString();
}

FName AReEchoEchoActor::GetEquippedWeaponId() const
{
	return Weapon ? Weapon->GetEquippedWeaponId() : NAME_None;
}

FVector AReEchoEchoActor::EvaluateRecordedPosition(const float EncounterTime) const
{
	return Playback ? Playback->EvaluateRecordedPosition(EncounterTime) : GetActorLocation();
}

void AReEchoEchoActor::AdvanceEcho(const float EncounterTime)
{
	Playback->AdvancePlayback(EncounterTime);
	if (bBornVfxPending && !bDeferredBornReveal)
	{
		bBornVfxPending = false;
		PlayBornVfx();
	}
}

void AReEchoEchoActor::PrepareDeferredBornReveal(const float EncounterTime)
{
	Playback->AdvancePlayback(EncounterTime);
	PrepareBornRevealAtCurrentLocation();
}

void AReEchoEchoActor::PrepareBornRevealAtCurrentLocation()
{
	bBornVfxPending = true;
	bDeferredBornReveal = true;
	SetActorHiddenInGame(true);
	if (Weapon)
	{
		Weapon->SetActorHiddenInGame(true);
	}
}

bool AReEchoEchoActor::BeginDeferredBornReveal()
{
	if (!bDeferredBornReveal || !bBornVfxPending)
	{
		return false;
	}
	bBornVfxPending = false;
	return PlayBornVfx();
}

void AReEchoEchoActor::CompleteDeferredBornReveal()
{
	if (!bDeferredBornReveal)
	{
		return;
	}
	bDeferredBornReveal = false;
	bBornVfxPending = false;
	SetActorHiddenInGame(false);
	if (Weapon)
	{
		Weapon->SetActorHiddenInGame(false);
	}
}

bool AReEchoEchoActor::PlayBornVfx()
{
	if (!CombatVfx || !GroundShadow || !IsCombatTargetAlive())
	{
		return false;
	}
	constexpr float BornCircleToEchoWidthRatio = 1.45f;
	const float EchoWorldWidth = CalculateSpatialShadowWidth() * GetActorScale3D().GetAbsMax();
	FVector BornCircleCenter = GetActorLocation();
	BornCircleCenter.Z = GroundShadow->GetComponentLocation().Z;
	return CombatVfx->PlayEchoBornAtWorldLocation(BornCircleCenter, EchoWorldWidth * BornCircleToEchoWidthRatio);
}

bool AReEchoEchoActor::IsBornVfxPlaying() const
{
	return CombatVfx && CombatVfx->IsEchoBornEffectActive();
}

void AReEchoEchoActor::UpdatePresentationState()
{
	const FVector CurrentLocation = GetActorLocation();
	const bool bMoving =
	    bHasPresentationLocation && FVector::DistSquared2D(CurrentLocation, LastPresentationLocation) > 1.0f;
	if (PresentationController)
	{
		PresentationController->SetMoving(bMoving);
		PresentationController->SetFacingSign(VisualFacingSign);
	}
	LastPresentationLocation = CurrentLocation;
	bHasPresentationLocation = true;
	FlipbookRoot->SetRelativeLocation(BaseVisualLocation);
	FlipbookRoot->SetRelativeScale3D(BaseVisualScale);
	EffectsRoot->SetRelativeLocation(BaseEffectsLocation);
	EffectsRoot->SetRelativeScale3D(BaseEffectsScale);
	RefreshFootpointAlignment();
	PresentationMotionRoot->SetRelativeLocation(AuthoredMotionLocation + CalculatedFootAlignmentOffset);
	RefreshGroundShadowFromFlipbook();
	RefreshEchoAuraCenter();
}

void AReEchoEchoActor::RefreshEchoAuraCenter()
{
	const UPaperFlipbook* Flipbook = EchoAnimation ? EchoAnimation->GetFlipbook() : nullptr;
	if (!EffectsRoot || !EchoAuraVfxRoot || !EchoAnimation || !Flipbook)
	{
		return;
	}
	const FVector CenterWorld =
	    EchoAnimation->GetComponentTransform().TransformPosition(Flipbook->GetRenderBounds().Origin);
	EchoAuraVfxRoot->SetRelativeLocation(EffectsRoot->GetComponentTransform().InverseTransformPosition(CenterWorld));
}

void AReEchoEchoActor::RefreshFootpointAlignment()
{
	CalculatedFootAlignmentOffset = FVector::ZeroVector;
	const UPaperFlipbook* Flipbook = EchoAnimation ? EchoAnimation->GetFlipbook() : nullptr;
	if (!FlipbookRoot || !EchoAnimation || !Flipbook ||
	    (ActivePresentationProfile && !ActivePresentationProfile->bAutoAlignFootpoint))
	{
		return;
	}
	CalculatedFootAlignmentOffset = UReEcho2DAnimationComponent::CalculateFootAlignmentOffset(
	    Flipbook->GetRenderBounds(),
	    EchoAnimation->GetRelativeTransform(),
	    FlipbookRoot->GetRelativeTransform(),
	    AuthoredMotionLocation,
	    ActivePresentationProfile ? ActivePresentationProfile->FootpointOffset : FVector::ZeroVector);
}

void AReEchoEchoActor::RefreshGroundShadowFromFlipbook()
{
	const UPaperFlipbook* Flipbook = EchoAnimation ? EchoAnimation->GetFlipbook() : nullptr;
	const UStaticMesh* ShadowMesh = GroundShadow ? GroundShadow->GetStaticMesh() : nullptr;
	if (!FootRoot || !GroundRoot || !FlipbookRoot || !EchoAnimation || !Flipbook || !GroundShadow || !ShadowMesh)
	{
		return;
	}
	const FBoxSphereBounds FlipbookBounds = Flipbook->GetRenderBounds();
	const FVector LocalBottomCenter(
	    FlipbookBounds.Origin.X, FlipbookBounds.Origin.Y, FlipbookBounds.Origin.Z - FlipbookBounds.BoxExtent.Z);
	const FVector BottomWorld = EchoAnimation->GetComponentTransform().TransformPosition(LocalBottomCenter);
	const FVector BottomInFootRoot = FootRoot->GetComponentTransform().InverseTransformPosition(BottomWorld);
	GroundRoot->SetRelativeLocation(FVector(BottomInFootRoot.X, BottomInFootRoot.Y, AuthoredGroundRootLocation.Z));

	const float FlipbookWidth = CalculateSpatialShadowWidth();
	const float ShadowNativeWidth = ShadowMesh->GetBounds().BoxExtent.Y * 2.0f;
	if (FlipbookWidth > UE_SMALL_NUMBER && ShadowNativeWidth > UE_SMALL_NUMBER)
	{
		FVector ShadowScale = AuthoredGroundShadowScale;
		ShadowScale.Y = FlipbookWidth / ShadowNativeWidth;
		GroundShadow->SetRelativeScale3D(ShadowScale);
	}
}

float AReEchoEchoActor::CalculateSpatialShadowWidth() const
{
	const UPaperFlipbook* CurrentFlipbook = EchoAnimation ? EchoAnimation->GetFlipbook() : nullptr;
	if (!CurrentFlipbook || !FlipbookRoot)
	{
		return 0.0f;
	}

	const FName WeaponVisualSetId = Weapon ? Weapon->GetEquippedWeaponVisualKey() : NAME_None;
	const FGameplayTag SemanticKeys[] = {ReEcho2DAnimationTags::Move,
	                                     ReEcho2DAnimationTags::Attack_Basic,
	                                     ReEcho2DAnimationTags::Attack_Charge,
	                                     ReEcho2DAnimationTags::Hit,
	                                     ReEcho2DAnimationTags::Death};
	if (ActivePresentationProfile && ActiveSpatialProfile)
	{
		for (const FGameplayTag SemanticKey : SemanticKeys)
		{
			const FReEcho2DAnimationClip* EchoClip =
			    ActivePresentationProfile->ResolveClip(WeaponVisualSetId, SemanticKey);
			if (!EchoClip || EchoClip->Flipbook != CurrentFlipbook)
			{
				continue;
			}
			const FReEcho2DAnimationClip* SpatialClip =
			    ActiveSpatialProfile->ResolveClip(WeaponVisualSetId, SemanticKey);
			if (!SpatialClip || !SpatialClip->Flipbook)
			{
				break;
			}
			const FBoxSphereBounds SpatialBounds = SpatialClip->Flipbook->GetRenderBounds();
			const float NativeHeight = SpatialBounds.BoxExtent.Z * 2.0f;
			if (NativeHeight <= UE_SMALL_NUMBER)
			{
				break;
			}
			const float UniformScale = FMath::Max(ActiveSpatialProfile->WorldHeight, 1.0f) / NativeHeight;
			const FTransform SpatialRendererTransform(FQuat::Identity, SpatialClip->LocalOffset, FVector(UniformScale));
			return UReEcho2DAnimationComponent::CalculateFlipbookPresentationWidth(
			    SpatialBounds, SpatialRendererTransform, FlipbookRoot->GetRelativeTransform());
		}
	}

	return UReEcho2DAnimationComponent::CalculateFlipbookPresentationWidth(CurrentFlipbook->GetRenderBounds(),
	                                                                       EchoAnimation->GetRelativeTransform(),
	                                                                       FlipbookRoot->GetRelativeTransform());
}

void AReEchoEchoActor::UpdateFacingSign(const FVector& AimDirection)
{
	if (AimDirection.IsNearlyZero())
	{
		return;
	}
	const APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0);
	const FVector CameraRight = CameraManager
	                                ? FRotationMatrix(CameraManager->GetCameraRotation()).GetUnitAxis(EAxis::Y)
	                                : FVector::RightVector;
	const float HorizontalAim = FVector::DotProduct(AimDirection, CameraRight);
	if (FMath::Abs(HorizontalAim) > UE_SMALL_NUMBER)
	{
		VisualFacingSign = HorizontalAim >= 0.0f ? 1.0f : -1.0f;
	}
}

void AReEchoEchoActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (CharacterSprite->IsVisible())
	{
		ReEchoBillboardDebug::DrawBounds(this, CharacterSprite, FColor(180, 70, 255));
	}

	UpdatePresentationState();

	if (!Weapon || !bCanAttack || !IsCombatTargetAlive())
	{
		return;
	}

	AReEchoEnemyActor* NearestEnemy = nullptr;
	float NearestDistanceSquared = FMath::Square(AutoTargetRange);
	for (TActorIterator<AReEchoEnemyActor> EnemyIterator(GetWorld()); EnemyIterator; ++EnemyIterator)
	{
		if (!EnemyIterator->IsAlive())
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared2D(GetActorLocation(), EnemyIterator->GetActorLocation());
		if (DistanceSquared < NearestDistanceSquared)
		{
			NearestDistanceSquared = DistanceSquared;
			NearestEnemy = *EnemyIterator;
		}
	}

	if (!NearestEnemy)
	{
		return;
	}

	const FVector AimDirection = (NearestEnemy->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	if (!AimDirection.IsNearlyZero())
	{
		AttackAimDirection = AimDirection;
		UpdateFacingSign(AimDirection);
	}

	if (Weapon->TryBasicAttack(Combatant))
	{
		PresentationController->PlayAction(
		    ReEcho2DAnimationTags::Attack_Basic, true, NextPresentationAttackInstanceId++);
	}
}
