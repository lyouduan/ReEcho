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
	/** Commit a non-Boss phase after its externally orchestrated transformation has finished. */
	bool CompleteCinematicPhase2(FReEchoEnemyActionIntent& OutIntent);
	// WS4 (Plan 68): attempts to convert a just-lethal hit into a blood-depleted second-phase transition. Returns
	// true when the owner is a HealthThreshold boss that has not yet transformed (the caller defers real death).
	bool TryTriggerPhase2OnFatalWound(FReEchoEnemyActionIntent& OutIntent);
	/** Converts a Phase2 fatal wound into Phase3 when inside the fast-kill window or an Easter card is owned. */
	bool TryTriggerPhase3OnFatalWound(FReEchoEnemyActionIntent& OutIntent, bool bHasEasterEggCard = false);
	/** Read-only qualification for a Host-owned transformation presentation. */
	int32 ResolveFatalBossTransitionPhase(bool bHasEasterEggCard) const;
	/** Cancels the current target-locked attack when stun begins, preserving cooldowns, fuse and hit reaction. */
	void CancelActiveActionsForStun();
	/** Ends attack, hit-reaction and fuse phases without resetting identity, health or persistent cooldowns. */
	void ResetEncounterTransientState();
	void RestoreSnapshot(const FReEchoEnemyLogicSnapshot& InSnapshot);
	/** Host feedback after applying one Active Elite dash step through swept world movement. */
	void ResolveSpecialDashStep(bool bMovementBlocked, bool bDamageContactConsumed);

	FReEchoEnemyLogicSnapshot GetSnapshot() const;
	const FReEchoEnemyDefinition& GetDefinition() const;
	/** Development command: queues one configured Boss ability for the next normal ability start. */
	bool DebugQueueBossAbility(FName AbilityId, int32 ForcedComboCount = 0);
	/** Production one-shot command, issued only after a phase-three cinematic completes. */
	bool QueueBossPhaseOpening();
	/** Development command: immediately switches to one configured encounter phase. */
	bool DebugForceBossPhase(int32 PhaseIndex, FReEchoEnemyActionIntent& OutIntent);
	bool DebugSetBossEncounterElapsedSeconds(float Seconds);

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
	FReEchoEnemyActionIntent AdvanceIdleWander(const FReEchoEnemySenseSnapshot& Sense, float DeltaSeconds);
	FReEchoEnemyActionIntent AdvanceBoss(const FReEchoEnemySenseSnapshot& Sense, float DeltaSeconds);
	FReEchoEnemyActionIntent AdvanceSpecial(const FReEchoEnemySenseSnapshot& Sense, float DeltaSeconds);
	bool BuildSpecialRuntime();
	const FReEchoEnemyAbilityDefinition* GetNextSpecialAbility() const;
	const FReEchoEnemyAbilityDefinition* FindSpecialAbility(FName AbilityId) const;
	void BeginSpecialRecovery(const FReEchoEnemyAbilityDefinition& Ability);
	void ClearSpecialAction();
	void CancelSpecialAction();
	void AdvanceBossFixedStep(const FReEchoEnemySenseSnapshot& Sense,
	                          float FixedDeltaSeconds,
	                          FReEchoEnemyActionIntent& InOutIntent);
	void AdvanceBossAmbientTimers(const FReEchoEnemySenseSnapshot& Sense,
	                              float FixedDeltaSeconds,
	                              FReEchoEnemyActionIntent& InOutIntent);
	void AdvanceBossAbility(const FReEchoEnemySenseSnapshot& Sense,
	                        float FixedDeltaSeconds,
	                        FReEchoEnemyActionIntent& InOutIntent);
	void
	BeginBossAbility(const FReEchoEnemySenseSnapshot& Sense, int32 AbilityIndex, FReEchoEnemyActionIntent& InOutIntent);
	void LockBossTarget(const FReEchoEnemySenseSnapshot& Sense);
	void CommitBossAbility(const FReEchoEnemySenseSnapshot& Sense,
	                       const FReEchoEnemyAbilityDefinition& Ability,
	                       FReEchoEnemyActionIntent& InOutIntent);
	void BeginNextBossComboStrike(const FReEchoEnemySenseSnapshot& Sense,
	                              const FReEchoEnemyAbilityDefinition& Ability,
	                              FReEchoEnemyActionIntent& InOutIntent);
	void EndBossAbility(const FReEchoEnemyAbilityDefinition& Ability, FReEchoEnemyActionIntent& InOutIntent);
	bool IsBossComboAbility(const FReEchoEnemyAbilityDefinition& Ability) const;
	void AppendBossIntent(FReEchoBossIntent&& BossIntent, FReEchoEnemyActionIntent& InOutIntent);
	int32 SelectBossAbility(const FReEchoEnemySenseSnapshot& Sense);
	int32 FindBossAbilityIndex(FName AbilityId) const;
	float GetBossAbilityCooldown(FName AbilityId) const;
	void SetBossAbilityCooldown(FName AbilityId, float RemainingSeconds);
	const FReEchoBossPhaseDefinition* GetCurrentBossPhaseDefinition() const;
	float ResolveCurrentBossPhysicalDamage(float BaseDamage) const;
	float ResolveCurrentBossCooldown(float BaseCooldownSeconds) const;
	void ApplyBossHitReaction(float FixedDeltaSeconds, FReEchoEnemyActionIntent& InOutIntent);
	void ApplyStandardMovement(const FReEchoEnemySenseSnapshot& Sense,
	                           float DeltaSeconds,
	                           FReEchoEnemyActionIntent& InOutIntent);
	bool TryBeginPhaseTransition(const FReEchoEnemySenseSnapshot& Sense, FReEchoEnemyActionIntent& InOutIntent);
	// WS4 (Plan 68): true when the given current-health ratio is at or below the configured Phase2 threshold
	// (blood-bar depleted). The host samples health and passes it in; enemy logic never reads the combat component.
	bool IsHealthAtOrBelowPhase2Threshold(float CurrentHealthRatio) const;
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
	TArray<int32> SpecialActiveAbilityIndices;
	TArray<int32> BossActiveAbilityIndices;
	TArray<int32> BossPhaseIndices;
	int32 BossCleanseAbilityIndex = INDEX_NONE;
	FName DebugQueuedBossAbilityId = NAME_None;
	int32 DebugForcedComboCount = 0;
	bool bInitialized = false;
};
