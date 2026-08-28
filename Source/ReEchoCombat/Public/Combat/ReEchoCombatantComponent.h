#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/ReEchoCombatContracts.h"
#include "Combat/ReEchoCombatTypes.h"
#include "GameplayEffectTypes.h"
#include "ReEchoCombatantComponent.generated.h"

class UAbilitySystemComponent;
struct FOnAttributeChangeData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FReEchoHealthChanged, float, Current, float, Maximum);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FReEchoDeath);

/**
 * WS4 (Plan 68): fatal-damage pre-hook. Invoked the moment an attack would reduce health to zero or below,
 * before OnDeath is broadcast. If the bound delegate returns true, the death is deferred (health is clamped to a
 * survivable value instead of being killed) so the enemy can enter its second phase. Returns false by default,
 * preserving normal death for every unit that does not opt in.
 */
DECLARE_DELEGATE_RetVal_OneParam(bool, FReEchoFatalDamageIntercept, float&);

/** Compatibility facade over GAS combat attributes; legacy fallback remains for actors not migrated to ASC. */
UCLASS(ClassGroup = (ReEcho), meta = (BlueprintSpawnableComponent))

class REECHOCOMBAT_API UReEchoCombatantComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	bool HasLastPlayerEchoDamageSource() const
	{
		return bHasLastPlayerEchoDamageSource;
	}

	EReEchoDamageSource GetLastPlayerEchoDamageSource() const
	{
		return LastPlayerEchoDamageSource;
	}

	void RecordEffectivePlayerEchoDamageSource(EReEchoDamageSource DamageSource);
	UReEchoCombatantComponent();

	/** Read-only compatibility snapshot. GAS is authoritative whenever an ASC is bound. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	FReEchoStatBlock Stats;

	/** Read-only compatibility value. GAS is authoritative whenever an ASC is bound. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	float CurrentHealth = 100.f;

	UPROPERTY(BlueprintAssignable)
	FReEchoHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable)
	FReEchoDeath OnDeath;

	UFUNCTION(BlueprintCallable)
	void InitializeFromStats(const FReEchoStatBlock& InStats, bool bFillHealth = true);

	/** Applies healing through GAS when available and returns actual health restored. */
	UFUNCTION(BlueprintCallable)
	float ApplyHealing(float Healing);
	/** Allows healing to create a damage-absorbing health buffer above HpMax. Zero disables and clears the buffer. */
	void SetOverhealCapacityFraction(float Fraction);

	float GetOverhealth() const
	{
		return Overhealth;
	}

	float GetEffectiveCurrentHealth() const
	{
		return CurrentHealth + Overhealth;
	}

	/** Atomically applies a permanent maximum-health change and its typed current-health adjustment. */
	UFUNCTION(BlueprintCallable)
	bool ApplyHealthAdjustment(float NewMaximumHealth, EReEchoHealthAdjustment Adjustment);
	/** Restore serialized health without producing damage/heal feedback or consuming block. */
	void RestoreCurrentHealth(float SavedHealth);

	/**
	 * WS4 (Plan 68): optional fatal-damage intercept hook. When bound and returning true, a lethal hit is converted
	 * into a survivable state (death deferred) so the enemy can transition to its second phase instead of dying.
	 * The hook receives the incoming raw damage and the resulting post-damage health (would be <= 0).
	 */
	void SetFatalDamageInterceptDelegate(const FReEchoFatalDamageIntercept& InDelegate)
	{
		OnFatalDamage = InDelegate;
	}

	UFUNCTION(BlueprintPure)
	bool IsAlive() const;
	/** Development-only GM gate: reports resolved damage to presentation while preserving health. */
	void SetDebugInvulnerable(bool bEnabled);
	bool IsDebugInvulnerable() const;

	void BindToAbilitySystem(UAbilitySystemComponent* InAbilitySystem);
	UAbilitySystemComponent* GetBoundAbilitySystem() const;
	UFUNCTION(BlueprintPure)
	FReEchoCombatantSnapshot GetSnapshot() const;
	const FReEchoElementState& GetElementState() const;
	/** Clears attachment and burn, then grants immunity without shortening an existing immunity window. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Element")
	FReEchoElementCleanseResult ExecuteElementCleanse(const FReEchoElementCleanseCommand& Command);
	bool ApplyTimedStatus(const FReEchoTimedStatusCommand& Command);
	/** Bosses reject Z_Cursed at the Combat boundary; reused enemy actors set this on every configure. */
	void SetCursedImmune(bool bImmune);
	/** Rejects Z_Vertigo at the Combat authority boundary and clears any active stun when immunity is enabled. */
	void SetStunImmune(bool bImmune);

	bool IsStunImmune() const
	{
		return bStunImmune;
	}

	bool IsActionDisabled(float CurrentTimeSeconds) const;
	void GrantTimedInvulnerability(float CurrentTimeSeconds, float DurationSeconds);
	bool IsTimedInvulnerable(float CurrentTimeSeconds) const;
	void AddTransientStatModifier(FName SourceId,
	                              float AttackSpeedBonusFraction,
	                              float MovementSpeedBonusFraction,
	                              float DurationSeconds,
	                              int32 MaxStacks);
	bool RemoveOldestTransientStatModifier(FName SourceId);
	int32 GetTransientStatStackCount(FName SourceId) const;
	/** Host-agnostic runtime command: replace one source's additive attack modifier without cumulative drift. */
	void SetAdditiveAttackModifier(FName SourceId, float PhysicalAttackBonus, float ElementalAttackBonus);
	/** Card-agnostic host command: shorten an active immunity window without extending it. */
	void ClampElementImmunityDuration(float CurrentTimeSeconds, float MaximumRemainingSeconds);
	/** Save/continue migration entry; runtime attacks must go through HitResolver. */
	void RestoreElementState(const FReEchoElementState& SavedState);
	void ResetElementState();
