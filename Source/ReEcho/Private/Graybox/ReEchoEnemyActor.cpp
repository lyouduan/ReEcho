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
#include "Combat/ReEchoElementReaction.h"
#include "Components/BillboardComponent.h"
#include "Components/CapsuleComponent.h"
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
	CharacterSprite = CreateDefaultSubobject<UBillboardComponent>(TEXT("CharacterSprite"));
	CharacterSprite->SetupAttachment(RootComponent);
	CharacterSprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CharacterSprite->SetHiddenInGame(false);
	CharacterSprite->SetVisibility(false);
	CharacterSprite->SetAbsolute(false, false, true);
	CharacterSprite->bIsScreenSizeScaled = false;

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
	static ConstructorHelpers::FObjectFinder<UTexture2D> GruntTextureFinders[] = {
	    {TEXT("/Game/ReEcho/Textures/Characters/NewCast/Enemy_Slime.Enemy_Slime")},
	    {TEXT("/Game/ReEcho/Textures/Characters/NewCast/Enemy_ThornSlime.Enemy_ThornSlime")},
	    {TEXT("/Game/ReEcho/Textures/Characters/NewCast/Enemy_RabbitDoll.Enemy_RabbitDoll")},
	    {TEXT("/Game/ReEcho/Textures/Characters/NewCast/Enemy_RabbitBeast.Enemy_RabbitBeast")},
	    {TEXT("/Game/ReEcho/Textures/Characters/NewCast/Enemy_GoatPriest.Enemy_GoatPriest")},
	    {TEXT("/Game/ReEcho/Textures/Characters/NewCast/Enemy_DarkPriest.Enemy_DarkPriest")},
	};
	for (const ConstructorHelpers::FObjectFinder<UTexture2D>& Finder : GruntTextureFinders)
	{
		GruntTextures.Add(Finder.Object);
	}
	static ConstructorHelpers::FObjectFinder<UTexture2D> BossTextureFinder(
	    TEXT("/Game/ReEcho/Textures/Characters/Boss2D.Boss2D"));
	BossTexture = BossTextureFinder.Object;
	Combatant = CreateDefaultSubobject<UReEchoCombatantComponent>(TEXT("Combatant"));
	Tags.Add(TEXT("ReEchoEnemy"));
}

void AReEchoEnemyActor::BeginPlay()
{
	Super::BeginPlay();
	AbilitySystem->InitAbilityActorInfo(this, this);
	Combatant->BindToAbilitySystem(AbilitySystem);
}

UAbilitySystemComponent* AReEchoEnemyActor::GetAbilitySystemComponent() const
{
	return AbilitySystem;
}

void AReEchoEnemyActor::Configure(EReEchoEnemyKind InKind, int32 SpawnIndex)
{
	Kind = InKind;
	ElementState = FReEchoElementState{};
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

	UTexture2D* CharacterTexture = bIsBoss                   ? BossTexture
	                               : GruntTextures.IsEmpty() ? nullptr
	                                                         : GruntTextures[VisualVariantIndex % GruntTextures.Num()];
	const float CharacterHalfHeight = ReEchoEnemyVisual::CharacterWorldHeight * 0.5f;
	CharacterSprite->SetVisibility(true);
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

	BaseSpriteLocation = CharacterSprite->GetRelativeLocation();
	BaseSpriteScale = CharacterSprite->GetRelativeScale3D();
}

bool AReEchoEnemyActor::IsAlive() const
{
	return Combatant && Combatant->IsAlive();
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
		CharacterSprite->SetRelativeLocation(BaseSpriteLocation);
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
		CharacterSprite->SetRelativeLocation(BaseSpriteLocation);
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
		CharacterSprite->SetRelativeLocation(BaseSpriteLocation + PreviousShakeOffset);
		CharacterSprite->SetRelativeScale3D(BaseSpriteScale * FVector(1.12f, 0.86f, 1.0f));
	}

	return true;
}

