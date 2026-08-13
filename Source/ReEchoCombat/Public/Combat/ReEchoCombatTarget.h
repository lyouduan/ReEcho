#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ReEchoCombatTarget.generated.h"

UINTERFACE(MinimalAPI)

class UReEchoCombatTarget : public UInterface
{
	GENERATED_BODY()
};

class REECHOCOMBAT_API IReEchoCombatTarget
{
	GENERATED_BODY()

public:
	virtual bool IsCombatTargetAlive() const = 0;
	virtual FVector GetCombatTargetLocation() const = 0;
	virtual int32 GetCombatTargetTieBreakIndex() const = 0;
};

UCLASS(ClassGroup = (ReEcho), meta = (BlueprintSpawnableComponent))

class REECHOCOMBAT_API UReEchoTargetingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	AActor* FindNearestTarget(float RangeCm);

	AActor* GetCurrentTarget() const
	{
		return CurrentTarget.Get();
	}

	void ClearTarget()
	{
		CurrentTarget.Reset();
	}

private:
	TWeakObjectPtr<AActor> CurrentTarget;
};
