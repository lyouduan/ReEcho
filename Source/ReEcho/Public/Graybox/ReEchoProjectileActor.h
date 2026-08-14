#pragma once

#include "CoreMinimal.h"
#include "Core/ReEchoTypes.h"
#include "Combat/ReEchoCombatContracts.h"
#include "Weapons/ReEchoProjectileLogicComponent.h"
#include "GameFramework/Actor.h"
#include "ReEchoProjectileActor.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

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
	                          float InMaxRangeCm = 0.0f,
	                          FReEchoAttackIdentity InAttack = {});

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
		return ProjectileLogic ? ProjectileLogic->GetSnapshot().Velocity : FVector::ZeroVector;
	}

private:
	UPROPERTY()
	TObjectPtr<USphereComponent> Collision;
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Shape;
	UPROPERTY()
	TObjectPtr<UTextRenderComponent> ElementLabel;
	UPROPERTY()
	TObjectPtr<UReEchoProjectileLogicComponent> ProjectileLogic;
	float Damage = 1.f;
	EReEchoElement Element = EReEchoElement::None;
	float Speed = 950.f;
	float ExplosionRadiusCm = 0.0f;
};
