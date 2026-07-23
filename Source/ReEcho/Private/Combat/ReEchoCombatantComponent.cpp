#include "Combat/ReEchoCombatantComponent.h"

UReEchoCombatantComponent::UReEchoCombatantComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UReEchoCombatantComponent::BeginPlay()
{
	Super::BeginPlay();
	CurrentHealth = FMath::Clamp(CurrentHealth, 0.f, Stats.HpMax);
}

void UReEchoCombatantComponent::InitializeFromStats(const FReEchoStatBlock& InStats, const bool bFillHealth)
{
	Stats = InStats;
	CurrentHealth = bFillHealth ? Stats.HpMax : FMath::Min(CurrentHealth, Stats.HpMax);
	OnHealthChanged.Broadcast(CurrentHealth, Stats.HpMax);
}

float UReEchoCombatantComponent::ApplyFinalDamage(float Damage)
{
	if (!IsAlive() || Damage <= 0.f)
	{
		return 0.f;
	}
	if (Stats.Block > 0)
	{
		--Stats.Block;
		return 0.f;
	}
	const float Applied = FMath::Min(CurrentHealth, FMath::Max(1.f, Damage));
	CurrentHealth -= Applied;
	OnHealthChanged.Broadcast(CurrentHealth, Stats.HpMax);
	if (!IsAlive())
	{
		OnDeath.Broadcast();
	}
	return Applied;
}

bool UReEchoCombatantComponent::IsAlive() const
{
	return CurrentHealth > 0.f;
}

