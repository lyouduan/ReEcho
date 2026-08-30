#include "Combat/ReEchoCombatantComponent.h"

#include "Combat/ReEchoCombatTarget.h"

#include "AbilitySystem/ReEchoCombatAttributeSet.h"
#include "AbilitySystem/ReEchoGameplayEffects.h"
#include "AbilitySystem/ReEchoGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"

UReEchoCombatantComponent::UReEchoCombatantComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
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
	const float PreviousHealth = CurrentHealth;
	// GAS mirrors only a subset of the authored stat block. Preserve semantic fields before synchronizing attributes.
	Stats = InStats;
	if (bFillHealth)
	{
		Overhealth = 0.0f;
	}
	bDeathBroadcast = false;
	bHasLastPlayerEchoDamageSource = false;
	LastPlayerEchoDamageSource = EReEchoDamageSource::Player;
	for (int32 Index = TransientStatStacks.Num() - 1; Index >= 0; --Index)
	{
		RemoveTransientStatStack(Index);
	}
	AdditiveAttackModifiers.Reset();
	BleedingStacks.Reset();
	StunnedUntilWorldTime = 0.0f;
	// bFillHealth=false is also the production path for in-encounter stat refreshes after card reactions and kills.
	// Those refreshes must not silently cancel an already granted timed-invulnerability window.
	if (bFillHealth)
	{
		InvulnerableUntilWorldTime = 0.0f;
	}
	HealthChangeReason = TEXT("Initialize");
	HealthChangeAttack = {};
	if (BoundAbilitySystem)
	{
		BoundAbilitySystem->RemoveLooseGameplayTag(ReEchoGameplayTags::State_Dead);
	}
	if (BoundAbilitySystem)
	{
		// Initialization overrides health before physical/elemental attack. Suppress that intermediate health
		// notification: otherwise a health-dependent modifier (Brave's missing-health attack bonus) can be
		// recorded between those writes, then be overwritten by the later attack override while its map entry
		// incorrectly still says the bonus is applied. SyncFromAbilitySystem broadcasts once after every base
		// attribute is final, letting observers restore their persistent state from the committed health value.
		bDeferHealthNotifications = true;
		ReEchoGameplayEffects::ApplyInitialization(*BoundAbilitySystem, InStats, bFillHealth);
		// Keep the bookkeeping aligned with the just-overridden GAS bases even if another observer ran
		// during initialization. SyncFromAbilitySystem below will rebuild persistent health-dependent bonuses.
		AdditiveAttackModifiers.Reset();
		bDeferHealthNotifications = false;
		SyncFromAbilitySystem();
		HealthChangeReason = NAME_None;
		return;
	}
	Stats = InStats;
	CurrentHealth = bFillHealth ? Stats.HpMax : FMath::Min(CurrentHealth, Stats.HpMax);
	OnHealthChanged.Broadcast(GetEffectiveCurrentHealth(), Stats.HpMax);
	PublishHealthChange(PreviousHealth);
	HealthChangeReason = NAME_None;
}

