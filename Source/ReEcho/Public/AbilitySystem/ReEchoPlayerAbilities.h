#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "ReEchoPlayerAbilities.generated.h"

class AReEchoPlayerPawn;

UCLASS(Abstract)

class REECHO_API UReEchoPlayerGameplayAbility : public UGameplayAbility
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

	virtual bool ExecutePlayerAbility(AReEchoPlayerPawn& PlayerPawn) const
	    PURE_VIRTUAL(UReEchoPlayerGameplayAbility::ExecutePlayerAbility, return false;);
	virtual float GetCooldownDuration(const AReEchoPlayerPawn& PlayerPawn) const;

	TSubclassOf<UGameplayEffect> CooldownEffectClass;
	FGameplayTagContainer CooldownTags;
};

UCLASS()

class REECHO_API UReEchoBasicAttackAbility : public UReEchoPlayerGameplayAbility
{
	GENERATED_BODY()

public:
	UReEchoBasicAttackAbility();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	                             const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo,
	                             const FGameplayEventData* TriggerEventData) override;
	virtual bool ExecutePlayerAbility(AReEchoPlayerPawn& PlayerPawn) const override;
	virtual float GetCooldownDuration(const AReEchoPlayerPawn& PlayerPawn) const override;

private:
	bool CommitAndExecuteCurrentAttack();
	void ScheduleNextAttack(float Delay);

	UFUNCTION()
	void HandleRepeatDelay();
	UFUNCTION()
	void HandleInputReleased(float TimeHeld);
};

UCLASS()

class REECHO_API UReEchoActiveAttackAbility : public UReEchoPlayerGameplayAbility
{
	GENERATED_BODY()

public:
	UReEchoActiveAttackAbility();

protected:
	virtual bool ExecutePlayerAbility(AReEchoPlayerPawn& PlayerPawn) const override;
	virtual float GetCooldownDuration(const AReEchoPlayerPawn& PlayerPawn) const override;
};
