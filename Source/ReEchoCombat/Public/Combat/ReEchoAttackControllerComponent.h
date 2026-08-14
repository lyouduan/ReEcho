#pragma once

#include "Combat/ReEchoCombatContracts.h"
#include "Components/ActorComponent.h"
#include "UObject/Interface.h"
#include "ReEchoAttackControllerComponent.generated.h"

class UReEchoTargetingComponent;

UINTERFACE(MinimalAPI)

class UReEchoAttackControllerHost : public UInterface
{
	GENERATED_BODY()
};

class REECHOCOMBAT_API IReEchoAttackControllerHost
{
	GENERATED_BODY()

public:
	virtual bool CanIssueAttackRequest() const = 0;
	virtual float GetAutomaticAttackRange() const = 0;
	virtual void FaceAutomaticTarget(AActor& Target) = 0;
	virtual void PressBasicAttackInput() = 0;
	virtual void ReleaseBasicAttackInput() = 0;
	virtual FName GetAttackWeaponId() const = 0;
	virtual FName GetAttackStepId() const = 0;
	virtual float GetAttackReadinessRemaining() const = 0;
	virtual float GetEffectiveAttackSpeed() const = 0;
};

UCLASS(ClassGroup = (ReEcho), meta = (BlueprintSpawnableComponent))

class REECHOCOMBAT_API UReEchoAttackControllerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UReEchoAttackControllerComponent();

	void SetAttackMode(EReEchoAttackMode NewMode);

	EReEchoAttackMode GetAttackMode() const
	{
		return Mode;
	}

	void BeginManualAttack();
	void EndManualAttack();
	void ReleaseAttackRequests();
	void UpdateAutomaticAttack();
	FReEchoAttackSnapshot GetSnapshot() const;

	bool IsManualHeld() const
	{
		return bManualHeld;
	}

	bool IsAutomaticHeld() const
	{
		return bAutomaticHeld;
	}

private:
	IReEchoAttackControllerHost* ResolveHost() const;
	void ReleaseAutomaticAttack();

	EReEchoAttackMode Mode = EReEchoAttackMode::Automatic;
	bool bManualHeld = false;
	bool bAutomaticHeld = false;
	TWeakObjectPtr<AActor> CurrentTarget;
};
