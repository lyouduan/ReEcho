#pragma once

#include "CoreMinimal.h"
#include "Combat/ReEchoCombatTypes.h"
#include "Components/ActorComponent.h"
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
	virtual class UReEchoCombatantComponent* GetCombatTargetCombatant() const = 0;
	virtual bool IntersectsCombatPath(const FVector& PathStart, const FVector& PathEnd, float CarrierRadius) const = 0;

	/** Optional source-side transformation. Implementations may mutate only the supplied candidate intent. */
	virtual void ModifyOutgoingHit(FReEchoHitIntent& Intent) const
	{
	}

	virtual void NotifyReactionResolved(FName ReactionId) const
	{
	}

	virtual void NotifyKillResolved() const
	{
	}

	virtual void NotifyDefeated(EReEchoDamageSource DamageSource) const
	{
	}

	/** Target-specific defense profile hook; Combat still owns applying and publishing the final result. */
	virtual float ModifyIncomingRawDamage(const FReEchoHitIntent& Intent) const
	{
		return Intent.RawDamage;
	}
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
