#pragma once

#include "CoreMinimal.h"
#include "Core/ReEchoTypes.h"
#include "GameFramework/Actor.h"
#include "ReEchoProjectileActor.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class AReEchoEnemyActor;

UCLASS()

class REECHO_API AReEchoProjectileActor : public AActor
{
	GENERATED_BODY()
public:
	AReEchoProjectileActor();
	virtual void Tick(float DeltaSeconds) override;
	void InitializeProjectile(const FVector& Direction,
	                          float InDamage,
	                          const FVector& InDamageSource,
	                          const FLinearColor& Color,
	                          EReEchoElement InElement = EReEchoElement::None,
	                          float InReactionEfficiency = 1.0f,
	                          float InExplosionRadiusCm = 0.0f,
	                          float InMaxRangeCm = 0.0f);

	float GetDamage() const
	{
		return Damage;
	}

	EReEchoElement GetElement() const
	{
		return Element;
	}

	float GetExplosionRadiusCm() const
	{
		return ExplosionRadiusCm;
	}

	FVector GetVelocity() const
	{
		return Velocity;
	}

private:
	void ApplyDamageAtLocation(const FVector& ImpactLocation, AReEchoEnemyActor* DirectTarget = nullptr);

	UPROPERTY()
	TObjectPtr<USphereComponent> Collision;
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Shape;
	UPROPERTY()
	TObjectPtr<UTextRenderComponent> ElementLabel;
	FVector Velocity = FVector::ZeroVector;
	FVector DamageSource = FVector::ZeroVector;
	float Damage = 1.f;
	EReEchoElement Element = EReEchoElement::None;
	float ReactionEfficiency = 1.0f;
	float Speed = 950.f;
	float ExplosionRadiusCm = 0.0f;
	float MaxRangeCm = 0.0f;
	float TravelledCm = 0.0f;
};
