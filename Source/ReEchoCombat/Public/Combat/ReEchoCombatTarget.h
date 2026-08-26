#pragma once

#include "CoreMinimal.h"
#include "Combat/ReEchoCombatTypes.h"
#include "Components/ActorComponent.h"
#include "UObject/Interface.h"
#include "ReEchoCombatTarget.generated.h"

UINTERFACE(MinimalAPI)

class UReEchoCombatAffiliation : public UInterface
{
	GENERATED_BODY()
};

class REECHOCOMBAT_API IReEchoCombatAffiliation
{
	GENERATED_BODY()

public:
	virtual EReEchoCombatFaction GetCombatFaction() const = 0;
	virtual EReEchoDamageSource GetCombatDamageSource() const = 0;
};

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

	/** Optional stable data identity used by source-side rules; player/Echo targets may return None. */
	virtual FName GetCombatTargetDefinitionId() const
	{
		return NAME_None;
	}

	/** Optional source-side transformation. Implementations may mutate only the supplied candidate intent. */
	virtual void ModifyOutgoingHit(FReEchoHitIntent& Intent) const
	{
	}

	virtual void NotifyReactionResolved(FName ReactionId) const
	{
	}

	/** Source-side reaction rule query. It must be read-only and return a non-negative final multiplier. */
	virtual float GetReactionDamageMultiplier(FName ReactionId) const
	{
		return 1.0f;
	}

	/** Source-side Burn rule query; Combat remains the owner of duration and stack application. */
	virtual bool HasInfiniteStackingBurn() const
	{
		return false;
	}

	virtual void NotifyKillResolved(FName TargetDefinitionId) const
	{
	}

	/** Source-side observation of Combat's immutable final result. Gameplay must not mutate the result here. */
	virtual void NotifyHitResolved(const FReEchoHitResolved& Result) const
	{
	}

	virtual void NotifyNegativeStatusApplied(FName StatusId) const
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

namespace ReEchoCombatRelations
{
REECHOCOMBAT_API EReEchoCombatFaction ResolveActorFaction(const AActor* Actor);
REECHOCOMBAT_API EReEchoDamageSource ResolveActorDamageSource(const AActor* Actor, EReEchoDamageSource Fallback);
REECHOCOMBAT_API bool
CanDamage(EReEchoCombatFaction SourceFaction, EReEchoCombatFaction TargetFaction, bool bAllowSameFactionDamage = false);
REECHOCOMBAT_API bool
CanDamage(const FReEchoAttackIdentity& Attack, const AActor& Target, bool bAllowSameFactionDamage = false);
}
