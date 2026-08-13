#include "Graybox/ReEchoEnemyActor.h"

#include "AbilitySystem/ReEchoCombatAttributeSet.h"
#include "AbilitySystem/ReEchoGameplayEffects.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"

#include "Core/ReEchoBalanceSettings.h"
#include "Graybox/ReEchoAttackEffects.h"
#include "Graybox/ReEchoBomberRules.h"
#include "Graybox/ReEchoHealthBarActor.h"
#include "Graybox/ReEchoBillboardDebug.h"
#include "Graybox/ReEchoCollisionDebug.h"
#include "UI/ReEchoDamageNumberActor.h"
#include "Combat/ReEchoCombatantComponent.h"
#include "Combat/ReEchoCombatContracts.h"
#include "Combat/ReEchoCombatAudioAdapterComponent.h"
#include "Combat/ReEchoElementReaction.h"
#include "Combat/ReEchoHitResolver.h"
#include "Components/BillboardComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Camera/PlayerCameraManager.h"
#include "Player/ReEchoPlayerPawn.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Presentation/Animation2D/ReEcho2DAnimationComponent.h"
#include "Presentation/Animation2D/ReEcho2DAnimationTags.h"
#include "Presentation/Animation2D/ReEcho2DCharacterPresentationProfile.h"
#include "Presentation/Animation2D/ReEcho2DPresentationController.h"
#include "Presentation/Animation2D/ReEcho2DFrameCollisionDriver.h"
#include "UObject/ConstructorHelpers.h"

namespace ReEchoEnemyVisual
{
constexpr float ScaleMultiplier = 1.5f;
constexpr float CharacterWorldHeight = 244.8f * ScaleMultiplier;
constexpr float CollisionRadius = 34.56f * ScaleMultiplier;
constexpr float CollisionHalfHeight = 122.4f * ScaleMultiplier;
constexpr float ShadowScaleX = 0.512f * ScaleMultiplier;
constexpr float ShadowScaleY = 0.5376f * ScaleMultiplier;
constexpr float HealthBarHeight = 65.28f * ScaleMultiplier;
constexpr float HealthBarWidthScale = 0.72f * ScaleMultiplier;
}

