#pragma once

#include "CoreMinimal.h"
#include "Combat/ReEchoCombatContracts.h"
#include "Weapons/ReEchoProjectileLogicComponent.h"
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
	void InitializeWave(const FVector& Direction,
	                    float InDamage,
	                    const FVector& InDamageSource,
	                    float InRange,
	                    FReEchoAttackIdentity InAttack = {},
	                    EReEchoDamageSource InDamageSourceType = EReEchoDamageSource::Player);

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> Collision;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> WaveVisual;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UReEchoProjectileLogicComponent> ProjectileLogic;
	float Speed = 760.0f;
};