float UReEchoCombatantComponent::ApplyFinalDamage(const float Damage,
                                                  const FReEchoAttackIdentity& Attack,
                                                  const EReEchoDamageSource DamageSource)
{
	const float WorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	if (!IsAlive() || Damage <= 0.f)
	{
		return 0.f;
	}
	if (bDebugInvulnerable)
	{
		// GMGod preserves the resolved damage value for Hurt/VFX/damage-number consumers, but deliberately skips
		// every health, block and death mutation below.
		return Damage;
	}
	if (IsTimedInvulnerable(WorldTime))
	{
		UE_LOG(LogTemp,
		       Display,
		       TEXT("[CombatInvulnerability] blocked damage=%.3f owner=%s world=%.3f until=%.3f"),
		       Damage,
		       *GetNameSafe(GetOwner()),
		       WorldTime,
		       InvulnerableUntilWorldTime);
		return 0.f;
	}
	const FName CursedStatusId(TEXT("Z_Cursed"));
	const float* CursedUntil = ElementState.ActiveStatusUntilSeconds.Find(CursedStatusId);
	const bool bHasActiveCurse = !bCursedImmune && CursedUntil && *CursedUntil > WorldTime;
	float ResolvedDamage = bHasActiveCurse ? FMath::Max(Damage, GetEffectiveCurrentHealth()) : Damage;
	float OverhealthDamage = 0.0f;
	if (Stats.Block <= 0 && Overhealth > 0.0f)
	{
		const float PreviousEffectiveHealth = GetEffectiveCurrentHealth();
		OverhealthDamage = FMath::Min(Overhealth, ResolvedDamage);
		Overhealth -= OverhealthDamage;
		ResolvedDamage -= OverhealthDamage;
		OnHealthChanged.Broadcast(GetEffectiveCurrentHealth(), Stats.HpMax);
		PublishHealthChange(PreviousEffectiveHealth);
		if (ResolvedDamage <= 0.0f)
		{
			return OverhealthDamage;
		}
	}
	if (BoundAbilitySystem)
	{
		HealthChangeReason = TEXT("Damage");
		HealthChangeAttack = Attack;
		HealthChangeDamageSource = DamageSource;
		UAbilitySystemComponent* SourceAbilitySystem = nullptr;
		if (IAbilitySystemInterface* Source = Cast<IAbilitySystemInterface>(Attack.Source.Get()))
		{
			SourceAbilitySystem = Source->GetAbilitySystemComponent();
		}
		const float Applied =
		    ReEchoGameplayEffects::ApplyDamage(SourceAbilitySystem, *BoundAbilitySystem, ResolvedDamage);
		if (bHasActiveCurse && Applied > 0.0f)
		{
			ElementState.ActiveStatusUntilSeconds.Remove(CursedStatusId);
			PublishElementStateChange();
		}
		HealthChangeReason = NAME_None;
		HealthChangeAttack = {};
		return OverhealthDamage + Applied;
	}
	if (Stats.Block > 0)
	{
		--Stats.Block;
		return 0.f;
	}
	const float Applied = FMath::Min(CurrentHealth, ResolvedDamage);
	const float PreviousHealth = CurrentHealth;
	HealthChangeReason = TEXT("Damage");
	HealthChangeAttack = Attack;
	HealthChangeDamageSource = DamageSource;
	CurrentHealth -= Applied;
	if (bHasActiveCurse && Applied > 0.0f)
	{
		ElementState.ActiveStatusUntilSeconds.Remove(CursedStatusId);
		PublishElementStateChange();
	}
	OnHealthChanged.Broadcast(GetEffectiveCurrentHealth(), Stats.HpMax);
	PublishHealthChange(PreviousHealth);
	if (!IsAlive() && !bDeathBroadcast)
	{
		// WS4 (Plan 68): a lethal hit may be converted into a phase transition instead of death. If the bound hook
		// defers the kill, clamp health to a survivable value and skip OnDeath; otherwise fall through to normal death.
		if (TryDeferFatalDamageForPhaseTransition(CurrentHealth))
		{
			HealthChangeReason = NAME_None;
			HealthChangeAttack = {};
			return Applied;
		}
		bDeathBroadcast = true;
		if (BoundAbilitySystem)
		{
			BoundAbilitySystem->AddLooseGameplayTag(ReEchoGameplayTags::State_Dead);
		}
		OnDeath.Broadcast();
	}
	HealthChangeReason = NAME_None;
	HealthChangeAttack = {};
	return OverhealthDamage + Applied;
}

void UReEchoCombatantComponent::SetDebugInvulnerable(const bool bEnabled)
{
#if !UE_BUILD_SHIPPING
	bDebugInvulnerable = bEnabled;
#endif
}

bool UReEchoCombatantComponent::IsDebugInvulnerable() const
{
#if UE_BUILD_SHIPPING
	return false;
#else
	return bDebugInvulnerable;
#endif
}

bool UReEchoCombatantComponent::ApplyTimedStatus(const FReEchoTimedStatusCommand& Command)
{
	if (!IsAlive() || !FMath::IsFinite(Command.CurrentTimeSeconds) || Command.CurrentTimeSeconds < 0.0f ||
	    !FMath::IsFinite(Command.DurationSeconds) || Command.DurationSeconds <= 0.0f)
	{
		return false;
	}
	const float ExpiresAt = Command.CurrentTimeSeconds + Command.DurationSeconds;
	if (Command.StatusId == TEXT("Z_Vertigo") && !bStunImmune)
	{
		StunnedUntilWorldTime = FMath::Max(StunnedUntilWorldTime, ExpiresAt);
		ElementState.ActiveStatusUntilSeconds.Add(Command.StatusId, StunnedUntilWorldTime);
		if (BoundAbilitySystem)
		{
			BoundAbilitySystem->AddLooseGameplayTag(ReEchoGameplayTags::State_Stunned);
		}
	}
	else if (Command.StatusId == TEXT("Z_Bleeding") && Command.DamagePerTickMaxHealthFraction > 0.0f)
	{
		FBleedingStack& Stack = BleedingStacks.AddDefaulted_GetRef();
		Stack.ExpiresAt = ExpiresAt;
		Stack.NextTickAt = Command.CurrentTimeSeconds + 1.0f;
		Stack.DamageFraction = Command.DamagePerTickMaxHealthFraction;
		Stack.Attack = Command.Attack;
		Stack.DamageSource = Command.DamageSource;
		ElementState.ActiveStatusUntilSeconds.Add(
		    Command.StatusId, FMath::Max(ElementState.ActiveStatusUntilSeconds.FindRef(Command.StatusId), ExpiresAt));
	}
	else if (Command.StatusId == TEXT("Z_Cursed") && !bCursedImmune)
	{
		ElementState.ActiveStatusUntilSeconds.Add(
		    Command.StatusId, FMath::Max(ElementState.ActiveStatusUntilSeconds.FindRef(Command.StatusId), ExpiresAt));
	}
	else
	{
		return false;
	}
	PublishElementStateChange();
	RefreshTickState();
	if (const IReEchoCombatTarget* SourceRules = Cast<IReEchoCombatTarget>(Command.Attack.Source.Get()))
	{
		SourceRules->NotifyNegativeStatusApplied(Command.StatusId);
	}
	return true;
}