AReEchoEnemyActor::AReEchoEnemyActor()
{
	PrimaryActorTick.bCanEverTick = true;
	AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
	AbilitySystem->SetIsReplicated(true);
	AbilitySystem->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	CombatAttributes = CreateDefaultSubobject<UReEchoCombatAttributeSet>(TEXT("CombatAttributes"));
	Collision = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->InitCapsuleSize(ReEchoEnemyVisual::CollisionRadius, ReEchoEnemyVisual::CollisionHalfHeight);
	Collision->SetCollisionProfileName(TEXT("Pawn"));
	Collision->SetVisibility(false);
	GroundShadow = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GroundShadow"));
	GroundShadow->SetupAttachment(RootComponent);
	GroundShadow->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GroundShadow->SetCastShadow(false);
	GroundShadow->SetTranslucentSortPriority(-1);
	GroundShadow->SetAbsolute(false, false, true);
	GroundShadow->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")));
	GroundShadow->SetRelativeLocation(FVector(0.0f, 0.0f, -ReEchoEnemyVisual::CharacterWorldHeight * 0.28f));
	GroundShadow->SetRelativeScale3D(FVector(ReEchoEnemyVisual::ShadowScaleX, ReEchoEnemyVisual::ShadowScaleY, 1.0f));
	if (UMaterialInterface* ShadowBase = LoadObject<UMaterialInterface>(
	        nullptr, TEXT("/Paper2D/TranslucentUnlitSpriteMaterial.TranslucentUnlitSpriteMaterial")))
	{
		UMaterialInstanceDynamic* ShadowMaterial = UMaterialInstanceDynamic::Create(ShadowBase, this);
		ShadowMaterial->SetTextureParameterValue(
		    TEXT("SpriteTexture"),
		    LoadObject<UTexture2D>(nullptr,
		                           TEXT("/Game/ReEcho/Textures/Characters/SoftGroundShadow.SoftGroundShadow")));
		GroundShadow->SetMaterial(0, ShadowMaterial);
	}
	VisualEffectRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualEffectRoot"));
	VisualEffectRoot->SetupAttachment(RootComponent);
	CharacterSprite = CreateDefaultSubobject<UBillboardComponent>(TEXT("CharacterSprite"));
	CharacterSprite->SetupAttachment(VisualEffectRoot);
	CharacterSprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CharacterSprite->SetHiddenInGame(false);
	CharacterSprite->SetVisibility(false);
	CharacterSprite->SetAbsolute(false, false, true);
	CharacterSprite->bIsScreenSizeScaled = false;
	SequenceAnimation = CreateDefaultSubobject<UReEcho2DAnimationComponent>(TEXT("SequenceAnimation"));
	SequenceAnimation->SetupAttachment(VisualEffectRoot);
	PresentationController = CreateDefaultSubobject<UReEcho2DPresentationController>(TEXT("PresentationController"));
	FrameCollisionDriver = CreateDefaultSubobject<UReEcho2DFrameCollisionDriver>(TEXT("FrameCollisionDriver"));
	PresentationController->BindCollisionDriver(FrameCollisionDriver);

	ElementAuraRing = CreateDefaultSubobject<UTextRenderComponent>(TEXT("ElementAuraRing"));
	ElementAuraRing->SetupAttachment(RootComponent);
	ElementAuraRing->SetHorizontalAlignment(EHTA_Center);
	ElementAuraRing->SetVerticalAlignment(EVRTA_TextCenter);
	ElementAuraRing->SetWorldSize(270.0f);
	ElementAuraRing->SetText(FText::FromString(TEXT("O")));
	ElementAuraRing->SetRelativeLocation(FVector(0.0f, 0.0f, 18.0f));
	ElementAuraRing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ElementAuraRing->SetCastShadow(false);
	ElementAuraRing->SetTranslucentSortPriority(-2);
	ElementAuraRing->SetVisibility(false);

	ElementAttachmentLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("ElementAttachmentLabel"));
	ElementAttachmentLabel->SetupAttachment(RootComponent);
	ElementAttachmentLabel->SetHorizontalAlignment(EHTA_Center);
	ElementAttachmentLabel->SetVerticalAlignment(EVRTA_TextCenter);
	ElementAttachmentLabel->SetWorldSize(30.0f);
	ElementAttachmentLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 205.0f));
	ElementAttachmentLabel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ElementAttachmentLabel->SetCastShadow(false);
	ElementAttachmentLabel->SetTranslucentSortPriority(23);
	ElementAttachmentLabel->SetVisibility(false);

	if (UMaterialInterface* UnlitTextMaterial =
	        LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineMaterials/UnlitText.UnlitText")))
	{
		ElementAuraRing->SetTextMaterial(UnlitTextMaterial);
		ElementAttachmentLabel->SetTextMaterial(UnlitTextMaterial);
	}

	ElementAuraLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("ElementAuraLight"));
	ElementAuraLight->SetupAttachment(RootComponent);
	ElementAuraLight->SetRelativeLocation(FVector(0.0f, 0.0f, 45.0f));
	ElementAuraLight->SetIntensity(900.0f);
	ElementAuraLight->SetAttenuationRadius(240.0f);
	ElementAuraLight->SetSourceRadius(35.0f);
	ElementAuraLight->SetCastShadows(false);
	ElementAuraLight->SetVisibility(false);
	static ConstructorHelpers::FObjectFinder<UTexture2D> BossTextureFinder(
	    TEXT("/Game/ReEcho/Textures/Characters/Boss2D.Boss2D"));
	BossTexture = BossTextureFinder.Object;
	static ConstructorHelpers::FObjectFinder<UReEcho2DCharacterPresentationProfile> GruntProfileFinder(
	    TEXT("/Game/ReEcho/Animation2D/DA_Enemy_Grunt.DA_Enemy_Grunt"));
	GruntPresentationProfile = GruntProfileFinder.Object;
	static ConstructorHelpers::FObjectFinder<UReEcho2DCharacterPresentationProfile> RabbitDollProfileFinder(
	    TEXT("/Game/ReEcho/Animation2D/DA_Enemy_RabbitDoll.DA_Enemy_RabbitDoll"));
	RabbitDollPresentationProfile = RabbitDollProfileFinder.Object;
	static ConstructorHelpers::FObjectFinder<UReEcho2DCharacterPresentationProfile> GoatPriestProfileFinder(
	    TEXT("/Game/ReEcho/Animation2D/DA_Enemy_GoatPriest.DA_Enemy_GoatPriest"));
	GoatPriestPresentationProfile = GoatPriestProfileFinder.Object;
	static ConstructorHelpers::FObjectFinder<UReEcho2DCharacterPresentationProfile> FoxProfileFinder(
	    TEXT("/Game/ReEcho/Animation2D/DA_Enemy_Fox.DA_Enemy_Fox"));
	FoxPresentationProfile = FoxProfileFinder.Object;
	Combatant = CreateDefaultSubobject<UReEchoCombatantComponent>(TEXT("Combatant"));
	CombatEvents = CreateDefaultSubobject<UReEchoCombatEventsComponent>(TEXT("CombatEvents"));
	CombatAudioAdapter = CreateDefaultSubobject<UReEchoCombatAudioAdapterComponent>(TEXT("CombatAudioAdapter"));
	Tags.Add(TEXT("ReEchoEnemy"));
}

