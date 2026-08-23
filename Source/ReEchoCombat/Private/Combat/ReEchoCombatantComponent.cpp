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
	const float PreviousHealth = CurrentHealth;
	bDeathBroadcast = false;
	HealthChangeReason = TEXT("Initialize");
	HealthChangeAttack = {};
	if (BoundAbilitySystem)
	{
		BoundAbilitySystem->RemoveLooseGameplayTag(ReEchoGameplayTags::State_Dead);
	}
	if (BoundAbilitySystem)
	{
		ReEchoGameplayEffects::ApplyInitialization(*BoundAbilitySystem, InStats, bFillHealth);
		SyncFromAbilitySystem();
		HealthChangeReason = NAME_None;
		return;
	}
	Stats = InStats;
	CurrentHealth = bFillHealth ? Stats.HpMax : FMath::Min(CurrentHealth, Stats.HpMax);
	OnHealthChanged.Broadcast(CurrentHealth, Stats.HpMax);
	PublishHealthChange(PreviousHealth);
	HealthChangeReason = NAME_None;
}

float UReEchoCombatantComponent::ApplyFinalDamage(const float Damage,
                                                  const FReEchoAttackIdentity& Attack,
                                                  const EReEchoDamageSource DamageSource)
{
	if (!IsAlive() || Damage <= 0.f || bDebugInvulnerable)
	{
		return 0.f;
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
		const float Applied = ReEchoGameplayEffects::ApplyDamage(SourceAbilitySystem, *BoundAbilitySystem, Damage);
		HealthChangeReason = NAME_None;
		HealthChangeAttack = {};
		return Applied;
	}
	if (Stats.Block > 0)
	{
		--Stats.Block;
		return 0.f;
	}
	const float Applied = FMath::Min(CurrentHealth, FMath::Max(1.f, Damage));
	const float PreviousHealth = CurrentHealth;
	HealthChangeReason = TEXT("Damage");
	HealthChangeAttack = Attack;
	HealthChangeDamageSource = DamageSource;
	CurrentHealth -= Applied;
	OnHealthChanged.Broadcast(CurrentHealth, Stats.HpMax);
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
	return Applied;
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

float UReEchoCombatantComponent::ApplyHealing(const float Healing)
{
	if (!IsAlive() || Healing <= 0.0f)
	{
		return 0.0f;
	}
	if (BoundAbilitySystem)
	{
		HealthChangeReason = TEXT("Healing");
		HealthChangeAttack = {};
		const float Applied = ReEchoGameplayEffects::ApplyHealing(nullptr, *BoundAbilitySystem, Healing);
		HealthChangeReason = NAME_None;
		return Applied;
	}
	const float PreviousHealth = CurrentHealth;
	HealthChangeReason = TEXT("Healing");
	HealthChangeAttack = {};
	CurrentHealth = FMath::Clamp(CurrentHealth + Healing, 0.0f, Stats.HpMax);
	OnHealthChanged.Broadcast(CurrentHealth, Stats.HpMax);
	PublishHealthChange(PreviousHealth);
	HealthChangeReason = NAME_None;
	return CurrentHealth - PreviousHealth;
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
	const float PreviousHealth = CurrentHealth;
	HealthChangeReason = TEXT("Restore");
	HealthChangeAttack = {};
	const float ClampedHealth = FMath::Clamp(SavedHealth, 0.0f, Stats.HpMax);
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
		HealthChangeReason = NAME_None;
		return;
	}
	CurrentHealth = ClampedHealth;
	OnHealthChanged.Broadcast(CurrentHealth, Stats.HpMax);
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
	if (BoundAbilitySystem && InOutHealth <= 0.0f)
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
	Snapshot.CurrentHealth = CurrentHealth;
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
	    ElementState.BurnSourceLocation != FVector::ZeroVector || !ElementState.BurnAttack.Source.IsExplicitlyNull() ||
	    ElementState.BurnAttack.Sequence != 0;
	Result.bClearedAttachment = ElementState.Attached != EReEchoElement::None;
	Result.bClearedBurn = bHadBurnState;

	ElementState.Attached = EReEchoElement::None;
	ElementState.bBurnActive = false;
	ElementState.BurnTickDamage = 0.0f;
	ElementState.BurnNextTickTimeSeconds = 0.0f;
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
#endif

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
	Stats.AttackSpeed = Attributes->GetAttackSpeed();
	Stats.MovementSpeed = Attributes->GetMovementSpeed();
	Stats.EchoEfficiency = Attributes->GetEchoEfficiency();
	OnHealthChanged.Broadcast(CurrentHealth, Stats.HpMax);
}

void UReEchoCombatantComponent::HandleHealthChanged(const FOnAttributeChangeData& Data)
{
	CurrentHealth = Data.NewValue;
	OnHealthChanged.Broadcast(CurrentHealth, Stats.HpMax);
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

void UReEchoCombatantComponent::PublishHealthChange(const float PreviousHealth)
{
	if (AActor* Owner = GetOwner())
	{
		if (UReEchoCombatEventsComponent* Events = Owner->FindComponentByClass<UReEchoCombatEventsComponent>())
		{
			FReEchoHealthChangedEvent Event;
			Event.Combatant = this;
			Event.PreviousHealth = PreviousHealth;
			Event.CurrentHealth = CurrentHealth;
			Event.MaximumHealth = Stats.HpMax;
			Event.Reason = HealthChangeReason;
			Event.Attack = HealthChangeAttack;
			Event.DamageSource = HealthChangeDamageSource;
			Events->PublishHealthChanged(Event);
		}
	}
}
