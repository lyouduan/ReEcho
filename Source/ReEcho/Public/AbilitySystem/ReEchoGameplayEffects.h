#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "Core/ReEchoTypes.h"
#include "ReEchoGameplayEffects.generated.h"

class UAbilitySystemComponent;

UCLASS()

class REECHO_API UReEchoInitializeStatsEffect : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UReEchoInitializeStatsEffect();
};

UCLASS()

class REECHO_API UReEchoDamageEffect : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UReEchoDamageEffect();
};

UCLASS()

class REECHO_API UReEchoHealEffect : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UReEchoHealEffect();
};

UCLASS()

class REECHO_API UReEchoBasicAttackCooldownEffect : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UReEchoBasicAttackCooldownEffect(const FObjectInitializer& ObjectInitializer);
};

UCLASS()

class REECHO_API UReEchoActiveAttackCooldownEffect : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UReEchoActiveAttackCooldownEffect(const FObjectInitializer& ObjectInitializer);
};

namespace ReEchoGameplayEffects
{
REECHO_API bool
ApplyInitialization(UAbilitySystemComponent& Target, const FReEchoStatBlock& Stats, bool bFillHealth = true);
REECHO_API float ApplyDamage(UAbilitySystemComponent* Source, UAbilitySystemComponent& Target, float Damage);
REECHO_API float ApplyHealing(UAbilitySystemComponent* Source, UAbilitySystemComponent& Target, float Healing);
}