void AReEchoEnemyActor::BeginPlay()
{
	Super::BeginPlay();
	AbilitySystem->InitAbilityActorInfo(this, this);
	Combatant->BindToAbilitySystem(AbilitySystem);
	CombatEvents->OnElementStateChanged.AddDynamic(this, &AReEchoEnemyActor::HandleElementStateChanged);
	CombatEvents->OnHurt.AddDynamic(this, &AReEchoEnemyActor::HandleCombatHurt);
	CombatEvents->OnDeath.AddDynamic(this, &AReEchoEnemyActor::HandleCombatDeath);
}

UAbilitySystemComponent* AReEchoEnemyActor::GetAbilitySystemComponent() const
{
	return AbilitySystem;
}

void AReEchoEnemyActor::Configure(EReEchoEnemyKind InKind, int32 SpawnIndex)
{
	Kind = InKind;
	Combatant->ResetElementState();
	UpdateElementAttachmentVisual();
	VisualVariantIndex = SpawnIndex;
	FReEchoStatBlock Stats;
	switch (Kind)
	{
		case EReEchoEnemyKind::Shield:
			Stats.HpMax = 55.f;
			MoveSpeed = 50.f;
			ContactDamage = 14.f;
			AttackInterval = 1.7f;
			break;
		case EReEchoEnemyKind::Bomber:
			Stats.HpMax = 16.f;
			MoveSpeed = 175.f;
			ContactDamage = GetDefault<UReEchoBalanceSettings>()->BomberDamage;
			FuseRemaining = 0.0f;
			bBomberFuseActive = false;
			break;
		case EReEchoEnemyKind::Boss:
			Stats.HpMax = 650.f;
			MoveSpeed = 45.f;
			ContactDamage = 18.f;
			AttackInterval = 2.f;
			break;
		default:
			Stats.HpMax = 28.f;
			MoveSpeed = 95.f;
			ContactDamage = 9.f;
			AttackInterval = 1.3f;
			break;
	}
	Combatant->InitializeFromStats(Stats, true);
	ApplyVisual();
	if (!HealthBar)
	{
		HealthBar = GetWorld()->SpawnActor<AReEchoHealthBarActor>();
	}
	if (HealthBar)
	{
		HealthBar->Initialize(Combatant,
		                      FLinearColor(1.f, 0.08f, 0.04f),
		                      ReEchoEnemyVisual::HealthBarHeight,
		                      ReEchoEnemyVisual::HealthBarWidthScale,
		                      CharacterSprite);
	}
}

void AReEchoEnemyActor::ApplyVisual()
{
	const bool bIsBoss = Kind == EReEchoEnemyKind::Boss;
	Collision->SetCapsuleSize(ReEchoEnemyVisual::CollisionRadius, ReEchoEnemyVisual::CollisionHalfHeight);

	UTexture2D* CharacterTexture = bIsBoss ? BossTexture : nullptr;
	const float CharacterHalfHeight = ReEchoEnemyVisual::CharacterWorldHeight * 0.5f;
	CharacterSprite->SetVisibility(true);
	CharacterSprite->SetHiddenInGame(false);
	CharacterSprite->SetRelativeLocation(FVector::ZeroVector);
	GroundShadow->SetRelativeLocation(FVector(0.0f, 0.0f, -ReEchoEnemyVisual::CharacterWorldHeight * 0.28f));
	GroundShadow->SetRelativeScale3D(FVector(ReEchoEnemyVisual::ShadowScaleX, ReEchoEnemyVisual::ShadowScaleY, 1.0f));

	FVector CenteredLocation = GetActorLocation();
	CenteredLocation.Z += CharacterHalfHeight;
	SetActorLocation(CenteredLocation, false, nullptr, ETeleportType::TeleportPhysics);
	if (CharacterTexture)
	{
		CharacterSprite->SetSprite(CharacterTexture);
		// 1 Unreal unit 对应 1 个源图像素；不同怪物直接保留各自贴图宽高。
		CharacterSprite->SetWorldScale3D(FVector::OneVector);
	}

	PresentationController->Configure(CharacterSprite, SequenceAnimation,
	                                  bIsBoss ? nullptr : ResolveEnemyPresentationProfile());
	VisualEffectRoot->SetRelativeLocation(FVector::ZeroVector);
	VisualEffectRoot->SetRelativeScale3D(FVector::OneVector);
	BaseVisualLocation = VisualEffectRoot->GetRelativeLocation();
	BaseVisualScale = VisualEffectRoot->GetRelativeScale3D();
}

