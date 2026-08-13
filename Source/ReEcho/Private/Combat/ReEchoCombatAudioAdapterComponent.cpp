#include "Combat/ReEchoCombatAudioAdapterComponent.h"

#include "ReEchoAudioEvents.h"
#include "ReEchoAudioService.h"
#include "ReEchoAudioTypes.h"

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
	}
	BoundEvents.Reset();
	Super::EndPlay(EndPlayReason);
}

void UReEchoCombatAudioAdapterComponent::HandleAttackCommitted(const FReEchoAttackCommittedEvent& Event)
{
	if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
	{
		FReEchoAudioEventRequest Request;
		Request.EventId = FReEchoAudioEvents::CombatAttack;
		Request.WorldLocation = Event.Origin;
		Request.SourceCategory = EReEchoAudioSourceCategory::Player;
		Request.VariantId = Event.WeaponId;
		GameInstance->GetSubsystem<UReEchoAudioService>()->PostEvent(GetOwner(), Request);
	}
}

void UReEchoCombatAudioAdapterComponent::PostEvent(const FName EventId, const FReEchoDamageEvent& Event) const
{
	if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
	{
		FReEchoAudioEventRequest Request;
		Request.EventId = EventId;
		Request.WorldLocation = Event.WorldLocation;
		Request.SourceCategory =
		    Event.Source == GetOwner() ? EReEchoAudioSourceCategory::Player : EReEchoAudioSourceCategory::Enemy;
		Request.Intensity = FMath::Max(0.0f, Event.AppliedDamage);
		GameInstance->GetSubsystem<UReEchoAudioService>()->PostEvent(GetOwner(), Request);
	}
}

void UReEchoCombatAudioAdapterComponent::HandleHit(const FReEchoDamageEvent& Event)
{
	PostEvent(FReEchoAudioEvents::CombatHit, Event);
}

void UReEchoCombatAudioAdapterComponent::HandleHurt(const FReEchoDamageEvent& Event)
{
	PostEvent(Event.bBlocked ? FReEchoAudioEvents::CombatBlock : FReEchoAudioEvents::CombatHurt, Event);
}

void UReEchoCombatAudioAdapterComponent::HandleKill(const FReEchoDamageEvent& Event)
{
	PostEvent(FReEchoAudioEvents::CombatKill, Event);
}

void UReEchoCombatAudioAdapterComponent::HandleDeath(const FReEchoDamageEvent& Event)
{
	PostEvent(FReEchoAudioEvents::CombatDeath, Event);
}
