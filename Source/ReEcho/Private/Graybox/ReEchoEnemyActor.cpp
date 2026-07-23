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
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AReEchoEnemyActor::AReEchoEnemyActor()
{
	PrimaryActorTick.bCanEverTick = true;
	Collision = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->InitCapsuleSize(36.0f, 110.0f);
	Collision->SetCollisionProfileName(TEXT("Pawn"));
	Shape = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Shape"));
	Shape->SetupAttachment(RootComponent);
	Shape->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GroundShadow = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GroundShadow"));
	GroundShadow->SetupAttachment(RootComponent);
	GroundShadow->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GroundShadow->SetCastShadow(false);
	GroundShadow->SetTranslucentSortPriority(-1);
	GroundShadow->SetAbsolute(false, false, true);
	GroundShadow->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")));
	GroundShadow->SetRelativeLocation(FVector(0.0f, 0.0f, -49.0f));
	GroundShadow->SetWorldScale3D(FVector(0.50f, 0.31f, 1.0f));
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
	static ConstructorHelpers::FObjectFinder<UTexture2D> GruntTextureFinder(
	    TEXT("/Game/ReEcho/Textures/Characters/Grunt2D.Grunt2D"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> BossTextureFinder(
	    TEXT("/Game/ReEcho/Textures/Characters/Boss2D.Boss2D"));
	GruntTexture = GruntTextureFinder.Object;
	BossTexture = BossTextureFinder.Object;
	Combatant = CreateDefaultSubobject<UReEchoCombatantComponent>(TEXT("Combatant"));
	Tags.Add(TEXT("ReEchoEnemy"));
}

void AReEchoEnemyActor::Configure(EReEchoEnemyKind InKind, int32 SpawnIndex)
{
	Kind = InKind;
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
		                      Kind == EReEchoEnemyKind::Boss ? 105.f : 68.f,
		                      Kind == EReEchoEnemyKind::Boss ? 1.25f : 0.75f,
		                      CharacterSprite);
	}
}

void AReEchoEnemyActor::ApplyVisual()
{
	const TCHAR* MeshPath = TEXT("/Engine/BasicShapes/Cube.Cube");
	FVector Scale(0.65f);
	FLinearColor Color(1.f, 0.15f, 0.1f);
	if (Kind == EReEchoEnemyKind::Shield)
	{
		MeshPath = TEXT("/Engine/BasicShapes/Plane.Plane");
		Scale = FVector(0.8f, 0.8f, 1.1f);
		Color = FLinearColor(0.15f, 0.35f, 1.f);
	}
	if (Kind == EReEchoEnemyKind::Bomber)
	{
		MeshPath = TEXT("/Engine/BasicShapes/Cone.Cone");
		Scale = FVector(0.55f);
		Color = FLinearColor(1.f, 0.55f, 0.05f);
	}
	if (Kind == EReEchoEnemyKind::Boss)
	{
		MeshPath = TEXT("/Engine/BasicShapes/Cube.Cube");
		Scale = FVector(1.8f);
		Color = FLinearColor(0.45f, 0.02f, 0.02f);
	}
	Shape->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, MeshPath));
	Shape->SetRelativeScale3D(Scale);
	const float CollisionRadius = Kind == EReEchoEnemyKind::Boss     ? 65.0f
	                              : Kind == EReEchoEnemyKind::Shield ? 44.0f
	                              : Kind == EReEchoEnemyKind::Bomber ? 30.0f
	                                                                 : 36.0f;
	const float CollisionHalfHeight = Kind == EReEchoEnemyKind::Boss     ? 140.0f
	                                  : Kind == EReEchoEnemyKind::Shield ? 80.0f
	                                  : Kind == EReEchoEnemyKind::Bomber ? 65.0f
	                                                                     : 127.5f;
	Collision->SetCapsuleSize(CollisionRadius, CollisionHalfHeight);
	GroundShadow->SetRelativeLocation(FVector(0.0f, 0.0f, -49.0f));
	GroundShadow->SetWorldScale3D(Kind == EReEchoEnemyKind::Boss ? FVector(0.60f, 0.38f, 1.0f)
	                                                             : FVector(0.34f, 0.22f, 1.0f));

	const bool bUseCharacterSprite = Kind == EReEchoEnemyKind::Grunt || Kind == EReEchoEnemyKind::Boss;
	Shape->SetVisibility(!bUseCharacterSprite);
	CharacterSprite->SetVisibility(bUseCharacterSprite);
	if (bUseCharacterSprite)
	{
		const bool bIsBoss = Kind == EReEchoEnemyKind::Boss;
		UTexture2D* CharacterTexture = bIsBoss ? BossTexture : GruntTexture;
		const float CharacterWorldHeight = bIsBoss ? 280.0f : 255.0f;
		const float CharacterHalfHeight = CharacterWorldHeight * 0.5f;
		CharacterSprite->SetRelativeLocation(FVector::ZeroVector);
		GroundShadow->SetRelativeLocation(FVector(0.0f, 0.0f, -CharacterHalfHeight));
		FVector CenteredLocation = GetActorLocation();
		CenteredLocation.Z += CharacterHalfHeight;
		SetActorLocation(CenteredLocation, false, nullptr, ETeleportType::TeleportPhysics);
		if (CharacterTexture)
		{
			CharacterSprite->SetSprite(CharacterTexture);
			const float TextureScale = CharacterWorldHeight / FMath::Max(1, CharacterTexture->GetSizeY());
			CharacterSprite->SetWorldScale3D(FVector(TextureScale));
		}
	}
	if (UMaterialInterface* Base =
	        LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
	{
		UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Base, this);
		Material->SetVectorParameterValue(TEXT("Color"), Color);
		Shape->SetMaterial(0, Material);
	}
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
		AddActorWorldOffset(-PreviousShakeOffset, false);
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
		AddActorWorldOffset(-PreviousShakeOffset, false);
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
		AddActorWorldOffset(PreviousShakeOffset, false);
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
		    GetWorld(), GetActorLocation(), Applied, FLinearColor(1.0f, 0.25f, 0.08f));
		StartHitReaction(SourceLocation);
	}
	if (!IsAlive())
	{
		SetActorEnableCollision(false);
		SetLifeSpan(0.15f);
	}
	return Applied;
}

void AReEchoEnemyActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ReEchoBillboardDebug::DrawBounds(
	    this, CharacterSprite, Kind == EReEchoEnemyKind::Boss ? FColor::Yellow : FColor::Red);
	ReEchoCollisionDebug::DrawCapsule(this, Collision, Kind == EReEchoEnemyKind::Boss ? FColor::Orange : FColor::Cyan);
	if (UpdateHitReaction(DeltaSeconds))
	{
		return;
	}

	if (!IsAlive())
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
		if (UReEchoCombatantComponent* Target = Player->FindComponentByClass<UReEchoCombatantComponent>())
		{
			const float Applied = Target->ApplyFinalDamage(ContactDamage);
			if (Applied > 0.f)
			{
				ReEchoAttackEffects::SpawnHitImpact(GetWorld(), Player->GetActorLocation());
				AReEchoDamageNumberActor::SpawnDamageNumber(
				    GetWorld(), Player->GetActorLocation(), Applied, FLinearColor(1.0f, 0.05f, 0.05f));
			}
		}
		AttackCooldown = AttackInterval;
		if (Kind == EReEchoEnemyKind::Bomber)
		{
			ReceiveGrayboxDamage(9999.f, GetActorLocation());
		}
	}
}