UReEcho2DCharacterPresentationProfile* AReEchoEnemyActor::ResolveEnemyPresentationProfile() const
{
	// Minimal validation presentation: every non-Boss enemy deterministically cycles through
	// the three currently approved animated appearances without any static texture authority.
	switch (FMath::Abs(VisualVariantIndex) % 4)
	{
		case 1:
			return RabbitDollPresentationProfile ? RabbitDollPresentationProfile : GruntPresentationProfile;
		case 2:
			return GoatPriestPresentationProfile ? GoatPriestPresentationProfile : GruntPresentationProfile;
		case 3:
			return FoxPresentationProfile ? FoxPresentationProfile : GruntPresentationProfile;
		default:
			return GruntPresentationProfile;
	}
}

bool AReEchoEnemyActor::IsAlive() const
{
	return Combatant && Combatant->IsAlive();
}

FReEchoEnemyRuntimeState AReEchoEnemyActor::CaptureRuntimeState() const
{
	FReEchoEnemyRuntimeState Result;
	Result.Kind = static_cast<uint8>(Kind);
	Result.SpawnIndex = VisualVariantIndex;
	Result.Transform = GetActorTransform();
	Result.CurrentHealth = Combatant ? Combatant->CurrentHealth : 0.0f;
	Result.ElementState = Combatant ? Combatant->GetElementState() : FReEchoElementState{};
	const float CurrentTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	Result.ElementState.ImmunityUntil = FMath::Max(0.0f, Result.ElementState.ImmunityUntil - CurrentTimeSeconds);
	for (TPair<FName, float>& ActiveStatus : Result.ElementState.ActiveStatusUntilSeconds)
	{
		ActiveStatus.Value = FMath::Max(0.0f, ActiveStatus.Value - CurrentTimeSeconds);
	}
	Result.ElementState.BurnNextTickTimeSeconds =
	    Result.ElementState.bBurnActive
	        ? FMath::Max(0.0f, Result.ElementState.BurnNextTickTimeSeconds - CurrentTimeSeconds)
	        : 0.0f;
	Result.ElementState.BurnAttack = {};
	Result.AttackCooldown = AttackCooldown;
	Result.FuseRemaining = FuseRemaining;
	Result.bBomberFuseActive = bBomberFuseActive;
	Result.HitReactionRemaining = HitReactionRemaining;
	Result.KnockbackVelocity = KnockbackVelocity;
	Result.ShakeDirection = ShakeDirection;
	return Result;
}

void AReEchoEnemyActor::RestoreRuntimeState(const FReEchoEnemyRuntimeState& SavedState)
{
	const EReEchoEnemyKind SavedKind = static_cast<EReEchoEnemyKind>(SavedState.Kind);
	Configure(SavedKind, SavedState.SpawnIndex);
	SetActorTransform(SavedState.Transform, false, nullptr, ETeleportType::TeleportPhysics);
	if (Combatant)
	{
		Combatant->RestoreCurrentHealth(SavedState.CurrentHealth);
	}
	FReEchoElementState RestoredElementState = SavedState.ElementState;
	const float CurrentTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	RestoredElementState.ImmunityUntil = CurrentTimeSeconds + FMath::Max(0.0f, SavedState.ElementState.ImmunityUntil);
	for (TPair<FName, float>& ActiveStatus : RestoredElementState.ActiveStatusUntilSeconds)
	{
		ActiveStatus.Value = CurrentTimeSeconds + FMath::Max(0.0f, ActiveStatus.Value);
	}
	RestoredElementState.BurnNextTickTimeSeconds =
	    RestoredElementState.bBurnActive
	        ? CurrentTimeSeconds + FMath::Max(0.0f, SavedState.ElementState.BurnNextTickTimeSeconds)
	        : 0.0f;
	RestoredElementState.BurnAttack = {};
	if (Combatant)
	{
		Combatant->RestoreElementState(RestoredElementState);
	}
	AttackCooldown = FMath::Max(0.0f, SavedState.AttackCooldown);
	FuseRemaining = FMath::Max(0.0f, SavedState.FuseRemaining);
	bBomberFuseActive = SavedState.bBomberFuseActive;
	HitReactionRemaining = FMath::Max(0.0f, SavedState.HitReactionRemaining);
	KnockbackVelocity = SavedState.KnockbackVelocity;
	ShakeDirection = SavedState.ShakeDirection;
	PreviousShakeOffset = FVector::ZeroVector;
	UpdateElementAttachmentVisual();
}