void UReEchoCombatantComponent::SetCursedImmune(const bool bImmune)
{
	bCursedImmune = bImmune;
	if (bCursedImmune && ElementState.ActiveStatusUntilSeconds.Remove(TEXT("Z_Cursed")) > 0)
	{
		PublishElementStateChange();
	}
	RefreshTickState();
}

void UReEchoCombatantComponent::SetStunImmune(const bool bImmune)
{
	bStunImmune = bImmune;
	if (!bStunImmune)
	{
		return;
	}

	const bool bHadStunTimer = StunnedUntilWorldTime > 0.0f;
	const bool bHadStunStatus = ElementState.ActiveStatusUntilSeconds.Remove(TEXT("Z_Vertigo")) > 0;
	const bool bHadActiveStun = bHadStunTimer || bHadStunStatus;
	StunnedUntilWorldTime = 0.0f;
	if (BoundAbilitySystem)
	{
		BoundAbilitySystem->RemoveLooseGameplayTag(ReEchoGameplayTags::State_Stunned);
	}
	if (bHadActiveStun)
	{
		PublishElementStateChange();
	}
	RefreshTickState();
}

bool UReEchoCombatantComponent::IsActionDisabled(const float CurrentTimeSeconds) const
{
	return CurrentTimeSeconds < StunnedUntilWorldTime;
}

void UReEchoCombatantComponent::GrantTimedInvulnerability(const float CurrentTimeSeconds, const float DurationSeconds)
{
	if (CurrentTimeSeconds >= 0.0f && DurationSeconds > 0.0f)
	{
		InvulnerableUntilWorldTime = FMath::Max(InvulnerableUntilWorldTime, CurrentTimeSeconds + DurationSeconds);
		RefreshTickState();
	}
}

bool UReEchoCombatantComponent::IsTimedInvulnerable(const float CurrentTimeSeconds) const
{
	return CurrentTimeSeconds < InvulnerableUntilWorldTime;
}

void UReEchoCombatantComponent::AddTransientStatModifier(const FName SourceId,
                                                         const float AttackSpeedBonusFraction,
                                                         const float MovementSpeedBonusFraction,
                                                         const float DurationSeconds,
                                                         const int32 MaxStacks)
{
	if (SourceId.IsNone() || DurationSeconds <= 0.0f || MaxStacks <= 0)
	{
		return;
	}
	while (GetTransientStatStackCount(SourceId) >= MaxStacks)
	{
		if (!RemoveOldestTransientStatModifier(SourceId))
		{
			break;
		}
	}
	FTransientStatStack& Stack = TransientStatStacks.AddDefaulted_GetRef();
	Stack.SourceId = SourceId;
	Stack.ExpiresAt = (GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f) + DurationSeconds;
	Stack.AttackSpeedMultiplier = FMath::Max(0.01f, 1.0f + AttackSpeedBonusFraction);
	Stack.MovementSpeedMultiplier = FMath::Max(0.01f, 1.0f + MovementSpeedBonusFraction);
	float FallbackBaseAttackSpeed = Stats.AttackSpeed;
	float FallbackBaseMovementSpeed = Stats.MovementSpeed;
	for (const FTransientStatStack& Existing : TransientStatStacks)
	{
		if (&Existing != &Stack)
		{
			FallbackBaseAttackSpeed -= Existing.FallbackAttackSpeedDelta;
			FallbackBaseMovementSpeed -= Existing.FallbackMovementSpeedDelta;
		}
	}
	Stack.FallbackAttackSpeedDelta = FallbackBaseAttackSpeed * (Stack.AttackSpeedMultiplier - 1.0f);
	Stack.FallbackMovementSpeedDelta = FallbackBaseMovementSpeed * (Stack.MovementSpeedMultiplier - 1.0f);
	if (BoundAbilitySystem)
	{
		Stack.GameplayEffectHandle = ReEchoGameplayEffects::ApplyTransientStatMultiplier(
		    *BoundAbilitySystem, Stack.AttackSpeedMultiplier, Stack.MovementSpeedMultiplier);
	}
	// Keep the raw stat block in sync for BOTH backends. Echoes (no ability system) and the player
	// (ability-system bound) must both see transient attack/move speed in weapon cadence and movement.
	// Previously only the no-ability-system fallback mutated Stats, so player attack-speed runes had no effect.
	Stats.AttackSpeed = FMath::Min(Stats.AttackSpeed + Stack.FallbackAttackSpeedDelta,
	                            FReEchoStatBlock::MaxAttackSpeedMultiplier);
	Stats.MovementSpeed += Stack.FallbackMovementSpeedDelta;
	RefreshTickState();
}

