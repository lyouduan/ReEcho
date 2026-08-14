#include "Combat/ReEchoAttackControllerComponent.h"

#include "Combat/ReEchoCombatTarget.h"

UReEchoAttackControllerComponent::UReEchoAttackControllerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

IReEchoAttackControllerHost* UReEchoAttackControllerComponent::ResolveHost() const
{
	return Cast<IReEchoAttackControllerHost>(GetOwner());
}

void UReEchoAttackControllerComponent::SetAttackMode(const EReEchoAttackMode NewMode)
{
	if (Mode == NewMode)
	{
		return;
	}
	ReleaseAttackRequests();
	Mode = NewMode;
}

void UReEchoAttackControllerComponent::BeginManualAttack()
{
	IReEchoAttackControllerHost* Host = ResolveHost();
	if (Mode != EReEchoAttackMode::Manual || bManualHeld || !Host || !Host->CanIssueAttackRequest())
	{
		return;
	}
	bManualHeld = true;
	Host->PressBasicAttackInput();
}

void UReEchoAttackControllerComponent::EndManualAttack()
{
	if (Mode != EReEchoAttackMode::Manual || !bManualHeld)
	{
		return;
	}
	bManualHeld = false;
	if (IReEchoAttackControllerHost* Host = ResolveHost())
	{
		Host->ReleaseBasicAttackInput();
	}
}

void UReEchoAttackControllerComponent::ReleaseAutomaticAttack()
{
	CurrentTarget.Reset();
	if (!bAutomaticHeld)
	{
		return;
	}
	bAutomaticHeld = false;
	if (IReEchoAttackControllerHost* Host = ResolveHost())
	{
		Host->ReleaseBasicAttackInput();
	}
}

void UReEchoAttackControllerComponent::ReleaseAttackRequests()
{
	IReEchoAttackControllerHost* Host = ResolveHost();
	const bool bWasHeld = bManualHeld || bAutomaticHeld;
	bManualHeld = false;
	bAutomaticHeld = false;
	CurrentTarget.Reset();
	if (bWasHeld && Host)
	{
		Host->ReleaseBasicAttackInput();
	}
}

void UReEchoAttackControllerComponent::UpdateAutomaticAttack()
{
	IReEchoAttackControllerHost* Host = ResolveHost();
	UReEchoTargetingComponent* Targeting =
	    GetOwner() ? GetOwner()->FindComponentByClass<UReEchoTargetingComponent>() : nullptr;
	if (Mode != EReEchoAttackMode::Automatic || !Host || !Targeting || !Host->CanIssueAttackRequest())
	{
		ReleaseAutomaticAttack();
		return;
	}
	AActor* Target = Targeting->FindNearestTarget(Host->GetAutomaticAttackRange());
	if (!Target)
	{
		ReleaseAutomaticAttack();
		return;
	}
	CurrentTarget = Target;
	Host->FaceAutomaticTarget(*Target);
	if (!bAutomaticHeld)
	{
		bAutomaticHeld = true;
		Host->PressBasicAttackInput();
	}
}

FReEchoAttackSnapshot UReEchoAttackControllerComponent::GetSnapshot() const
{
	FReEchoAttackSnapshot Snapshot;
	Snapshot.Mode = Mode;
	Snapshot.bAttackHeld = bManualHeld || bAutomaticHeld;
	Snapshot.CurrentTarget = CurrentTarget.Get();
	if (const IReEchoAttackControllerHost* Host = ResolveHost())
	{
		Snapshot.WeaponId = Host->GetAttackWeaponId();
		Snapshot.AttackStepId = Host->GetAttackStepId();
		Snapshot.NextAttackRemaining = Host->GetAttackReadinessRemaining();
		Snapshot.AttackSpeed = Host->GetEffectiveAttackSpeed();
	}
	return Snapshot;
}
