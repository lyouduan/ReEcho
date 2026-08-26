#include "Presentation/Combat/ReEchoCombatPresentationCoordinator.h"

#include "Combat/ReEchoCombatContracts.h"

UReEchoCombatPresentationCoordinator::UReEchoCombatPresentationCoordinator()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UReEchoCombatPresentationCoordinator::IsWeaponTrackEnabled(const EReEchoPresentationHostKind HostKind,
                                                                const FName WeaponPresentationId)
{
	if (HostKind == EReEchoPresentationHostKind::OrdinaryEnemy)
	{
		return false;
	}
	return !WeaponPresentationId.IsNone();
}

bool UReEchoCombatPresentationCoordinator::CanAdvance(const EReEchoPresentationActionPhase Current,
                                                      const EReEchoPresentationActionPhase Next)
{
	if (Next == EReEchoPresentationActionPhase::Cancelled || Next == EReEchoPresentationActionPhase::Ended)
	{
		return Current != EReEchoPresentationActionPhase::Ended && Current != EReEchoPresentationActionPhase::Cancelled;
	}
	return static_cast<uint8>(Next) > static_cast<uint8>(Current) && Current != EReEchoPresentationActionPhase::Ended &&
	       Current != EReEchoPresentationActionPhase::Cancelled;
}

void UReEchoCombatPresentationCoordinator::BeginPlay()
{
	Super::BeginPlay();
	AActor* Owner = GetOwner();
	BindEventSources(Owner ? Owner->FindComponentByClass<UReEchoEnemyEventsComponent>() : nullptr,
	                 Owner ? Owner->FindComponentByClass<UReEchoCombatEventsComponent>() : nullptr);
}

void UReEchoCombatPresentationCoordinator::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	PublishTerminal(EReEchoPresentationActionPhase::Cancelled);
	BindEventSources(nullptr, nullptr);
	Super::EndPlay(EndPlayReason);
}

void UReEchoCombatPresentationCoordinator::BindEventSources(UReEchoEnemyEventsComponent* InEnemyEvents,
                                                            UReEchoCombatEventsComponent* InCombatEvents)
{
	if (EnemyEvents)
	{
		EnemyEvents->OnActionCommitted.RemoveAll(this);
		EnemyEvents->OnSpecialAction.RemoveAll(this);
	}
	if (CombatEvents)
	{
		CombatEvents->OnDeath.RemoveAll(this);
	}
	EnemyEvents = InEnemyEvents;
	CombatEvents = InCombatEvents;
	if (EnemyEvents)
	{
		EnemyEvents->OnActionCommitted.AddDynamic(this,
		                                          &UReEchoCombatPresentationCoordinator::HandleEnemyActionCommitted);
		EnemyEvents->OnSpecialAction.AddDynamic(this, &UReEchoCombatPresentationCoordinator::HandleSpecialAction);
	}
	if (CombatEvents)
	{
		CombatEvents->OnDeath.AddDynamic(this, &UReEchoCombatPresentationCoordinator::HandleDeath);
	}
}

void UReEchoCombatPresentationCoordinator::HandleEnemyActionCommitted(const FReEchoEnemyActionCommittedEvent& Event)
{
	if (!bHasActiveAction)
	{
		ActiveAction = {};
		ActiveAction.Key.Source = Event.Attack.Source.IsValid() ? Event.Attack.Source.Get() : GetOwner();
		ActiveAction.Key.Sequence = Event.Attack.Sequence;
		ActiveAction.Key.AbilityId = TEXT("Enemy.Basic");
		bHasActiveAction = true;
	}
	else if (!CanAdvance(ActiveAction.Phase, EReEchoPresentationActionPhase::Committed))
	{
		return;
	}
	ActiveAction.Phase = EReEchoPresentationActionPhase::Committed;
	ActiveAction.Attack = Event.Attack;
	ActiveAction.Origin = Event.Origin;
#if WITH_DEV_AUTOMATION_TESTS
	++PublishedPhaseCountForTests;
#endif
	OnActionPhase.Broadcast(ActiveAction);
	if (ActiveAction.Key.AbilityId == TEXT("Enemy.Basic"))
	{
		bHasActiveAction = false;
	}
}

void UReEchoCombatPresentationCoordinator::HandleSpecialAction(const FReEchoEnemySpecialActionEvent& Event)
{
	EReEchoPresentationActionPhase NextPhase = EReEchoPresentationActionPhase::Windup;
	if (Event.Type == EReEchoEnemySpecialActionEventType::ActionCommitted)
	{
		NextPhase = EReEchoPresentationActionPhase::Committed;
	}
	else if (Event.Type == EReEchoEnemySpecialActionEventType::RecoveryStarted)
	{
		NextPhase = EReEchoPresentationActionPhase::Recovery;
	}
	else if (Event.Type == EReEchoEnemySpecialActionEventType::ActionEnded)
	{
		NextPhase = EReEchoPresentationActionPhase::Ended;
	}
	else if (Event.Type == EReEchoEnemySpecialActionEventType::ActionCancelled)
	{
		NextPhase = EReEchoPresentationActionPhase::Cancelled;
	}

	const bool bSameAction = bHasActiveAction && ActiveAction.Key.AbilityId == Event.AbilityId;
	if (NextPhase == EReEchoPresentationActionPhase::Windup)
	{
		if (bSameAction && ActiveAction.Phase == NextPhase)
		{
			return;
		}
		if (bHasActiveAction)
		{
			PublishTerminal(EReEchoPresentationActionPhase::Cancelled);
		}
		ActiveAction = {};
		ActiveAction.Key.Source = Event.Attack.Source.IsValid() ? Event.Attack.Source.Get() : GetOwner();
		ActiveAction.Key.Sequence = Event.Attack.IsValid() ? Event.Attack.Sequence : --SyntheticSequence;
		ActiveAction.Key.AbilityId = Event.AbilityId;
		bHasActiveAction = true;
	}
	else if (!bSameAction || !CanAdvance(ActiveAction.Phase, NextPhase))
	{
		return;
	}

	ActiveAction.Phase = NextPhase;
	ActiveAction.Attack = Event.Attack;
	ActiveAction.Origin = Event.Origin;
	ActiveAction.LockedDirection = Event.LockedDirection;
	ActiveAction.LockedTargetLocation = Event.LockedTargetLocation;
#if WITH_DEV_AUTOMATION_TESTS
	++PublishedPhaseCountForTests;
#endif
	OnActionPhase.Broadcast(ActiveAction);
	if (NextPhase == EReEchoPresentationActionPhase::Ended || NextPhase == EReEchoPresentationActionPhase::Cancelled)
	{
		bHasActiveAction = false;
	}
}

void UReEchoCombatPresentationCoordinator::HandleDeath(const FReEchoDamageEvent& Event)
{
	if (Event.Target == GetOwner())
	{
		PublishTerminal(EReEchoPresentationActionPhase::Cancelled);
	}
}

void UReEchoCombatPresentationCoordinator::PublishTerminal(const EReEchoPresentationActionPhase Phase)
{
	if (!bHasActiveAction || !CanAdvance(ActiveAction.Phase, Phase))
	{
		return;
	}
	ActiveAction.Phase = Phase;
#if WITH_DEV_AUTOMATION_TESTS
	++PublishedPhaseCountForTests;
#endif
	OnActionPhase.Broadcast(ActiveAction);
	bHasActiveAction = false;
}
