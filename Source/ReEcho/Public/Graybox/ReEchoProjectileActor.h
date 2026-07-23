#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ReEchoProjectileActor.generated.h"

class USphereComponent;
class UStaticMeshComponent;

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
	                          const FLinearColor& Color);

private:
	UPROPERTY()
	TObjectPtr<USphereComponent> Collision;
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Shape;
	FVector Velocity = FVector::ZeroVector;
	FVector DamageSource = FVector::ZeroVector;
	float Damage = 1.f;
	float Speed = 950.f;
};

