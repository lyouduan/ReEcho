#include "Combat/ReEchoCombatantComponent.h"

#include "AbilitySystem/ReEchoCombatAttributeSet.h"
#include "AbilitySystem/ReEchoGameplayEffects.h"
#include "AbilitySystem/ReEchoGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"

UReEchoCombatantComponent::UReEchoCombatantComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UReEchoCombatantComponent::BeginPlay()
{
	Super::BeginPlay();
	if (IAbilitySystemInterface* AbilityOwner = Cast<IAbilitySystemInterface>(GetOwner()))
	{
		BindToAbilitySystem(AbilityOwner->GetAbilitySystemComponent());
	}
	if (!BoundAbilitySystem)
	{
		CurrentHealth = FMath::Clamp(CurrentHealth, 0.f, Stats.HpMax);
	}
}

void UReEchoCombatantComponent::BindToAbilitySystem(UAbilitySystemComponent* InAbilitySystem)
{
	if (!InAbilitySystem || BoundAbilitySystem == InAbilitySystem)
	{
		return;
	}
	BoundAbilitySystem = InAbilitySystem;
	BoundAbilitySystem->GetGameplayAttributeValueChangeDelegate(UReEchoCombatAttributeSet::GetHealthAttribute())
	    .AddUObject(this, &UReEchoCombatantComponent::HandleHealthChanged);
	BoundAbilitySystem->GetGameplayAttributeValueChangeDelegate(UReEchoCombatAttributeSet::GetMaxHealthAttribute())
	    .AddUObject(this, &UReEchoCombatantComponent::HandleMaxHealthChanged);
	BoundAbilitySystem->GetGameplayAttributeValueChangeDelegate(UReEchoCombatAttributeSet::GetBlockAttribute())
	    .AddUObject(this, &UReEchoCombatantComponent::HandleBlockChanged);
	BoundAbilitySystem->GetGameplayAttributeValueChangeDelegate(UReEchoCombatAttributeSet::GetPhysicalAttackAttribute())
	    .AddUObject(this, &UReEchoCombatantComponent::HandlePhysicalAttackChanged);
	BoundAbilitySystem
	    ->GetGameplayAttributeValueChangeDelegate(UReEchoCombatAttributeSet::GetElementalAttackAttribute())
	    .AddUObject(this, &UReEchoCombatantComponent::HandleElementalAttackChanged);
	BoundAbilitySystem->GetGameplayAttributeValueChangeDelegate(UReEchoCombatAttributeSet::GetAttackSpeedAttribute())
	    .AddUObject(this, &UReEchoCombatantComponent::HandleAttackSpeedChanged);
	BoundAbilitySystem->GetGameplayAttributeValueChangeDelegate(UReEchoCombatAttributeSet::GetMovementSpeedAttribute())
	    .AddUObject(this, &UReEchoCombatantComponent::HandleMovementSpeedChanged);
	BoundAbilitySystem->GetGameplayAttributeValueChangeDelegate(UReEchoCombatAttributeSet::GetEchoEfficiencyAttribute())
	    .AddUObject(this, &UReEchoCombatantComponent::HandleEchoEfficiencyChanged);
	SyncFromAbilitySystem();
}

UAbilitySystemComponent* UReEchoCombatantComponent::GetBoundAbilitySystem() const
{
	return BoundAbilitySystem;
}

void UReEchoCombatantComponent::InitializeFromStats(const FReEchoStatBlock& InStats, const bool bFillHealth)
{
	bDeathBroadcast = false;
	if (BoundAbilitySystem)
	{
		BoundAbilitySystem->RemoveLooseGameplayTag(ReEchoGameplayTags::State_Dead);
	}
	if (BoundAbilitySystem)
	{
		ReEchoGameplayEffects::ApplyInitialization(*BoundAbilitySystem, InStats, bFillHealth);
		SyncFromAbilitySystem();
		return;
	}
	Stats = InStats;
	CurrentHealth = bFillHealth ? Stats.HpMax : FMath::Min(CurrentHealth, Stats.HpMax);
	OnHealthChanged.Broadcast(CurrentHealth, Stats.HpMax);
}

