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
REECHOCOMBAT_API EReEchoDamageSource ResolveActorDamageSource(const AActor* Actor,
                                                              EReEchoDamageSource Fallback);
REECHOCOMBAT_API bool CanDamage(EReEchoCombatFaction SourceFaction,
                                EReEchoCombatFaction TargetFaction,
                                bool bAllowSameFactionDamage = false);
REECHOCOMBAT_API bool CanDamage(const FReEchoAttackIdentity& Attack,
                                const AActor& Target,
                                bool bAllowSameFactionDamage = false);
}
