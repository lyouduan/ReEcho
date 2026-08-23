#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "Combat/ReEchoCombatTypes.h"
#include "ReEchoGameplayEffects.generated.h"

class UAbilitySystemComponent;

UCLASS()

class REECHOCOMBAT_API UReEchoInitializeStatsEffect : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UReEchoInitializeStatsEffect();
};

UCLASS()

class REECHOCOMBAT_API UReEchoDamageEffect : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UReEchoDamageEffect();
};

UCLASS()

class REECHOCOMBAT_API UReEchoHealEffect : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UReEchoHealEffect();
};

UCLASS()

class REECHOCOMBAT_API UReEchoTransientStatEffect : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UReEchoTransientStatEffect();
};

UCLASS()

class REECHOCOMBAT_API UReEchoBasicAttackCooldownEffect : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UReEchoBasicAttackCooldownEffect(const FObjectInitializer& ObjectInitializer);
};

UCLASS()

class REECHOCOMBAT_API UReEchoActiveAttackCooldownEffect : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UReEchoActiveAttackCooldownEffect(const FObjectInitializer& ObjectInitializer);
};

namespace ReEchoGameplayEffects
{
REECHOCOMBAT_API bool
ApplyInitialization(UAbilitySystemComponent& Target, const FReEchoStatBlock& Stats, bool bFillHealth = true);
REECHOCOMBAT_API float ApplyDamage(UAbilitySystemComponent* Source, UAbilitySystemComponent& Target, float Damage);
REECHOCOMBAT_API float ApplyHealing(UAbilitySystemComponent* Source, UAbilitySystemComponent& Target, float Healing);
REECHOCOMBAT_API FActiveGameplayEffectHandle ApplyTransientStatMultiplier(UAbilitySystemComponent& Target,
                                                                          float AttackSpeedMultiplier,
                                                                          float MovementSpeedMultiplier);
}