bool UReEchoCombatantComponent::RemoveOldestTransientStatModifier(const FName SourceId)
{
	int32 OldestIndex = INDEX_NONE;
	float OldestExpiry = TNumericLimits<float>::Max();
	for (int32 Index = 0; Index < TransientStatStacks.Num(); ++Index)
	{
		const FTransientStatStack& Stack = TransientStatStacks[Index];
		if (Stack.SourceId == SourceId && Stack.ExpiresAt < OldestExpiry)
		{
			OldestExpiry = Stack.ExpiresAt;
			OldestIndex = Index;
		}
	}
	if (OldestIndex == INDEX_NONE)
	{
		return false;
	}
	RemoveTransientStatStack(OldestIndex);
	return true;
}

int32 UReEchoCombatantComponent::GetTransientStatStackCount(const FName SourceId) const
{
	int32 Count = 0;
	for (const FTransientStatStack& Stack : TransientStatStacks)
	{
		if (Stack.SourceId == SourceId)
		{
			++Count;
		}
	}
	return Count;
}

void UReEchoCombatantComponent::SetAdditiveAttackModifier(const FName SourceId,
                                                          const float PhysicalAttackBonus,
                                                          const float ElementalAttackBonus)
{
	if (SourceId.IsNone())
	{
		return;
	}
	const FVector2D Previous = AdditiveAttackModifiers.FindRef(SourceId);
	const FVector2D Requested(FMath::Max(0.0f, PhysicalAttackBonus), FMath::Max(0.0f, ElementalAttackBonus));
	const FVector2D Delta = Requested - Previous;
	if (Delta.IsNearlyZero())
	{
		return;
	}
	if (Requested.IsNearlyZero())
	{
		AdditiveAttackModifiers.Remove(SourceId);
	}
	else
	{
		AdditiveAttackModifiers.Add(SourceId, Requested);
	}
	if (BoundAbilitySystem)
	{
		const FGameplayAttribute Physical = UReEchoCombatAttributeSet::GetPhysicalAttackAttribute();
		const FGameplayAttribute Elemental = UReEchoCombatAttributeSet::GetElementalAttackAttribute();
		BoundAbilitySystem->SetNumericAttributeBase(
		    Physical, FMath::Max(0.0f, BoundAbilitySystem->GetNumericAttributeBase(Physical) + Delta.X));
		BoundAbilitySystem->SetNumericAttributeBase(
		    Elemental, FMath::Max(0.0f, BoundAbilitySystem->GetNumericAttributeBase(Elemental) + Delta.Y));
	}
	else
	{
		Stats.PhysicalAttack = FMath::Max(0.0f, Stats.PhysicalAttack + Delta.X);
		Stats.ElementalAttack = FMath::Max(0.0f, Stats.ElementalAttack + Delta.Y);
	}
}

void UReEchoCombatantComponent::RemoveTransientStatStack(const int32 Index)
{
	if (!TransientStatStacks.IsValidIndex(Index))
	{
		return;
	}
	const FTransientStatStack Stack = TransientStatStacks[Index];
	if (BoundAbilitySystem && Stack.GameplayEffectHandle.IsValid())
	{
		BoundAbilitySystem->RemoveActiveGameplayEffect(Stack.GameplayEffectHandle);
	}
	Stats.AttackSpeed -= Stack.FallbackAttackSpeedDelta;
	Stats.MovementSpeed -= Stack.FallbackMovementSpeedDelta;
	TransientStatStacks.RemoveAt(Index);
}

void UReEchoCombatantComponent::RefreshTickState()
{
	const float WorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	const bool bCurseNeedsExpiryTick = ElementState.ActiveStatusUntilSeconds.FindRef(TEXT("Z_Cursed")) > WorldTime;
	SetComponentTickEnabled(!BleedingStacks.IsEmpty() || !TransientStatStacks.IsEmpty() ||
	                        StunnedUntilWorldTime > WorldTime || InvulnerableUntilWorldTime > WorldTime ||
	                        bCurseNeedsExpiryTick);
}

