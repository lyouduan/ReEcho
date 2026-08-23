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

/** Compatibility facade over GAS combat attributes; legacy fallback remains for actors not migrated to ASC. */
UCLASS(ClassGroup = (ReEcho), meta = (BlueprintSpawnableComponent))

class REECHOCOMBAT_API UReEchoCombatantComponent : public UActorComponent
{
	GENERATED_BODY()

public:
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
	/** Restore serialized health without producing damage/heal feedback or consuming block. */
	void RestoreCurrentHealth(float SavedHealth);

	UFUNCTION(BlueprintPure)
	bool IsAlive() const;
	/** Development-only final-damage gate used by player GM testing. */
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

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> BoundAbilitySystem;

	bool bDeathBroadcast = false;
	bool bDebugInvulnerable = false;
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
	float StunnedUntilWorldTime = 0.0f;
	float InvulnerableUntilWorldTime = 0.0f;
	FName HealthChangeReason = NAME_None;
	FReEchoAttackIdentity HealthChangeAttack;
	EReEchoDamageSource HealthChangeDamageSource = EReEchoDamageSource::Player;

	friend class FReEchoElementResolverAccess;
	friend class FReEchoHitResolverAccess;
};