bool AReEchoEnemyActor::IntersectsProjectilePath(const FVector& PathStart,
                                                 const FVector& PathEnd,
                                                 const float ProjectileRadius) const
{
	if (!Collision || !Collision->IsCollisionEnabled())
	{
		return false;
	}

	const FVector CapsuleCenter = Collision->GetComponentLocation();
	const FVector CapsuleAxis = Collision->GetUpVector();
	const float CapsuleRadius = Collision->GetScaledCapsuleRadius();
	const float CapsuleSegmentHalfLength = FMath::Max(0.0f, Collision->GetScaledCapsuleHalfHeight() - CapsuleRadius);
	const FVector CapsuleStart = CapsuleCenter - CapsuleAxis * CapsuleSegmentHalfLength;
	const FVector CapsuleEnd = CapsuleCenter + CapsuleAxis * CapsuleSegmentHalfLength;

	FVector ClosestOnProjectile;
	FVector ClosestOnCapsule;
	FMath::SegmentDistToSegmentSafe(
	    PathStart, PathEnd, CapsuleStart, CapsuleEnd, ClosestOnProjectile, ClosestOnCapsule);
	const float CombinedRadius = CapsuleRadius + FMath::Max(0.0f, ProjectileRadius);
	return FVector::DistSquared(ClosestOnProjectile, ClosestOnCapsule) <= FMath::Square(CombinedRadius);
}

void AReEchoEnemyActor::StartHitReaction(const FVector& SourceLocation)
{
	constexpr float StandardKnockbackSpeed = 360.f;
	constexpr float BossKnockbackSpeed = 140.f;
	constexpr float ReactionDuration = 0.22f;

	if (!PreviousShakeOffset.IsNearlyZero())
	{
		VisualEffectRoot->SetRelativeLocation(BaseVisualLocation);
		PreviousShakeOffset = FVector::ZeroVector;
	}

	FVector KnockbackDirection = (GetActorLocation() - SourceLocation).GetSafeNormal2D();
	if (KnockbackDirection.IsNearlyZero())
	{
		KnockbackDirection = -GetActorForwardVector().GetSafeNormal2D();
	}

	const float KnockbackSpeed = Kind == EReEchoEnemyKind::Boss ? BossKnockbackSpeed : StandardKnockbackSpeed;
	KnockbackVelocity = KnockbackDirection * KnockbackSpeed;
	ShakeDirection = FVector::CrossProduct(FVector::UpVector, KnockbackDirection).GetSafeNormal();
	HitReactionRemaining = ReactionDuration;
}

bool AReEchoEnemyActor::UpdateHitReaction(float DeltaSeconds)
{
	constexpr float ReactionDuration = 0.22f;
	constexpr float KnockbackDrag = 10.f;
	constexpr float ShakeFrequency = 90.f;
	constexpr float MaximumShakeDistance = 8.f;

	if (HitReactionRemaining <= 0.f)
	{
		return false;
	}

	if (!PreviousShakeOffset.IsNearlyZero())
	{
		VisualEffectRoot->SetRelativeLocation(BaseVisualLocation);
		PreviousShakeOffset = FVector::ZeroVector;
	}

	AddActorWorldOffset(KnockbackVelocity * DeltaSeconds, true);
	KnockbackVelocity = FMath::VInterpTo(KnockbackVelocity, FVector::ZeroVector, DeltaSeconds, KnockbackDrag);
	HitReactionRemaining = FMath::Max(0.f, HitReactionRemaining - DeltaSeconds);

	if (HitReactionRemaining > 0.f)
	{
		const float ElapsedTime = ReactionDuration - HitReactionRemaining;
		const float RemainingRatio = HitReactionRemaining / ReactionDuration;
		const float ShakeDistance = FMath::Sin(ElapsedTime * ShakeFrequency) * MaximumShakeDistance * RemainingRatio;
		PreviousShakeOffset = ShakeDirection * ShakeDistance;
		VisualEffectRoot->SetRelativeLocation(BaseVisualLocation + PreviousShakeOffset);
		VisualEffectRoot->SetRelativeScale3D(BaseVisualScale * FVector(1.12f, 0.86f, 1.0f));
	}

	return true;
}