float UReEchoCombatantComponent::ApplyHealing(const float Healing)
{
	if (!IsAlive() || Healing <= 0.0f)
	{
		return 0.0f;
	}
	const float PreviousEffectiveHealth = GetEffectiveCurrentHealth();
	float BaseHealingApplied = 0.0f;
	if (BoundAbilitySystem)
	{
		HealthChangeReason = TEXT("Healing");
		HealthChangeAttack = {};
		BaseHealingApplied = ReEchoGameplayEffects::ApplyHealing(nullptr, *BoundAbilitySystem, Healing);
		HealthChangeReason = NAME_None;
	}
	else
	{
		const float PreviousHealth = CurrentHealth;
		HealthChangeReason = TEXT("Healing");
		HealthChangeAttack = {};
		CurrentHealth = FMath::Clamp(CurrentHealth + Healing, 0.0f, Stats.HpMax);
		BaseHealingApplied = CurrentHealth - PreviousHealth;
		HealthChangeReason = NAME_None;
	}
	const float RemainingHealing = FMath::Max(0.0f, Healing - BaseHealingApplied);
	const float PreviousOverhealth = Overhealth;
	const float Capacity = Stats.HpMax * OverhealCapacityFraction;
	Overhealth = FMath::Clamp(Overhealth + RemainingHealing, 0.0f, Capacity);
	const float OverhealthApplied = Overhealth - PreviousOverhealth;
	if (!FMath::IsNearlyEqual(GetEffectiveCurrentHealth(), PreviousEffectiveHealth))
	{
		HealthChangeReason = TEXT("Healing");
		OnHealthChanged.Broadcast(GetEffectiveCurrentHealth(), Stats.HpMax);
		PublishHealthChange(PreviousEffectiveHealth);
		HealthChangeReason = NAME_None;
	}
	return BaseHealingApplied + OverhealthApplied;
}

void UReEchoCombatantComponent::SetOverhealCapacityFraction(const float Fraction)
{
	const float PreviousEffectiveHealth = GetEffectiveCurrentHealth();
	OverhealCapacityFraction = FMath::Clamp(Fraction, 0.0f, 1.0f);
	Overhealth = FMath::Min(Overhealth, Stats.HpMax * OverhealCapacityFraction);
	if (!FMath::IsNearlyEqual(GetEffectiveCurrentHealth(), PreviousEffectiveHealth))
	{
		OnHealthChanged.Broadcast(GetEffectiveCurrentHealth(), Stats.HpMax);
		PublishHealthChange(PreviousEffectiveHealth);
	}
}

bool UReEchoCombatantComponent::ApplyHealthAdjustment(const float NewMaximumHealth,
                                                      const EReEchoHealthAdjustment Adjustment)
{
	if (Adjustment == EReEchoHealthAdjustment::None)
	{
		return false;
	}

	const float PreviousEffectiveHealth = GetEffectiveCurrentHealth();
	const float ClampedMaximumHealth = FMath::Max(1.0f, NewMaximumHealth);
	const float AdjustedHealth = Adjustment == EReEchoHealthAdjustment::FillToMax
	                                 ? ClampedMaximumHealth
	                                 : FMath::Min(CurrentHealth, ClampedMaximumHealth);
	HealthChangeReason = TEXT("Build");
	HealthChangeAttack = {};
	HealthChangeDamageSource = EReEchoDamageSource::Player;

	if (BoundAbilitySystem)
	{
		// Attribute delegates are synchronous. Defer their intermediate broadcasts so observers see the maximum and
		// current health as one committed state instead of two partially applied states.
		bDeferHealthNotifications = true;
		BoundAbilitySystem->SetNumericAttributeBase(UReEchoCombatAttributeSet::GetMaxHealthAttribute(),
		                                            ClampedMaximumHealth);
		BoundAbilitySystem->SetNumericAttributeBase(UReEchoCombatAttributeSet::GetHealthAttribute(), AdjustedHealth);
		bDeferHealthNotifications = false;

		const UReEchoCombatAttributeSet* Attributes = BoundAbilitySystem->GetSet<UReEchoCombatAttributeSet>();
		if (!Attributes)
		{
			HealthChangeReason = NAME_None;
			return false;
		}
		Stats.HpMax = Attributes->GetMaxHealth();
		CurrentHealth = Attributes->GetHealth();
	}
	else
	{
		Stats.HpMax = ClampedMaximumHealth;
		CurrentHealth = AdjustedHealth;
	}

	Overhealth = FMath::Min(Overhealth, Stats.HpMax * OverhealCapacityFraction);
	Stats.HpPoint = CurrentHealth;
	bDeathBroadcast = CurrentHealth <= 0.0f;
	if (BoundAbilitySystem && CurrentHealth > 0.0f)
	{
		BoundAbilitySystem->RemoveLooseGameplayTag(ReEchoGameplayTags::State_Dead);
	}
	OnHealthChanged.Broadcast(GetEffectiveCurrentHealth(), Stats.HpMax);
	PublishHealthChange(PreviousEffectiveHealth);
	HealthChangeReason = NAME_None;
	return true;
}

void UReEchoCombatantComponent::ClampElementImmunityDuration(const float CurrentTimeSeconds,
                                                             const float MaximumRemainingSeconds)
{
	if (CurrentTimeSeconds < 0.0f || MaximumRemainingSeconds < 0.0f || ElementState.ImmunityUntil <= CurrentTimeSeconds)
	{
		return;
	}
	ElementState.ImmunityUntil = FMath::Min(ElementState.ImmunityUntil, CurrentTimeSeconds + MaximumRemainingSeconds);
	const FName ImmunityStatusId = TEXT("Z_Elemental_Immunity");
	if (float* StatusUntil = ElementState.ActiveStatusUntilSeconds.Find(ImmunityStatusId))
	{
		*StatusUntil = FMath::Min(*StatusUntil, ElementState.ImmunityUntil);
	}
}

