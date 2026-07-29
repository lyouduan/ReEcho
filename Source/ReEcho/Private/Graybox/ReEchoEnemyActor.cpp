#include "Graybox/ReEchoEnemyActor.h"

#include "Graybox/ReEchoAttackEffects.h"
#include "Graybox/ReEchoHealthBarActor.h"
#include "Graybox/ReEchoBillboardDebug.h"
#include "Graybox/ReEchoCollisionDebug.h"
#include "UI/ReEchoDamageNumberActor.h"
#include "Combat/ReEchoCombatantComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
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

void AReEchoEnemyActor::Configure(EReEchoEnemyKind InKind, int32 SpawnIndex)
{
	Kind = InKind;
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
			ContactDamage = 22.f;
			FuseRemaining = 2.2f;
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

	UTexture2D* CharacterTexture = bIsBoss ? BossTexture
	                                        : GruntTextures.IsEmpty()
	                                            ? nullptr
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

float AReEchoEnemyActor::ReceiveGrayboxDamage(float Damage, const FVector& SourceLocation)
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
	const float Applied = Combatant->ApplyFinalDamage(Damage);
	if (Applied > 0.f)
	{
		ReEchoAttackEffects::SpawnHitImpact(GetWorld(), GetActorLocation());
		AReEchoDamageNumberActor::SpawnDamageNumber(
		    GetWorld(), GetActorLocation(), Applied, FLinearColor(1.0f, 1.0f, 1.0f));
		StartHitReaction(SourceLocation);
	}
	if (!IsAlive())
	{
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
	if (Kind == EReEchoEnemyKind::Bomber)
	{
		FuseRemaining -= DeltaSeconds;
	}
	if ((Distance <= 85.f && AttackCooldown <= 0.f) || (Kind == EReEchoEnemyKind::Bomber && FuseRemaining <= 0.f))
	{
		StartAttackVisual();
		if (UReEchoCombatantComponent* Target = Player->FindComponentByClass<UReEchoCombatantComponent>())
		{
			const float Applied = Target->ApplyFinalDamage(ContactDamage);
			if (Applied > 0.f)
			{
				if (AReEchoPlayerPawn* ReEchoPlayer = Cast<AReEchoPlayerPawn>(Player))
				{
					ReEchoPlayer->PlayHitVisual();
				}
				ReEchoAttackEffects::SpawnHitImpact(GetWorld(), Player->GetActorLocation());
				AReEchoDamageNumberActor::SpawnDamageNumber(
				    GetWorld(), Player->GetActorLocation(), Applied, FLinearColor(1.0f, 1.0f, 1.0f));
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
