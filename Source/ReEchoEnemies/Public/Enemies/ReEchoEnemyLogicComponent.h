#pragma once

#include "Combat/ReEchoCombatContracts.h"
#include "Components/ActorComponent.h"
#include "Enemies/ReEchoEnemyTypes.h"
#include "ReEchoEnemyLogicComponent.generated.h"

class UReEchoCombatEventsComponent;
class UReEchoEnemyEventsComponent;

/** Authoritative, presentation-free enemy behavior. The host supplies sense input and applies returned intents. */
UCLASS(ClassGroup = (ReEcho), meta = (BlueprintSpawnableComponent))

class REECHOENEMIES_API UReEchoEnemyLogicComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UReEchoEnemyLogicComponent();

	bool Initialize(const FReEchoEnemyDefinition& InDefinition, int32 InSpawnIndex);
	void BindEventSources(UReEchoEnemyEventsComponent* InEnemyEvents, UReEchoCombatEventsComponent* InCombatEvents);
	FReEchoEnemyActionIntent Advance(const FReEchoEnemySenseSnapshot& Sense, float DeltaSeconds);

	void NotifyHurt(float AppliedDamage, const FVector& SourceLocation, const FVector& SelfLocation);
	/** Counts one actual health-reduction event for the optional phase trigger, then applies hurt reaction. */
	void NotifyReceivedAttack(const FReEchoDamageEvent& Event);
	void NotifyDeath();
	/** Ends attack, hit-reaction and fuse phases without resetting identity, health or persistent cooldowns. */
	void ResetEncounterTransientState();
	void RestoreSnapshot(const FReEchoEnemyLogicSnapshot& InSnapshot);

	FReEchoEnemyLogicSnapshot GetSnapshot() const;
	const FReEchoEnemyDefinition& GetDefinition() const;

	bool IsInitialized() const
	{
		return bInitialized;
	}

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleCombatHurt(const FReEchoDamageEvent& Event);

	UFUNCTION()
	void HandleCombatDeath(const FReEchoDamageEvent& Event);

	void PublishFuse(bool bStarted) const;
	void PublishAction(const FReEchoEnemyActionIntent& Intent) const;
	void PublishBossIntent(const FReEchoBossIntent& Intent) const;
	FReEchoEnemyActionIntent AdvanceHitReaction(float DeltaSeconds);
	FReEchoEnemyActionIntent AdvanceBoss(const FReEchoEnemySenseSnapshot& Sense, float DeltaSeconds);
	FReEchoEnemyActionIntent AdvanceSpecial(const FReEchoEnemySenseSnapshot& Sense, float DeltaSeconds);
	const FReEchoEnemyAbilityDefinition* GetSpecialAbility() const;
	void AdvanceBossFixedStep(const FReEchoEnemySenseSnapshot& Sense,
	                          float FixedDeltaSeconds,
	                          FReEchoEnemyActionIntent& InOutIntent);
	void AdvanceBossAmbientTimers(float FixedDeltaSeconds, FReEchoEnemyActionIntent& InOutIntent);
	void AdvanceBossAbility(const FReEchoEnemySenseSnapshot& Sense,
	                        float FixedDeltaSeconds,
	                        FReEchoEnemyActionIntent& InOutIntent);
	void
	BeginBossAbility(const FReEchoEnemySenseSnapshot& Sense, int32 AbilityIndex, FReEchoEnemyActionIntent& InOutIntent);
	void LockBossTarget(const FReEchoEnemySenseSnapshot& Sense);
	void CommitBossAbility(const FReEchoEnemySenseSnapshot& Sense,
	                       const FReEchoEnemyAbilityDefinition& Ability,
	                       FReEchoEnemyActionIntent& InOutIntent);
	void EndBossAbility(const FReEchoEnemyAbilityDefinition& Ability, FReEchoEnemyActionIntent& InOutIntent);
	void AppendBossIntent(FReEchoBossIntent&& BossIntent, FReEchoEnemyActionIntent& InOutIntent);
	int32 SelectBossAbility(const FReEchoEnemySenseSnapshot& Sense) const;
	int32 FindBossAbilityIndex(FName AbilityId) const;
	float GetBossAbilityCooldown(FName AbilityId) const;
	void SetBossAbilityCooldown(FName AbilityId, float RemainingSeconds);
	void ApplyBossHitReaction(float FixedDeltaSeconds, FReEchoEnemyActionIntent& InOutIntent);
	void ApplyStandardMovement(const FReEchoEnemySenseSnapshot& Sense,
	                           float DeltaSeconds,
	                           FReEchoEnemyActionIntent& InOutIntent);
	bool TryBeginPhaseTransition(const FReEchoEnemySenseSnapshot& Sense, FReEchoEnemyActionIntent& InOutIntent);
	FReEchoEnemyActionIntent AdvancePhaseTransition(float DeltaSeconds);
	void CancelUncommittedActionsForPhaseTransition();
	bool BuildBossRuntime();
	void CommitAttack(const FReEchoEnemySenseSnapshot& Sense,
	                  bool bCanDamageTarget,
	                  bool bSelfDestruct,
	                  FReEchoEnemyActionIntent& InOutIntent);

	UPROPERTY()
	TObjectPtr<UReEchoEnemyEventsComponent> EnemyEvents;

	UPROPERTY()
	TObjectPtr<UReEchoCombatEventsComponent> CombatEvents;

	FReEchoEnemyDefinition Definition;
	FReEchoEnemyLogicSnapshot State;
	TArray<int32> BossActiveAbilityIndices;
	TArray<int32> BossPhaseIndices;
	int32 BossCleanseAbilityIndex = INDEX_NONE;
	bool bInitialized = false;
};
