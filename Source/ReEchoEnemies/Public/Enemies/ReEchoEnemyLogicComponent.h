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
	void NotifyDeath();
	void RestoreSnapshot(const FReEchoEnemyLogicSnapshot& InSnapshot);

	FReEchoEnemyLogicSnapshot GetSnapshot() const;
	const FReEchoEnemyDefinition& GetDefinition() const;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleCombatHurt(const FReEchoDamageEvent& Event);

	UFUNCTION()
	void HandleCombatDeath(const FReEchoDamageEvent& Event);

	void PublishFuse(bool bStarted) const;
	void PublishAction(const FReEchoEnemyActionIntent& Intent) const;
	FReEchoEnemyActionIntent AdvanceHitReaction(float DeltaSeconds);
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
	bool bInitialized = false;
};