void UReEchoCombatantComponent::RestoreCurrentHealth(const float SavedHealth)
{
	const float PreviousHealth = GetEffectiveCurrentHealth();
	HealthChangeReason = TEXT("Restore");
	HealthChangeAttack = {};
	const float MaximumEffectiveHealth = Stats.HpMax * (1.0f + OverhealCapacityFraction);
	const float ClampedEffectiveHealth = FMath::Clamp(SavedHealth, 0.0f, MaximumEffectiveHealth);
	const float ClampedHealth = FMath::Min(ClampedEffectiveHealth, Stats.HpMax);
	Overhealth = FMath::Max(0.0f, ClampedEffectiveHealth - Stats.HpMax);
	bDeathBroadcast = ClampedHealth <= 0.0f;
	if (BoundAbilitySystem)
	{
		BoundAbilitySystem->SetNumericAttributeBase(UReEchoCombatAttributeSet::GetHealthAttribute(), ClampedHealth);
		if (bDeathBroadcast)
		{
			BoundAbilitySystem->AddLooseGameplayTag(ReEchoGameplayTags::State_Dead);
		}
		else
		{
			BoundAbilitySystem->RemoveLooseGameplayTag(ReEchoGameplayTags::State_Dead);
		}
		SyncFromAbilitySystem();
		OnHealthChanged.Broadcast(GetEffectiveCurrentHealth(), Stats.HpMax);
		PublishHealthChange(PreviousHealth);
		HealthChangeReason = NAME_None;
		return;
	}
	CurrentHealth = ClampedHealth;
	OnHealthChanged.Broadcast(GetEffectiveCurrentHealth(), Stats.HpMax);
	PublishHealthChange(PreviousHealth);
	HealthChangeReason = NAME_None;
}

bool UReEchoCombatantComponent::IsAlive() const
{
	return CurrentHealth > 0.f;
}

bool UReEchoCombatantComponent::TryDeferFatalDamageForPhaseTransition(float& InOutHealth)
{
	if (!OnFatalDamage.IsBound())
	{
		return false;
	}
	// The hook decides whether this lethal hit should be converted into a phase transition. While it runs we mark
	// bDeathBroadcast so a recursive health-change delegate (e.g. the GAS attribute write-back below) cannot re-enter
	// and broadcast OnDeath a second time. Normal death flow resets bDeathBroadcast appropriately afterwards.
	const bool bPreviousDeathBroadcast = bDeathBroadcast;
	bDeathBroadcast = true;
	const bool bDeferDeath = OnFatalDamage.Execute(InOutHealth);
	bDeathBroadcast = bPreviousDeathBroadcast;
	if (!bDeferDeath)
	{
		return false;
	}
	// Convert the would-be-lethal value into a survivable one. Callers route this through their own path (fallback
	// writes CurrentHealth directly; the GAS delegate path must also correct the attribute).
	InOutHealth = FMath::Max(1.0f, InOutHealth);
	if (BoundAbilitySystem)
	{
		// GAS is authoritative: pin the attribute to the survivable value and clear the dead tag so the enemy lives
		// long enough to enter its second phase. The caller will then publish the corrected health change.
		BoundAbilitySystem->SetNumericAttributeBase(UReEchoCombatAttributeSet::GetHealthAttribute(), InOutHealth);
		BoundAbilitySystem->RemoveLooseGameplayTag(ReEchoGameplayTags::State_Dead);
		SyncFromAbilitySystem();
	}
	return true;
}

FReEchoCombatantSnapshot UReEchoCombatantComponent::GetSnapshot() const
{
	FReEchoCombatantSnapshot Snapshot;
	Snapshot.Combatant = const_cast<UReEchoCombatantComponent*>(this);
	Snapshot.CurrentHealth = GetEffectiveCurrentHealth();
	Snapshot.MaximumHealth = Stats.HpMax;
	Snapshot.Stats = Stats;
	Snapshot.ElementState = ElementState;
	Snapshot.bAlive = IsAlive();
	return Snapshot;
}

const FReEchoElementState& UReEchoCombatantComponent::GetElementState() const
{
	return ElementState;
}

