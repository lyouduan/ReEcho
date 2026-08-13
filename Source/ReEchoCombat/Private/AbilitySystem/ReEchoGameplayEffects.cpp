#include "AbilitySystem/ReEchoGameplayEffects.h"

#include "AbilitySystem/ReEchoCombatAttributeSet.h"
#include "AbilitySystem/ReEchoGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

namespace
{
void AddSetByCallerModifier(UGameplayEffect& Effect,
                            const FGameplayAttribute& Attribute,
                            const EGameplayModOp::Type Operation,
                            const FGameplayTag DataTag)
{
	FSetByCallerFloat CallerMagnitude;
	CallerMagnitude.DataTag = DataTag;
	FGameplayModifierInfo& Modifier = Effect.Modifiers.AddDefaulted_GetRef();
	Modifier.Attribute = Attribute;
	Modifier.ModifierOp = Operation;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(CallerMagnitude);
}

void ConfigureCooldownDuration(UGameplayEffect& Effect)
{
	Effect.DurationPolicy = EGameplayEffectDurationType::HasDuration;
	FSetByCallerFloat Duration;
	Duration.DataTag = ReEchoGameplayTags::Data_Cooldown;
	Effect.DurationMagnitude = FGameplayEffectModifierMagnitude(Duration);
}

FGameplayEffectSpecHandle MakeSpec(UAbilitySystemComponent& Source, const TSubclassOf<UGameplayEffect> EffectClass)
{
	return Source.MakeOutgoingSpec(EffectClass, 1.0f, Source.MakeEffectContext());
}
}

UReEchoInitializeStatsEffect::UReEchoInitializeStatsEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	AddSetByCallerModifier(*this,
	                       UReEchoCombatAttributeSet::GetMaxHealthAttribute(),
	                       EGameplayModOp::Override,
	                       ReEchoGameplayTags::Data_MaxHealth);
	AddSetByCallerModifier(*this,
	                       UReEchoCombatAttributeSet::GetHealthAttribute(),
	                       EGameplayModOp::Override,
	                       ReEchoGameplayTags::Data_Health);
	AddSetByCallerModifier(*this,
	                       UReEchoCombatAttributeSet::GetBlockAttribute(),
	                       EGameplayModOp::Override,
	                       ReEchoGameplayTags::Data_Block);
	AddSetByCallerModifier(*this,
	                       UReEchoCombatAttributeSet::GetPhysicalAttackAttribute(),
	                       EGameplayModOp::Override,
	                       ReEchoGameplayTags::Data_PhysicalAttack);
	AddSetByCallerModifier(*this,
	                       UReEchoCombatAttributeSet::GetElementalAttackAttribute(),
	                       EGameplayModOp::Override,
	                       ReEchoGameplayTags::Data_ElementalAttack);
	AddSetByCallerModifier(*this,
	                       UReEchoCombatAttributeSet::GetAttackSpeedAttribute(),
	                       EGameplayModOp::Override,
	                       ReEchoGameplayTags::Data_AttackSpeed);
	AddSetByCallerModifier(*this,
	                       UReEchoCombatAttributeSet::GetMovementSpeedAttribute(),
	                       EGameplayModOp::Override,
	                       ReEchoGameplayTags::Data_MovementSpeed);
	AddSetByCallerModifier(*this,
	                       UReEchoCombatAttributeSet::GetEchoEfficiencyAttribute(),
	                       EGameplayModOp::Override,
	                       ReEchoGameplayTags::Data_EchoEfficiency);
}

UReEchoDamageEffect::UReEchoDamageEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	AddSetByCallerModifier(*this,
	                       UReEchoCombatAttributeSet::GetIncomingDamageAttribute(),
	                       EGameplayModOp::Additive,
	                       ReEchoGameplayTags::Data_Damage);
}

UReEchoHealEffect::UReEchoHealEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	AddSetByCallerModifier(*this,
	                       UReEchoCombatAttributeSet::GetIncomingHealingAttribute(),
	                       EGameplayModOp::Additive,
	                       ReEchoGameplayTags::Data_Heal);
}

UReEchoBasicAttackCooldownEffect::UReEchoBasicAttackCooldownEffect(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
	ConfigureCooldownDuration(*this);
	UTargetTagsGameplayEffectComponent* TagsComponent =
	    ObjectInitializer.CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(this, TEXT("BasicCooldownTags"));
	GEComponents.Add(TagsComponent);
	FInheritedTagContainer GrantedTags;
	GrantedTags.Added.AddTag(ReEchoGameplayTags::Cooldown_Attack_Basic);
	TagsComponent->SetAndApplyTargetTagChanges(GrantedTags);
}

