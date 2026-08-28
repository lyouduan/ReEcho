#include "Combat/ReEchoCombatAudioAdapterComponent.h"

#include "ReEchoAudioEvents.h"
#include "ReEchoAudioService.h"
#include "ReEchoAudioTypes.h"

#include "Combat/ReEchoCombatTarget.h"
#include "Combat/ReEchoElementReaction.h"

namespace
{
EReEchoAudioSourceCategory ToAudioSourceCategory(const EReEchoCombatAudioSource Source)
{
	switch (Source)
	{
		case EReEchoCombatAudioSource::Enemy:
			return EReEchoAudioSourceCategory::Enemy;
		case EReEchoCombatAudioSource::Boss:
			return EReEchoAudioSourceCategory::Boss;
		case EReEchoCombatAudioSource::Echo:
			return EReEchoAudioSourceCategory::Echo;
		default:
			return EReEchoAudioSourceCategory::Player;
	}
}
}

UReEchoCombatAudioAdapterComponent::UReEchoCombatAudioAdapterComponent()
    : AttackEventId(FReEchoAudioEvents::CombatAttack), DeathEventId(FReEchoAudioEvents::CombatDeath)
{
}

void UReEchoCombatAudioAdapterComponent::ConfigureRouting(const EReEchoCombatAudioSource InSource,
                                                          const FName InAttackEventId,
                                                          const FName InDeathEventId)
{
	Source = InSource;
	AttackEventId = InAttackEventId;
	DeathEventId = InDeathEventId;
}

void UReEchoCombatAudioAdapterComponent::BeginPlay()
{
	Super::BeginPlay();
	UReEchoCombatEventsComponent* Events =
	    GetOwner() ? GetOwner()->FindComponentByClass<UReEchoCombatEventsComponent>() : nullptr;
	if (!Events)
	{
		return;
	}
	BoundEvents = Events;
	Events->OnAttackCommitted.AddDynamic(this, &UReEchoCombatAudioAdapterComponent::HandleAttackCommitted);
	Events->OnHit.AddDynamic(this, &UReEchoCombatAudioAdapterComponent::HandleHit);
	Events->OnHurt.AddDynamic(this, &UReEchoCombatAudioAdapterComponent::HandleHurt);
	Events->OnKill.AddDynamic(this, &UReEchoCombatAudioAdapterComponent::HandleKill);
	Events->OnDeath.AddDynamic(this, &UReEchoCombatAudioAdapterComponent::HandleDeath);
	Events->OnElementReactionResolved.AddDynamic(
	    this, &UReEchoCombatAudioAdapterComponent::HandleElementReactionResolved);
}

void UReEchoCombatAudioAdapterComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UReEchoCombatEventsComponent* Events = BoundEvents.Get())
	{
		Events->OnAttackCommitted.RemoveAll(this);
		Events->OnHit.RemoveAll(this);
		Events->OnHurt.RemoveAll(this);
		Events->OnKill.RemoveAll(this);
		Events->OnDeath.RemoveAll(this);
		Events->OnElementReactionResolved.RemoveAll(this);
	}
	BoundEvents.Reset();
	Super::EndPlay(EndPlayReason);
}

void UReEchoCombatAudioAdapterComponent::HandleAttackCommitted(const FReEchoAttackCommittedEvent& Event)
{
	PostConfiguredAttack(Event.Origin, Event.WeaponId);
}

void UReEchoCombatAudioAdapterComponent::PostConfiguredAttack(const FVector& WorldLocation, const FName VariantId) const
{
	PostConfiguredEvent(AttackEventId, WorldLocation, VariantId);
}

void UReEchoCombatAudioAdapterComponent::PostConfiguredEvent(const FName EventId,
                                                             const FVector& WorldLocation,
                                                             const FName VariantId) const
{
	if (EventId.IsNone())
	{
		return;
	}
	if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
	{
		FReEchoAudioEventRequest Request;
		Request.EventId = EventId;
		Request.WorldLocation = WorldLocation;
		Request.SourceCategory = ToAudioSourceCategory(Source);
		Request.VariantId = VariantId;
		if (UReEchoAudioService* AudioService = GameInstance->GetSubsystem<UReEchoAudioService>())
		{
			AudioService->PostEvent(GetOwner(), Request);
		}
	}
}

void UReEchoCombatAudioAdapterComponent::PostDamageEvent(const FName EventId, const FReEchoDamageEvent& Event) const
{
	if (EventId.IsNone())
	{
		return;
	}
	if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
	{
		FReEchoAudioEventRequest Request;
		Request.EventId = EventId;
		Request.WorldLocation = Event.WorldLocation;
		const AActor* SourceActor = Event.Attack.Source.Get();
		const UReEchoCombatAudioAdapterComponent* SourceAdapter =
		    SourceActor ? SourceActor->FindComponentByClass<UReEchoCombatAudioAdapterComponent>() : nullptr;
		Request.SourceCategory =
		    SourceAdapter ? ToAudioSourceCategory(SourceAdapter->Source) : EReEchoAudioSourceCategory::Player;
		if (EventId == FReEchoAudioEvents::CombatHit)
		{
			Request.VariantId = ReEchoElementReaction::GetElementId(Event.Element);
		}
		Request.Intensity = FMath::Max(0.0f, Event.AppliedDamage);
		if (UReEchoAudioService* AudioService = GameInstance->GetSubsystem<UReEchoAudioService>())
		{
			AudioService->PostEvent(GetOwner(), Request);
		}
	}
}

void UReEchoCombatAudioAdapterComponent::HandleHit(const FReEchoDamageEvent& Event)
{
	PostDamageEvent(FReEchoAudioEvents::CombatHit, Event);
}

void UReEchoCombatAudioAdapterComponent::HandleHurt(const FReEchoDamageEvent& Event)
{
	const IReEchoCombatTarget* Target = Event.Target ? Cast<IReEchoCombatTarget>(Event.Target) : nullptr;
	if (!Event.bBlocked && Target && !Target->IsCombatTargetAlive())
	{
		return;
	}
	PostDamageEvent(Event.bBlocked ? FReEchoAudioEvents::CombatBlock : FReEchoAudioEvents::CombatHurt, Event);
}

void UReEchoCombatAudioAdapterComponent::HandleKill(const FReEchoDamageEvent& Event)
{
	PostDamageEvent(FReEchoAudioEvents::CombatKill, Event);
}

void UReEchoCombatAudioAdapterComponent::HandleDeath(const FReEchoDamageEvent& Event)
{
	PostDamageEvent(DeathEventId, Event);
}

void UReEchoCombatAudioAdapterComponent::HandleElementReactionResolved(
    const FReEchoElementReactionResolvedEvent& Event)
{
	if (Event.ReactionBehaviorId.IsNone() || !Event.PrimaryTarget)
	{
		return;
	}
	PostConfiguredEvent(
	    FReEchoAudioEvents::CombatReaction, Event.PrimaryTarget->GetActorLocation(), Event.ReactionBehaviorId);
}