FReEchoElementCleanseResult
UReEchoCombatantComponent::ExecuteElementCleanse(const FReEchoElementCleanseCommand& Command)
{
	FReEchoElementCleanseResult Result;
	if (!FMath::IsFinite(Command.CurrentTimeSeconds) || Command.CurrentTimeSeconds < 0.0f ||
	    !FMath::IsFinite(Command.ImmunityDurationSeconds) || Command.ImmunityDurationSeconds <= 0.0f)
	{
		return Result;
	}

	const float RequestedImmunityUntil = Command.CurrentTimeSeconds + Command.ImmunityDurationSeconds;
	if (!FMath::IsFinite(RequestedImmunityUntil))
	{
		return Result;
	}

	static const FName ElementalImmunityStatusId = TEXT("Z_Elemental_Immunity");
	static const FName BurnStatusId = TEXT("Z_Burn");
	const bool bHadBurnState =
	    ElementState.bBurnActive || ElementState.ActiveStatusUntilSeconds.Contains(BurnStatusId) ||
	    ElementState.BurnTickDamage != 0.0f || ElementState.BurnNextTickTimeSeconds != 0.0f ||
	    !ElementState.BurnReactionBehaviorId.IsNone() || ElementState.BurnSourceLocation != FVector::ZeroVector ||
	    !ElementState.BurnAttack.Source.IsExplicitlyNull() || ElementState.BurnAttack.Sequence != 0;
	Result.bClearedAttachment = ElementState.Attached != EReEchoElement::None;
	Result.bClearedBurn = bHadBurnState;

	ElementState.Attached = EReEchoElement::None;
	ElementState.bBurnActive = false;
	ElementState.BurnTickDamage = 0.0f;
	ElementState.BurnNextTickTimeSeconds = 0.0f;
	ElementState.BurnReactionBehaviorId = NAME_None;
	ElementState.BurnSourceLocation = FVector::ZeroVector;
	ElementState.BurnAttack = {};
	ElementState.ActiveStatusUntilSeconds.Remove(BurnStatusId);

	const float PreviousImmunityUntil = ElementState.ImmunityUntil;
	const float PreviousStatusUntil = ElementState.ActiveStatusUntilSeconds.FindRef(ElementalImmunityStatusId);
	const float GrantedImmunityUntil = FMath::Max3(PreviousImmunityUntil, PreviousStatusUntil, RequestedImmunityUntil);
	ElementState.ImmunityUntil = GrantedImmunityUntil;
	ElementState.ActiveStatusUntilSeconds.Add(ElementalImmunityStatusId, GrantedImmunityUntil);

	Result.bSucceeded = true;
	Result.ImmunityUntil = ElementState.ImmunityUntil;
	Result.bStateChanged =
	    Result.bClearedAttachment || Result.bClearedBurn || PreviousImmunityUntil != ElementState.ImmunityUntil ||
	    PreviousStatusUntil != ElementState.ActiveStatusUntilSeconds.FindRef(ElementalImmunityStatusId);
	if (Result.bStateChanged)
	{
		PublishElementStateChange();
	}
	return Result;
}

void UReEchoCombatantComponent::RestoreElementState(const FReEchoElementState& SavedState)
{
	ElementState = SavedState;
	if (bStunImmune)
	{
		ElementState.ActiveStatusUntilSeconds.Remove(TEXT("Z_Vertigo"));
	}
	PublishElementStateChange();
}

void UReEchoCombatantComponent::ResetElementState()
{
	ElementState = {};
	PublishElementStateChange();
}

#if WITH_DEV_AUTOMATION_TESTS
FReEchoElementState& UReEchoCombatantComponent::EditElementStateForTests()
{
	return ElementState;
}

float UReEchoCombatantComponent::ApplyFinalDamageForTests(const float Damage)
{
	return ApplyFinalDamage(Damage, {}, EReEchoDamageSource::Player);
}

void UReEchoCombatantComponent::AdvanceTimedRuneEffectsForTests(const float CurrentTimeSeconds)
{
	AdvanceTimedRuntimeState(CurrentTimeSeconds);
}
#endif

void UReEchoCombatantComponent::TickComponent(const float DeltaTime,
                                              const ELevelTick TickType,
                                              FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	const float WorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	AdvanceTimedRuntimeState(WorldTime);
}

