#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/ReEchoCombatContracts.h"
#include "Combat/ReEchoCombatTypes.h"
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

	/** Applies final damage through GAS when available and returns actual health removed. */
	UFUNCTION(BlueprintCallable)
	float ApplyFinalDamage(float Damage);

	/** Applies healing through GAS when available and returns actual health restored. */
	UFUNCTION(BlueprintCallable)
	float ApplyHealing(float Healing);
	/** Restore serialized health without producing damage/heal feedback or consuming block. */
	void RestoreCurrentHealth(float SavedHealth);

	UFUNCTION(BlueprintPure)
	bool IsAlive() const;

	void BindToAbilitySystem(UAbilitySystemComponent* InAbilitySystem);
	UAbilitySystemComponent* GetBoundAbilitySystem() const;
	UFUNCTION(BlueprintPure)
	FReEchoCombatantSnapshot GetSnapshot() const;

protected:
	virtual void BeginPlay() override;

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
	void PublishDeath();

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> BoundAbilitySystem;

	bool bDeathBroadcast = false;
};
