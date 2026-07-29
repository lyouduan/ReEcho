#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ReEchoStaffLightWaveActor.generated.h"

class USphereComponent;
class UStaticMeshComponent;

/** 法杖发射的2D光波：沿瞄准方向飞行，并用连续路径检测命中第一个敌人。 */
UCLASS()
class REECHO_API AReEchoStaffLightWaveActor : public AActor
{
	GENERATED_BODY()

public:
	AReEchoStaffLightWaveActor();
	virtual void Tick(float DeltaSeconds) override;
	void InitializeWave(const FVector& Direction, float InDamage, const FVector& InDamageSource, float InRange);

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> Collision;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> WaveVisual;

	FVector Velocity = FVector::ZeroVector;
	FVector DamageSource = FVector::ZeroVector;
	FVector SpawnLocation = FVector::ZeroVector;
	float Damage = 0.0f;
	float MaximumRange = 260.0f;
	float Speed = 760.0f;
};