void AReEchoEnemyActor::UpdateElementAttachmentVisual()
{
	const bool bHasAttachment = ReEchoElementReaction::IsCombatElement(ElementState.Attached);
	ElementAuraRing->SetVisibility(bHasAttachment);
	ElementAttachmentLabel->SetVisibility(bHasAttachment);
	ElementAuraLight->SetVisibility(bHasAttachment);
	if (!bHasAttachment)
	{
		return;
	}

	const FLinearColor ElementColor = ReEchoElementReaction::GetElementColor(ElementState.Attached);
	ElementAuraRing->SetTextRenderColor(ElementColor.ToFColor(false));
	ElementAttachmentLabel->SetText(FText::FromString(ReEchoElementReaction::GetElementLabel(ElementState.Attached)));
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

float AReEchoEnemyActor::ReceiveElementalDamage(const float Damage,
                                                const EReEchoElement Element,
                                                const FVector& SourceLocation,
                                                AActor* SourceActor,
                                                const float ReactionEfficiency)
{
	const FReEchoElementHitResult Result =
	    ReEchoElementReaction::ResolveHit(ElementState, Element, Damage, ReactionEfficiency);
	UpdateElementAttachmentVisual();
	const FLinearColor DamageColor = Result.bTriggeredReaction ? FLinearColor(1.0f, 0.72f, 0.12f, 1.0f)
	                                                           : ReEchoElementReaction::GetElementColor(Element);
	return ReceiveGrayboxDamage(Result.Damage, SourceLocation, SourceActor, DamageColor);
}

float AReEchoEnemyActor::ReceiveGrayboxDamage(float Damage,
                                              const FVector& SourceLocation,
                                              AActor* SourceActor,
                                              const FLinearColor& DamageNumberColor)
{
	if (Kind == EReEchoEnemyKind::Shield)
	{
		const FVector ToSource = (SourceLocation - GetActorLocation()).GetSafeNormal2D();
		const FVector Forward = GetActorForwardVector().GetSafeNormal2D();
		if (FVector::DotProduct(Forward, ToSource) >= 0.f)
		{
			return 0.f;
		}
		Damage *= 2.f;
	}
	UAbilitySystemComponent* SourceAbilitySystem = nullptr;
	if (IAbilitySystemInterface* AbilitySource = Cast<IAbilitySystemInterface>(SourceActor))
	{
		SourceAbilitySystem = AbilitySource->GetAbilitySystemComponent();
	}
	const float Applied = ReEchoGameplayEffects::ApplyDamage(SourceAbilitySystem, *AbilitySystem, Damage);
	if (Applied > 0.f)
	{
		ReEchoAttackEffects::SpawnHitImpact(GetWorld(), GetActorLocation());
		AReEchoDamageNumberActor::SpawnDamageNumber(GetWorld(), GetActorLocation(), Applied, DamageNumberColor);
		StartHitReaction(SourceLocation);
	}
	if (!IsAlive())
	{
		ElementState.Attached = EReEchoElement::None;
		UpdateElementAttachmentVisual();
		SetActorEnableCollision(false);
		DeathVisualRemaining = 0.45f;
		SetLifeSpan(0.45f);
	}
	return Applied;
}

void AReEchoEnemyActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ReEchoBillboardDebug::DrawBounds(
	    this, CharacterSprite, Kind == EReEchoEnemyKind::Boss ? FColor::Yellow : FColor::Red);
	ReEchoCollisionDebug::DrawCapsule(this, Collision, Kind == EReEchoEnemyKind::Boss ? FColor::Orange : FColor::Cyan);
	VisualTime += DeltaSeconds;
	UpdateElementAttachmentFacing();
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
			if (UReEchoCombatantComponent* Target = Player->FindComponentByClass<UReEchoCombatantComponent>())
			{
				const float Applied = Target->GetBoundAbilitySystem()
				                          ? ReEchoGameplayEffects::ApplyDamage(
				                                AbilitySystem, *Target->GetBoundAbilitySystem(), ContactDamage)
				                          : Target->ApplyFinalDamage(ContactDamage);
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
}

void AReEchoEnemyActor::UpdateSpriteAnimation(const float DeltaSeconds, const bool bMoving)
{
	if (!CharacterSprite || HitReactionRemaining > 0.0f)
	{
		return;
	}
	AttackVisualRemaining = FMath::Max(0.0f, AttackVisualRemaining - DeltaSeconds);
	const float Bob = FMath::Sin(VisualTime * (bMoving ? 8.0f : 2.6f)) * (bMoving ? 3.5f : 1.5f);
	const float AttackPulse =
	    AttackVisualRemaining > 0.0f ? FMath::Sin((1.0f - AttackVisualRemaining / 0.22f) * PI) : 0.0f;
	CharacterSprite->SetRelativeLocation(BaseSpriteLocation + FVector(AttackPulse * 15.0f, 0.0f, Bob));
	CharacterSprite->SetRelativeScale3D(BaseSpriteScale *
	                                    FVector(1.0f + AttackPulse * 0.08f, 1.0f - AttackPulse * 0.04f, 1.0f));
}

void AReEchoEnemyActor::UpdateDeathAnimation(const float DeltaSeconds)
{
	DeathVisualRemaining = FMath::Max(0.0f, DeathVisualRemaining - DeltaSeconds);
	const float Ratio = DeathVisualRemaining / 0.45f;
	CharacterSprite->SetRelativeLocation(BaseSpriteLocation + FVector(0.0f, 0.0f, -28.0f * (1.0f - Ratio)));
	CharacterSprite->SetRelativeScale3D(BaseSpriteScale * FVector(Ratio, Ratio, 1.0f));
}