UReEchoActiveAttackCooldownEffect::UReEchoActiveAttackCooldownEffect(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
	ConfigureCooldownDuration(*this);
	UTargetTagsGameplayEffectComponent* TagsComponent =
	    ObjectInitializer.CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(this, TEXT("ActiveCooldownTags"));
	GEComponents.Add(TagsComponent);
	FInheritedTagContainer GrantedTags;
	GrantedTags.Added.AddTag(ReEchoGameplayTags::Cooldown_Attack_Active);
	TagsComponent->SetAndApplyTargetTagChanges(GrantedTags);
}

bool ReEchoGameplayEffects::ApplyInitialization(UAbilitySystemComponent& Target,
                                                const FReEchoStatBlock& Stats,
                                                const bool bFillHealth)
{
	FGameplayEffectSpecHandle Spec = MakeSpec(Target, UReEchoInitializeStatsEffect::StaticClass());
	if (!Spec.IsValid())
	{
		return false;
	}
	const UReEchoCombatAttributeSet* Attributes = Target.GetSet<UReEchoCombatAttributeSet>();
	const float Health = bFillHealth || !Attributes ? Stats.HpMax : FMath::Min(Attributes->GetHealth(), Stats.HpMax);
	Spec.Data->SetSetByCallerMagnitude(ReEchoGameplayTags::Data_MaxHealth, Stats.HpMax);
	Spec.Data->SetSetByCallerMagnitude(ReEchoGameplayTags::Data_Health, Health);
	Spec.Data->SetSetByCallerMagnitude(ReEchoGameplayTags::Data_Block, static_cast<float>(Stats.Block));
	Spec.Data->SetSetByCallerMagnitude(ReEchoGameplayTags::Data_PhysicalAttack, Stats.PhysicalAttack);
	Spec.Data->SetSetByCallerMagnitude(ReEchoGameplayTags::Data_ElementalAttack, Stats.ElementalAttack);
	Spec.Data->SetSetByCallerMagnitude(ReEchoGameplayTags::Data_AttackSpeed, Stats.AttackSpeed);
	Spec.Data->SetSetByCallerMagnitude(ReEchoGameplayTags::Data_MovementSpeed, Stats.MovementSpeed);
	Spec.Data->SetSetByCallerMagnitude(ReEchoGameplayTags::Data_EchoEfficiency, Stats.EchoEfficiency);
	Target.ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	return true;
}

float ReEchoGameplayEffects::ApplyDamage(UAbilitySystemComponent* Source,
                                         UAbilitySystemComponent& Target,
                                         const float Damage)
{
	const UReEchoCombatAttributeSet* Attributes = Target.GetSet<UReEchoCombatAttributeSet>();
	if (!Attributes || Damage <= 0.0f || Attributes->GetHealth() <= 0.0f)
	{
		return 0.0f;
	}
	const float PreviousHealth = Attributes->GetHealth();
	UAbilitySystemComponent& SpecSource = Source ? *Source : Target;
	FGameplayEffectSpecHandle Spec = MakeSpec(SpecSource, UReEchoDamageEffect::StaticClass());
	if (!Spec.IsValid())
	{
		return 0.0f;
	}
	Spec.Data->SetSetByCallerMagnitude(ReEchoGameplayTags::Data_Damage, Damage);
	if (Source && Source != &Target)
	{
		Source->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), &Target);
	}
	else
	{
		Target.ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	}
	return FMath::Max(0.0f, PreviousHealth - Attributes->GetHealth());
}

float ReEchoGameplayEffects::ApplyHealing(UAbilitySystemComponent* Source,
                                          UAbilitySystemComponent& Target,
                                          const float Healing)
{
	const UReEchoCombatAttributeSet* Attributes = Target.GetSet<UReEchoCombatAttributeSet>();
	if (!Attributes || Healing <= 0.0f || Attributes->GetHealth() <= 0.0f)
	{
		return 0.0f;
	}
	const float PreviousHealth = Attributes->GetHealth();
	UAbilitySystemComponent& SpecSource = Source ? *Source : Target;
	FGameplayEffectSpecHandle Spec = MakeSpec(SpecSource, UReEchoHealEffect::StaticClass());
	if (!Spec.IsValid())
	{
		return 0.0f;
	}
	Spec.Data->SetSetByCallerMagnitude(ReEchoGameplayTags::Data_Heal, Healing);
	if (Source && Source != &Target)
	{
		Source->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), &Target);
	}
	else
	{
		Target.ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	}
	return FMath::Max(0.0f, Attributes->GetHealth() - PreviousHealth);
}