void AReEchoEnemyActor::UpdateElementAttachmentVisual()
{
	const EReEchoElement AttachedElement = GetAttachedElement();
	const bool bHasAttachment = ReEchoElementReaction::IsCombatElement(AttachedElement);
	ElementAuraRing->SetVisibility(bHasAttachment);
	ElementAttachmentLabel->SetVisibility(bHasAttachment);
	ElementAuraLight->SetVisibility(bHasAttachment);
	if (!bHasAttachment)
	{
		return;
	}

	const FLinearColor ElementColor = ReEchoElementReaction::GetElementColor(AttachedElement);
	ElementAuraRing->SetTextRenderColor(ElementColor.ToFColor(false));
	ElementAttachmentLabel->SetText(FText::FromString(ReEchoElementReaction::GetElementLabel(AttachedElement)));
	ElementAttachmentLabel->SetTextRenderColor(ElementColor.ToFColor(false));
	ElementAuraLight->SetLightColor(ElementColor);
}

void AReEchoEnemyActor::UpdateElementAttachmentFacing()
{
	if (!ElementAuraRing->IsVisible())
	{
		return;
	}
	if (APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		const FRotator CameraFacingRotation = (-Camera->GetCameraRotation().Vector()).Rotation();
		ElementAuraRing->SetWorldRotation(CameraFacingRotation);
		ElementAttachmentLabel->SetWorldRotation(CameraFacingRotation);
	}
	const float Pulse = 1.0f + 0.055f * FMath::Sin(VisualTime * 3.2f);
	ElementAuraRing->SetRelativeScale3D(FVector(Pulse, Pulse, 1.0f));
	ElementAuraLight->SetIntensity(850.0f + 180.0f * FMath::Sin(VisualTime * 2.6f));
}

void AReEchoEnemyActor::RefreshElementAttachmentVisual()
{
	UpdateElementAttachmentVisual();
}

EReEchoElement AReEchoEnemyActor::GetAttachedElement() const
{
	return Combatant ? Combatant->GetElementState().Attached : EReEchoElement::None;
}

const FReEchoElementState& AReEchoEnemyActor::GetElementState() const
{
	static const FReEchoElementState EmptyState;
	return Combatant ? Combatant->GetElementState() : EmptyState;
}

#if WITH_DEV_AUTOMATION_TESTS
FReEchoElementState& AReEchoEnemyActor::EditElementState()
{
	check(Combatant);
	return Combatant->EditElementStateForTests();
}
#endif

void AReEchoEnemyActor::HandleElementStateChanged(const FReEchoElementStateChangedEvent& Event)
{
	if (Event.Combatant == Combatant)
	{
		UpdateElementAttachmentVisual();
	}
}

void AReEchoEnemyActor::HandleCombatHurt(const FReEchoDamageEvent& Event)
{
	if (Event.Target != this || Event.AppliedDamage <= 0.0f)
	{
		return;
	}
	ReEchoAttackEffects::SpawnHitImpact(GetWorld(), Event.WorldLocation);
	const FLinearColor Color = Event.Element == EReEchoElement::None
	                               ? FLinearColor::White
	                               : ReEchoElementReaction::GetElementColor(Event.Element);
	AReEchoDamageNumberActor::SpawnDamageNumber(GetWorld(), Event.WorldLocation, Event.AppliedDamage, Color);
	StartHitReaction(Event.SourceWorldLocation);
}

void AReEchoEnemyActor::HandleCombatDeath(const FReEchoDamageEvent& Event)
{
	if (Event.Target != this)
	{
		return;
	}
	UpdateElementAttachmentVisual();
	SetActorEnableCollision(false);
	DeathVisualRemaining = 0.45f;
	SetLifeSpan(0.45f);
}

