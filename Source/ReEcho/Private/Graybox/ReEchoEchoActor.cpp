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
	FootRoot = CreateDefaultSubobject<USceneComponent>(TEXT("FootRoot"));
	FootRoot->SetupAttachment(PresentationRoot);
	PresentationMotionRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PresentationMotionRoot"));
	PresentationMotionRoot->SetupAttachment(FootRoot);
	FlipbookRoot = CreateDefaultSubobject<USceneComponent>(TEXT("FlipbookRoot"));
	FlipbookRoot->SetupAttachment(PresentationMotionRoot);
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
	GroundShadow = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GroundShadow"));
	GroundShadow->SetupAttachment(GroundRoot);
	GroundShadow->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GroundShadow->SetCastShadow(false);
	GroundShadow->SetTranslucentSortPriority(-10);
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
	ConfigureEchoAppearance(TEXT("J_SPADE"));
	BaseVisualLocation = FlipbookRoot->GetRelativeLocation();
	BaseVisualScale = FlipbookRoot->GetRelativeScale3D();
	AuthoredMotionLocation = PresentationMotionRoot->GetRelativeLocation();
	AuthoredGroundRootLocation = GroundRoot->GetRelativeLocation();
	AuthoredGroundShadowScale = GroundShadow->GetRelativeScale3D();

	Playback = CreateDefaultSubobject<UReEchoPlaybackComponent>(TEXT("Playback"));
	Combatant = CreateDefaultSubobject<UReEchoCombatantComponent>(TEXT("Combatant"));
	CombatEvents = CreateDefaultSubobject<UReEchoCombatEventsComponent>(TEXT("CombatEvents"));
	CombatAudioAdapter = CreateDefaultSubobject<UReEchoCombatAudioAdapterComponent>(TEXT("CombatAudioAdapter"));
	CombatVfx = CreateDefaultSubobject<UReEchoCombatVfxComponent>(TEXT("CombatVfx"));
	CombatVfx->ConfigureAttachmentRoots(AttackVfxRoot, HurtVfxRoot);
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

void AReEchoEchoActor::RefreshPresentationProfile()
{
	ActivePresentationProfile =
	    EchoPresentationCatalog ? EchoPresentationCatalog->ResolveProfile(ConfiguredCharacterId) : nullptr;
	if (PresentationController)
	{
		PresentationController->Configure(
		    EchoAnimation, ActivePresentationProfile, Weapon ? Weapon->GetEquippedWeaponVisualKey() : NAME_None);
	}
	BaseVisualLocation = FlipbookRoot->GetRelativeLocation();
	BaseVisualScale = FlipbookRoot->GetRelativeScale3D();
	AuthoredMotionLocation = PresentationMotionRoot->GetRelativeLocation();
}

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
	                           TargetCombatant && TargetCombatant->GetElementState().Attached != EReEchoElement::None);
}

void AReEchoEchoActor::NotifyReactionResolved(const FName ReactionId) const
{
	if (UReEchoRunSubsystem* Run = GetGameInstance() ? GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>() : nullptr)
	{
		Run->NotifyCardReaction(ReactionId, false);
	}
}

void AReEchoEchoActor::NotifyKillResolved() const
{
	if (UReEchoRunSubsystem* Run = GetGameInstance() ? GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>() : nullptr)
	{
		Run->NotifyCardKill(true);
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
	RefreshFootpointAlignment();
	PresentationMotionRoot->SetRelativeLocation(AuthoredMotionLocation + CalculatedFootAlignmentOffset);
	RefreshGroundShadowFromFlipbook();
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

	const float FlipbookWidth = UReEcho2DAnimationComponent::CalculateFlipbookPresentationWidth(
	    FlipbookBounds, EchoAnimation->GetRelativeTransform(), FlipbookRoot->GetRelativeTransform());
	const float ShadowNativeWidth = ShadowMesh->GetBounds().BoxExtent.Y * 2.0f;
	if (FlipbookWidth > UE_SMALL_NUMBER && ShadowNativeWidth > UE_SMALL_NUMBER)
	{
		FVector ShadowScale = AuthoredGroundShadowScale;
		ShadowScale.Y = FlipbookWidth / ShadowNativeWidth;
		GroundShadow->SetRelativeScale3D(ShadowScale);
	}
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