float UReEchoCombatantComponent::ApplyFinalDamage(const float Damage)
{
	if (!IsAlive() || Damage <= 0.f)
	{
		return 0.f;
	}
	if (BoundAbilitySystem)
	{
		return ReEchoGameplayEffects::ApplyDamage(nullptr, *BoundAbilitySystem, Damage);
	}
	if (Stats.Block > 0)
	{
		--Stats.Block;
		return 0.f;
	}
	const float Applied = FMath::Min(CurrentHealth, FMath::Max(1.f, Damage));
	CurrentHealth -= Applied;
	OnHealthChanged.Broadcast(CurrentHealth, Stats.HpMax);
	if (!IsAlive() && !bDeathBroadcast)
	{
		bDeathBroadcast = true;
		if (BoundAbilitySystem)
		{
			BoundAbilitySystem->AddLooseGameplayTag(ReEchoGameplayTags::State_Dead);
		}
		OnDeath.Broadcast();
	}
	return Applied;
}

float UReEchoCombatantComponent::ApplyHealing(const float Healing)
{
	if (!IsAlive() || Healing <= 0.0f)
	{
		return 0.0f;
	}
	if (BoundAbilitySystem)
	{
		return ReEchoGameplayEffects::ApplyHealing(nullptr, *BoundAbilitySystem, Healing);
	}
	const float PreviousHealth = CurrentHealth;
	CurrentHealth = FMath::Clamp(CurrentHealth + Healing, 0.0f, Stats.HpMax);
	OnHealthChanged.Broadcast(CurrentHealth, Stats.HpMax);
	return CurrentHealth - PreviousHealth;
}

bool UReEchoCombatantComponent::IsAlive() const
{
	return CurrentHealth > 0.f;
}

void UReEchoCombatantComponent::SyncFromAbilitySystem()
{
	if (!BoundAbilitySystem)
	{
		return;
	}
	const UReEchoCombatAttributeSet* Attributes = BoundAbilitySystem->GetSet<UReEchoCombatAttributeSet>();
	if (!Attributes)
	{
		return;
	}
	CurrentHealth = Attributes->GetHealth();
	Stats.HpMax = Attributes->GetMaxHealth();
	Stats.Block = FMath::RoundToInt(Attributes->GetBlock());
	Stats.PhysicalAttack = Attributes->GetPhysicalAttack();
	Stats.ElementalAttack = Attributes->GetElementalAttack();
	Stats.AttackSpeed = Attributes->GetAttackSpeed();
	Stats.MovementSpeed = Attributes->GetMovementSpeed();
	Stats.EchoEfficiency = Attributes->GetEchoEfficiency();
	OnHealthChanged.Broadcast(CurrentHealth, Stats.HpMax);
}

void UReEchoCombatantComponent::HandleHealthChanged(const FOnAttributeChangeData& Data)
{
	CurrentHealth = Data.NewValue;
	OnHealthChanged.Broadcast(CurrentHealth, Stats.HpMax);
	if (Data.OldValue > 0.0f && Data.NewValue <= 0.0f && !bDeathBroadcast)
	{
		bDeathBroadcast = true;
		if (BoundAbilitySystem)
		{
			BoundAbilitySystem->AddLooseGameplayTag(ReEchoGameplayTags::State_Dead);
		}
		OnDeath.Broadcast();
	}
}

void UReEchoCombatantComponent::HandleMaxHealthChanged(const FOnAttributeChangeData& Data)
{
	Stats.HpMax = Data.NewValue;
	OnHealthChanged.Broadcast(CurrentHealth, Stats.HpMax);
}

void UReEchoCombatantComponent::HandleBlockChanged(const FOnAttributeChangeData& Data)
{
	Stats.Block = FMath::RoundToInt(Data.NewValue);
}

void UReEchoCombatantComponent::HandlePhysicalAttackChanged(const FOnAttributeChangeData& Data)
{
	Stats.PhysicalAttack = Data.NewValue;
}

void UReEchoCombatantComponent::HandleElementalAttackChanged(const FOnAttributeChangeData& Data)
{
	Stats.ElementalAttack = Data.NewValue;
}

void UReEchoCombatantComponent::HandleAttackSpeedChanged(const FOnAttributeChangeData& Data)
{
	Stats.AttackSpeed = Data.NewValue;
}

void UReEchoCombatantComponent::HandleMovementSpeedChanged(const FOnAttributeChangeData& Data)
{
	Stats.MovementSpeed = Data.NewValue;
}

void UReEchoCombatantComponent::HandleEchoEfficiencyChanged(const FOnAttributeChangeData& Data)
{
	Stats.EchoEfficiency = Data.NewValue;
}