float AReEchoEnemyActor::ReceiveElementalDamage(const float Damage,
                                                const EReEchoElement Element,
                                                const FVector& SourceLocation,
                                                const float ReactionEfficiency,
                                                const FReEchoAttackIdentity Attack)
{
	FReEchoElementHitContext Context;
	Context.SourceLocation = SourceLocation;
	Context.Attack = Attack;
	Context.ReactionEfficiency = ReactionEfficiency;
	Context.SourceElementalAttack = Damage;
	Context.SourceEchoEfficiency = 1.0f;
	AActor* SourceActor = Attack.Source.Get();
	if (SourceActor)
	{
		if (const UReEchoCombatantComponent* SourceCombatant =
		        SourceActor->FindComponentByClass<UReEchoCombatantComponent>())
		{
			Context.SourceElementalAttack = SourceCombatant->Stats.ElementalAttack;
			Context.SourceEchoEfficiency = SourceCombatant->Stats.EchoEfficiency;
		}
	}
	const FReEchoElementExecutionResult Result =
	    ReEchoElementReaction::ApplyHitToWorld(*this, Element, Damage, Context);
	return Result.ImmediateDamageApplied;
}

float AReEchoEnemyActor::ModifyIncomingRawDamage(const FReEchoHitIntent& Intent) const
{
	if (Kind != EReEchoEnemyKind::Shield)
	{
		return Intent.RawDamage;
	}
	const FVector ToSource = (Intent.SourceLocation - GetActorLocation()).GetSafeNormal2D();
	const FVector Forward = GetActorForwardVector().GetSafeNormal2D();
	return FVector::DotProduct(Forward, ToSource) >= 0.0f ? 0.0f : Intent.RawDamage * 2.0f;
}

float AReEchoEnemyActor::ReceiveGrayboxDamage(float Damage,
                                              const FVector& SourceLocation,
                                              const FLinearColor& DamageNumberColor,
                                              const FReEchoAttackIdentity Attack)
{
	FReEchoHitIntent Intent;
	Intent.Attack = Attack;
	Intent.Target = this;
	Intent.RawDamage = Damage;
	Intent.SourceLocation = SourceLocation;
	Intent.HitLocation = GetActorLocation();
	const FReEchoHitResolved Result = ReEchoHitResolver::ResolvePhysicalHit(Intent);
	return Result.AppliedDamage;
}

void AReEchoEnemyActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!SequenceAnimation->IsAnimationActive())
	{
		ReEchoBillboardDebug::DrawBounds(
		    this, CharacterSprite, Kind == EReEchoEnemyKind::Boss ? FColor::Yellow : FColor::Red);
	}
	ReEchoCollisionDebug::DrawCapsule(this, Collision, Kind == EReEchoEnemyKind::Boss ? FColor::Orange : FColor::Cyan);
	VisualTime += DeltaSeconds;
	UpdateElementAttachmentFacing();
	if (!IsAlive())
	{
		UpdateDeathAnimation(DeltaSeconds);
		return;
	}
	ReEchoElementReaction::TickElementStatuses(*this, GetWorld() ? GetWorld()->GetTimeSeconds() : -1.0f);
	if (!IsAlive())
	{
		UpdateDeathAnimation(DeltaSeconds);
		return;
	}
	if (UpdateHitReaction(DeltaSeconds))
	{
		return;
	}

	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Player)
	{
		return;
	}
	FVector Delta = Player->GetActorLocation() - GetActorLocation();
	Delta.Z = 0.f;
	if (!Delta.IsNearlyZero())
	{
		SetActorRotation(Delta.Rotation());
	}
	const float Distance = Delta.Size();
	const bool bMoving = Distance > 75.0f;
	UpdateSpriteAnimation(DeltaSeconds, bMoving);
	if (Distance > 75.f)
	{
		AddActorWorldOffset(Delta.GetSafeNormal() * MoveSpeed * DeltaSeconds, true);
	}
	AttackCooldown -= DeltaSeconds;
	bool bBomberExplosionReady = false;
	if (Kind == EReEchoEnemyKind::Bomber)
	{
		const UReEchoBalanceSettings* Settings = GetDefault<UReEchoBalanceSettings>();
		if (!bBomberFuseActive && ReEchoBomberRules::ShouldStartFuse(Distance, Settings->BomberTriggerRadius))
		{
			bBomberFuseActive = true;
			FuseRemaining = FMath::Max(0.1f, Settings->BomberFuseDuration);
		}
		if (bBomberFuseActive)
		{
			FuseRemaining -= DeltaSeconds;
			bBomberExplosionReady = FuseRemaining <= 0.0f;
		}
	}
	const bool bContactAttackReady = Kind != EReEchoEnemyKind::Bomber && Distance <= 85.0f && AttackCooldown <= 0.0f;
	if (bContactAttackReady || bBomberExplosionReady)
	{
		StartAttackVisual();
		const bool bCanDamageTarget =
		    Kind != EReEchoEnemyKind::Bomber ||
		    ReEchoBomberRules::IsInsideExplosion(Distance, GetDefault<UReEchoBalanceSettings>()->BomberDamageRadius);
		if (bCanDamageTarget)
		{
			if (const AReEchoPlayerPawn* ReEchoPlayer = Cast<AReEchoPlayerPawn>(Player);
			    ReEchoPlayer && ReEchoPlayer->IsWeaponInvulnerable())
			{
				AttackCooldown = AttackInterval;
				return;
			}
			if (Player->FindComponentByClass<UReEchoCombatantComponent>())
			{
				FReEchoHitIntent Intent;
				Intent.Attack.Source = this;
				Intent.Attack.Sequence = ++AttackSequence;
				Intent.Target = Player;
				Intent.RawDamage = ContactDamage;
				Intent.DamageSource = EReEchoDamageSource::Enemy;
				Intent.SourceLocation = GetActorLocation();
				Intent.HitLocation = Player->GetActorLocation();
				const float Applied = ReEchoHitResolver::ResolvePhysicalHit(Intent).AppliedDamage;
				if (Applied > 0.f)
				{
					if (AReEchoPlayerPawn* ReEchoPlayer = Cast<AReEchoPlayerPawn>(Player))
					{
						ReEchoPlayer->PlayHitVisual();
					}
					ReEchoAttackEffects::SpawnHitImpact(GetWorld(), Player->GetActorLocation());
					AReEchoDamageNumberActor::SpawnDamageNumber(
					    GetWorld(), Player->GetActorLocation(), Applied, FLinearColor::White);
				}
			}
		}
		AttackCooldown = AttackInterval;
		if (Kind == EReEchoEnemyKind::Bomber)
		{
			ReceiveGrayboxDamage(9999.f, GetActorLocation());
		}
	}
}

void AReEchoEnemyActor::StartAttackVisual()
{
	AttackVisualRemaining = 0.22f;
	// The action is requested only after the existing gameplay attack gate commits. Profiles without
	// an Attack clip ignore it; Fox plays its authored one-shot and returns to its looping base clip.
	PresentationController->PlayAction(ReEcho2DAnimationTags::Attack_Basic, true);
}

void AReEchoEnemyActor::UpdateSpriteAnimation(const float DeltaSeconds, const bool bMoving)
{
	if (!VisualEffectRoot || HitReactionRemaining > 0.0f)
	{
		return;
	}
	AttackVisualRemaining = FMath::Max(0.0f, AttackVisualRemaining - DeltaSeconds);
	const float Bob = FMath::Sin(VisualTime * (bMoving ? 8.0f : 2.6f)) * (bMoving ? 3.5f : 1.5f);
	const float AttackPulse =
	    AttackVisualRemaining > 0.0f ? FMath::Sin((1.0f - AttackVisualRemaining / 0.22f) * PI) : 0.0f;
	VisualEffectRoot->SetRelativeLocation(BaseVisualLocation + FVector(AttackPulse * 15.0f, 0.0f, Bob));
	VisualEffectRoot->SetRelativeScale3D(BaseVisualScale *
	                                     FVector(1.0f + AttackPulse * 0.08f, 1.0f - AttackPulse * 0.04f, 1.0f));
}

void AReEchoEnemyActor::UpdateDeathAnimation(const float DeltaSeconds)
{
	DeathVisualRemaining = FMath::Max(0.0f, DeathVisualRemaining - DeltaSeconds);
	const float Ratio = DeathVisualRemaining / 0.45f;
	VisualEffectRoot->SetRelativeLocation(BaseVisualLocation + FVector(0.0f, 0.0f, -28.0f * (1.0f - Ratio)));
	VisualEffectRoot->SetRelativeScale3D(BaseVisualScale * FVector(Ratio, Ratio, 1.0f));
}
