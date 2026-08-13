#pragma once

#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "CoreMinimal.h"
#include "ReEchoCombatAttributeSet.generated.h"

#define REECHO_ATTRIBUTE_ACCESSORS(ClassName, PropertyName)                                                            \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName)                                                         \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName)                                                                       \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName)                                                                       \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()

class REECHOCOMBAT_API UReEchoCombatAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UReEchoCombatAttributeSet();

	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FGameplayAttributeData Health;
	REECHO_ATTRIBUTE_ACCESSORS(UReEchoCombatAttributeSet, Health);

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FGameplayAttributeData MaxHealth;
	REECHO_ATTRIBUTE_ACCESSORS(UReEchoCombatAttributeSet, MaxHealth);

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FGameplayAttributeData Block;
	REECHO_ATTRIBUTE_ACCESSORS(UReEchoCombatAttributeSet, Block);

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FGameplayAttributeData PhysicalAttack;
	REECHO_ATTRIBUTE_ACCESSORS(UReEchoCombatAttributeSet, PhysicalAttack);

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FGameplayAttributeData ElementalAttack;
	REECHO_ATTRIBUTE_ACCESSORS(UReEchoCombatAttributeSet, ElementalAttack);

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FGameplayAttributeData AttackSpeed;
	REECHO_ATTRIBUTE_ACCESSORS(UReEchoCombatAttributeSet, AttackSpeed);

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FGameplayAttributeData MovementSpeed;
	REECHO_ATTRIBUTE_ACCESSORS(UReEchoCombatAttributeSet, MovementSpeed);

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FGameplayAttributeData EchoEfficiency;
	REECHO_ATTRIBUTE_ACCESSORS(UReEchoCombatAttributeSet, EchoEfficiency);

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Meta")
	FGameplayAttributeData IncomingDamage;
	REECHO_ATTRIBUTE_ACCESSORS(UReEchoCombatAttributeSet, IncomingDamage);

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Meta")
	FGameplayAttributeData IncomingHealing;
	REECHO_ATTRIBUTE_ACCESSORS(UReEchoCombatAttributeSet, IncomingHealing);
};

#undef REECHO_ATTRIBUTE_ACCESSORS