void UReEchoCombatantComponent::AdvanceTimedRuntimeState(const float CurrentTimeSeconds)
{
	if (const float* CursedUntil = ElementState.ActiveStatusUntilSeconds.Find(TEXT("Z_Cursed"));
	    CursedUntil && CurrentTimeSeconds >= *CursedUntil)
	{
		ElementState.ActiveStatusUntilSeconds.Remove(TEXT("Z_Cursed"));
		PublishElementStateChange();
	}
	if (StunnedUntilWorldTime > 0.0f && CurrentTimeSeconds >= StunnedUntilWorldTime)
	{
		StunnedUntilWorldTime = 0.0f;
		ElementState.ActiveStatusUntilSeconds.Remove(TEXT("Z_Vertigo"));
		if (BoundAbilitySystem)
		{
			BoundAbilitySystem->RemoveLooseGameplayTag(ReEchoGameplayTags::State_Stunned);
		}
		PublishElementStateChange();
	}
	if (InvulnerableUntilWorldTime > 0.0f && CurrentTimeSeconds >= InvulnerableUntilWorldTime)
	{
		InvulnerableUntilWorldTime = 0.0f;
	}
	for (int32 Index = BleedingStacks.Num() - 1; Index >= 0; --Index)
	{
		FBleedingStack& Stack = BleedingStacks[Index];
		while (Stack.NextTickAt <= CurrentTimeSeconds && Stack.NextTickAt <= Stack.ExpiresAt && IsAlive())
		{
			ApplyFinalDamage(Stats.HpMax * Stack.DamageFraction, Stack.Attack, Stack.DamageSource);
			Stack.NextTickAt += 1.0f;
		}
		if (CurrentTimeSeconds >= Stack.ExpiresAt)
		{
			BleedingStacks.RemoveAt(Index);
		}
	}
	if (BleedingStacks.IsEmpty())
	{
		ElementState.ActiveStatusUntilSeconds.Remove(TEXT("Z_Bleeding"));
	}
	else
	{
		float LatestExpiry = 0.0f;
		for (const FBleedingStack& Stack : BleedingStacks)
		{
			LatestExpiry = FMath::Max(LatestExpiry, Stack.ExpiresAt);
		}
		ElementState.ActiveStatusUntilSeconds.Add(TEXT("Z_Bleeding"), LatestExpiry);
	}
	for (int32 Index = TransientStatStacks.Num() - 1; Index >= 0; --Index)
	{
		if (CurrentTimeSeconds >= TransientStatStacks[Index].ExpiresAt)
		{
			RemoveTransientStatStack(Index);
		}
	}
	RefreshTickState();
}

void UReEchoCombatantComponent::PublishElementStateChange()
{
	AActor* Owner = GetOwner();
	UReEchoCombatEventsComponent* Events =
	    Owner ? Owner->FindComponentByClass<UReEchoCombatEventsComponent>() : nullptr;
	if (!Events)
	{
		return;
	}
	FReEchoElementStateChangedEvent Event;
	Event.Combatant = this;
	Event.State = ElementState;
	Events->PublishElementStateChanged(Event);
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
	Stats.AttackSpeed = FMath::Min(Attributes->GetAttackSpeed(), FReEchoStatBlock::MaxAttackSpeedMultiplier);
	Stats.MovementSpeed = Attributes->GetMovementSpeed();
	Stats.EchoEfficiency = Attributes->GetEchoEfficiency();
	OnHealthChanged.Broadcast(GetEffectiveCurrentHealth(), Stats.HpMax);
}

void UReEchoCombatantComponent::HandleHealthChanged(const FOnAttributeChangeData& Data)
{
	CurrentHealth = Data.NewValue;
	if (bDeferHealthNotifications)
	{
		return;
	}
	OnHealthChanged.Broadcast(GetEffectiveCurrentHealth(), Stats.HpMax);
	PublishHealthChange(Data.OldValue);
	if (Data.OldValue > 0.0f && Data.NewValue <= 0.0f && !bDeathBroadcast)
	{
		// WS4 (Plan 68): defer a lethal hit if the enemy opts into a blood-depleted phase transition.
		if (TryDeferFatalDamageForPhaseTransition(CurrentHealth))
		{
			return;
		}
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
	if (bDeferHealthNotifications)
	{
		return;
	}
	OnHealthChanged.Broadcast(GetEffectiveCurrentHealth(), Stats.HpMax);
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
	Stats.AttackSpeed = FMath::Min(Data.NewValue, FReEchoStatBlock::MaxAttackSpeedMultiplier);
}

void UReEchoCombatantComponent::HandleMovementSpeedChanged(const FOnAttributeChangeData& Data)
{
	Stats.MovementSpeed = Data.NewValue;
}

void UReEchoCombatantComponent::HandleEchoEfficiencyChanged(const FOnAttributeChangeData& Data)
{
	Stats.EchoEfficiency = Data.NewValue;
}

void UReEchoCombatantComponent::PublishHealthChange(const float PreviousHealth)
{
	if (AActor* Owner = GetOwner())
	{
		if (UReEchoCombatEventsComponent* Events = Owner->FindComponentByClass<UReEchoCombatEventsComponent>())
		{
			FReEchoHealthChangedEvent Event;
			Event.Combatant = this;
			Event.PreviousHealth = PreviousHealth;
			Event.CurrentHealth = GetEffectiveCurrentHealth();
			Event.MaximumHealth = Stats.HpMax;
			Event.Reason = HealthChangeReason;
			Event.Attack = HealthChangeAttack;
			Event.DamageSource = HealthChangeDamageSource;
			Events->PublishHealthChanged(Event);
		}
	}
}

void UReEchoCombatantComponent::RecordEffectivePlayerEchoDamageSource(const EReEchoDamageSource DamageSource)
{
	if (DamageSource != EReEchoDamageSource::Player && DamageSource != EReEchoDamageSource::Echo)
	{
		return;
	}
	bHasLastPlayerEchoDamageSource = true;
	LastPlayerEchoDamageSource = DamageSource;
}
