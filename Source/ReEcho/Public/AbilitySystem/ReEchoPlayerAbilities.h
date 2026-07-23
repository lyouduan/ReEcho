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

	virtual bool ExecutePlayerAbility(AReEchoPlayerPawn& PlayerPawn) const
	    PURE_VIRTUAL(UReEchoPlayerGameplayAbility::ExecutePlayerAbility, return false;);
};

UCLASS()

class REECHO_API UReEchoBasicAttackAbility : public UReEchoPlayerGameplayAbility
{
	GENERATED_BODY()

protected:
	virtual bool ExecutePlayerAbility(AReEchoPlayerPawn& PlayerPawn) const override;
};

UCLASS()

class REECHO_API UReEchoActiveAttackAbility : public UReEchoPlayerGameplayAbility
{
	GENERATED_BODY()

protected:
	virtual bool ExecutePlayerAbility(AReEchoPlayerPawn& PlayerPawn) const override;
};

UCLASS()

class REECHO_API UReEchoSelectWeaponSlot1Ability : public UReEchoPlayerGameplayAbility
{
	GENERATED_BODY()

protected:
	virtual bool ExecutePlayerAbility(AReEchoPlayerPawn& PlayerPawn) const override;
};

UCLASS()

class REECHO_API UReEchoSelectWeaponSlot2Ability : public UReEchoPlayerGameplayAbility
{
	GENERATED_BODY()

protected:
	virtual bool ExecutePlayerAbility(AReEchoPlayerPawn& PlayerPawn) const override;
};

UCLASS()

class REECHO_API UReEchoSelectWeaponSlot3Ability : public UReEchoPlayerGameplayAbility
{
	GENERATED_BODY()

protected:
	virtual bool ExecutePlayerAbility(AReEchoPlayerPawn& PlayerPawn) const override;
};