#if WITH_DEV_AUTOMATION_TESTS
	FReEchoElementState& EditElementStateForTests();
	float ApplyFinalDamageForTests(float Damage);
	void AdvanceTimedRuneEffectsForTests(float CurrentTimeSeconds);
#endif

protected:
	virtual void BeginPlay() override;
	virtual void
	TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void SyncFromAbilitySystem();
	void HandleHealthChanged(const FOnAttributeChangeData& Data);
	void HandleMaxHealthChanged(const FOnAttributeChangeData& Data);
	void HandleBlockChanged(const FOnAttributeChangeData& Data);
	void HandlePhysicalAttackChanged(const FOnAttributeChangeData& Data);
	void HandleElementalAttackChanged(const FOnAttributeChangeData& Data);
	void HandleAttackSpeedChanged(const FOnAttributeChangeData& Data);
	void HandleMovementSpeedChanged(const FOnAttributeChangeData& Data);
	void HandleEchoEfficiencyChanged(const FOnAttributeChangeData& Data);
	void PublishHealthChange(float PreviousHealth);
	void PublishElementStateChange();
	void RemoveTransientStatStack(int32 Index);
	void RefreshTickState();
	void AdvanceTimedRuntimeState(float CurrentTimeSeconds);
	float ApplyFinalDamage(float Damage, const FReEchoAttackIdentity& Attack, EReEchoDamageSource DamageSource);
	// WS4 (Plan 68): asks the optional fatal-damage hook to defer a lethal hit into a phase transition. On defer it
	// clamps InOutHealth to a survivable value (and corrects the GAS attribute) and returns true; callers then skip
	// OnDeath. Returns false when no hook is bound or the hook declines, leaving normal death flow to the caller.
	bool TryDeferFatalDamageForPhaseTransition(float& InOutHealth);

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> BoundAbilitySystem;

	FReEchoFatalDamageIntercept OnFatalDamage;

	bool bDeathBroadcast = false;
	bool bDebugInvulnerable = false;
	bool bCursedImmune = false;
	bool bStunImmune = false;
	bool bHasLastPlayerEchoDamageSource = false;
	EReEchoDamageSource LastPlayerEchoDamageSource = EReEchoDamageSource::Player;
	bool bDeferHealthNotifications = false;
	FReEchoElementState ElementState;

	struct FBleedingStack
	{
		float ExpiresAt = 0.0f;
		float NextTickAt = 0.0f;
		float DamageFraction = 0.0f;
		FReEchoAttackIdentity Attack;
		EReEchoDamageSource DamageSource = EReEchoDamageSource::Player;
	};

	struct FTransientStatStack
	{
		FName SourceId = NAME_None;
		float ExpiresAt = 0.0f;
		float AttackSpeedMultiplier = 1.0f;
		float MovementSpeedMultiplier = 1.0f;
		float FallbackAttackSpeedDelta = 0.0f;
		float FallbackMovementSpeedDelta = 0.0f;
		FActiveGameplayEffectHandle GameplayEffectHandle;
	};

	TArray<FBleedingStack> BleedingStacks;
	TArray<FTransientStatStack> TransientStatStacks;
	TMap<FName, FVector2D> AdditiveAttackModifiers;
	float StunnedUntilWorldTime = 0.0f;
	float InvulnerableUntilWorldTime = 0.0f;
	float OverhealCapacityFraction = 0.0f;
	float Overhealth = 0.0f;
	FName HealthChangeReason = NAME_None;
	FReEchoAttackIdentity HealthChangeAttack;
	EReEchoDamageSource HealthChangeDamageSource = EReEchoDamageSource::Player;

	friend class FReEchoElementResolverAccess;
	friend class FReEchoHitResolverAccess;
};
