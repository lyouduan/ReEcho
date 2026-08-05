#pragma once

#include "CoreMinimal.h"
#include "Core/ReEchoTypes.h"
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
	                          float InReactionEfficiency = 1.0f);

private:
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
};
