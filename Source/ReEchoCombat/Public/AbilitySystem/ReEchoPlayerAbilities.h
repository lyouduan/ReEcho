#pragma once

#include "Abilities/GameplayAbility.h"
#include "CoreMinimal.h"
#include "ReEchoPlayerAbilities.generated.h"

class IReEchoAttackHost;

UCLASS(Abstract)

class REECHOCOMBAT_API UReEchoPlayerGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UReEchoPlayerGameplayAbility();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	                             const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo,
	                             const FGameplayEventData* TriggerEventData) override;
	virtual const FGameplayTagContainer* GetCooldownTags() const override;
	virtual UGameplayEffect* GetCooldownGameplayEffect() const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle,
	                           const FGameplayAbilityActorInfo* ActorInfo,
	                           const FGameplayAbilityActivationInfo ActivationInfo) const override;

	virtual bool ExecuteHostAbility(IReEchoAttackHost& Host) const
	    PURE_VIRTUAL(UReEchoPlayerGameplayAbility::ExecuteHostAbility, return false;);
	virtual float GetCooldownDuration(const IReEchoAttackHost& Host) const;
	IReEchoAttackHost* ResolveHost() const;

	TSubclassOf<UGameplayEffect> CooldownEffectClass;
	FGameplayTagContainer CooldownTags;
};

UCLASS()

class REECHOCOMBAT_API UReEchoBasicAttackAbility : public UReEchoPlayerGameplayAbility
{
	GENERATED_BODY()

public:
	UReEchoBasicAttackAbility();
#if WITH_DEV_AUTOMATION_TESTS
	void TriggerHeldRepeatForTesting()
	{
		HandleRepeatDelay();
	}
#endif

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	                             const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo,
	                             const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
	                        const FGameplayAbilityActorInfo* ActorInfo,
	                        const FGameplayAbilityActivationInfo ActivationInfo,
	                        bool bReplicateEndAbility,
	                        bool bWasCancelled) override;
	virtual bool ExecuteHostAbility(IReEchoAttackHost& Host) const override;

private:
	void AttemptOrWait();
	void ScheduleNextAttack(float Delay);

	UFUNCTION() void HandleRepeatDelay();
	UFUNCTION() void HandleInputReleased(float TimeHeld);
	FTimerHandle RepeatTimerHandle;
};

UCLASS()

class REECHOCOMBAT_API UReEchoActiveAttackAbility : public UReEchoPlayerGameplayAbility
{
	GENERATED_BODY()

public:
	UReEchoActiveAttackAbility();

protected:
	virtual bool ExecuteHostAbility(IReEchoAttackHost& Host) const override;
	virtual float GetCooldownDuration(const IReEchoAttackHost& Host) const override;
};
