#include "AbilitySystem/ReEchoCombatAttributeSet.h"

#include "GameplayEffectExtension.h"

UReEchoCombatAttributeSet::UReEchoCombatAttributeSet()
{
	InitHealth(100.0f);
	InitMaxHealth(100.0f);
	InitBlock(0.0f);
	InitPhysicalAttack(10.0f);
	InitElementalAttack(10.0f);
	InitAttackSpeed(1.0f);
	InitMovementSpeed(1.0f);
	InitEchoEfficiency(1.0f);
	InitIncomingDamage(0.0f);
	InitIncomingHealing(0.0f);
}

void UReEchoCombatAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max(1.0f, NewValue);
	}
	else if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	else if (Attribute == GetBlockAttribute())
	{
		NewValue = FMath::Max(0.0f, NewValue);
	}
	else if (Attribute == GetAttackSpeedAttribute() || Attribute == GetMovementSpeedAttribute() ||
	         Attribute == GetEchoEfficiencyAttribute())
	{
		NewValue = FMath::Max(0.1f, NewValue);
	}
}

void UReEchoCombatAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		const float Damage = FMath::Max(0.0f, GetIncomingDamage());
		SetIncomingDamage(0.0f);
		if (Damage > 0.0f)
		{
			if (GetBlock() > 0.0f)
			{
				SetBlock(FMath::Max(0.0f, GetBlock() - 1.0f));
			}
			else
			{
				SetHealth(FMath::Clamp(GetHealth() - Damage, 0.0f, GetMaxHealth()));
			}
		}
	}
	else if (Data.EvaluatedData.Attribute == GetIncomingHealingAttribute())
	{
		const float Healing = FMath::Max(0.0f, GetIncomingHealing());
		SetIncomingHealing(0.0f);
		SetHealth(FMath::Clamp(GetHealth() + Healing, 0.0f, GetMaxHealth()));
	}
	else if (Data.EvaluatedData.Attribute == GetMaxHealthAttribute())
	{
		SetMaxHealth(FMath::Max(1.0f, GetMaxHealth()));
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
	}
	else if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
	}
	else if (Data.EvaluatedData.Attribute == GetBlockAttribute())
	{
		SetBlock(FMath::Max(0.0f, GetBlock()));
	}
}