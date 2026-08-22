#pragma once

#include "Combat/ReEchoCombatContracts.h"
#include "Components/ActorComponent.h"
#include "Enemies/ReEchoEnemyEventsComponent.h"
#include "Presentation/Combat/ReEchoCombatPresentationTypes.h"
#include "ReEchoCombatPresentationCoordinator.generated.h"

class UReEchoCombatEventsComponent;

/** Normalizes gameplay events into one ordered, deduplicated presentation action lifecycle. */
UCLASS(ClassGroup = (ReEcho), meta = (BlueprintSpawnableComponent))

class REECHO_API UReEchoCombatPresentationCoordinator : public UActorComponent
{
	GENERATED_BODY()

public:
	UReEchoCombatPresentationCoordinator();
	UPROPERTY(BlueprintAssignable) FReEchoPresentationActionDelegate OnActionPhase;
	static bool IsWeaponTrackEnabled(EReEchoPresentationHostKind HostKind, FName WeaponPresentationId);
	static bool CanAdvance(EReEchoPresentationActionPhase Current, EReEchoPresentationActionPhase Next);
#if WITH_DEV_AUTOMATION_TESTS
	void ConsumeEnemyActionCommittedForTests(const FReEchoEnemyActionCommittedEvent& Event)
	{
		HandleEnemyActionCommitted(Event);
	}

	void ConsumeSpecialActionForTests(const FReEchoEnemySpecialActionEvent& Event)
	{
		HandleSpecialAction(Event);
	}

	bool HasActiveActionForTests() const
	{
		return bHasActiveAction;
	}

	int32 GetPublishedPhaseCountForTests() const
	{
		return PublishedPhaseCountForTests;
	}

	EReEchoPresentationActionPhase GetActivePhaseForTests() const
	{
		return ActiveAction.Phase;
	}
#endif

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void BindEventSources(UReEchoEnemyEventsComponent* InEnemyEvents, UReEchoCombatEventsComponent* InCombatEvents);
	void PublishTerminal(EReEchoPresentationActionPhase Phase);
	UFUNCTION() void HandleEnemyActionCommitted(const FReEchoEnemyActionCommittedEvent& Event);
	UFUNCTION() void HandleSpecialAction(const FReEchoEnemySpecialActionEvent& Event);
	UFUNCTION() void HandleDeath(const FReEchoDamageEvent& Event);
	UPROPERTY() TObjectPtr<UReEchoEnemyEventsComponent> EnemyEvents;
	UPROPERTY() TObjectPtr<UReEchoCombatEventsComponent> CombatEvents;
	FReEchoPresentationActionEvent ActiveAction;
	bool bHasActiveAction = false;
	int64 SyntheticSequence = 0;
#if WITH_DEV_AUTOMATION_TESTS
	int32 PublishedPhaseCountForTests = 0;
#endif
};
