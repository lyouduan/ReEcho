#include "AbilitySystem/ReEchoPlayerAbilities.h"

#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "AbilitySystem/ReEchoGameplayEffects.h"
#include "AbilitySystem/ReEchoGameplayTags.h"
#include "Combat/ReEchoAttackHost.h"
#include "Engine/World.h"
#include "TimerManager.h"

namespace
{
FGameplayTagContainer MakeTags(std::initializer_list<FGameplayTag> Tags)
{
	FGameplayTagContainer Result;
	for (const FGameplayTag Tag : Tags)
	{
		Result.AddTag(Tag);
	}
	return Result;
}
}

UReEchoPlayerGameplayAbility::UReEchoPlayerGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	ActivationBlockedTags =
	    MakeTags({ReEchoGameplayTags::State_Dead, ReEchoGameplayTags::State_Stunned, ReEchoGameplayTags::State_Menu});
}

IReEchoAttackHost* UReEchoPlayerGameplayAbility::ResolveHost() const
{
	return Cast<IReEchoAttackHost>(GetAvatarActorFromActorInfo());
}

void UReEchoPlayerGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                                   const FGameplayAbilityActorInfo* ActorInfo,
                                                   const FGameplayAbilityActivationInfo ActivationInfo,
                                                   const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	IReEchoAttackHost* Host = ResolveHost();
	if (!Host || !CommitAbility(Handle, ActorInfo, ActivationInfo) || !ExecuteHostAbility(*Host))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

const FGameplayTagContainer* UReEchoPlayerGameplayAbility::GetCooldownTags() const
{
	return CooldownTags.IsEmpty() ? Super::GetCooldownTags() : &CooldownTags;
}

UGameplayEffect* UReEchoPlayerGameplayAbility::GetCooldownGameplayEffect() const
{
	return CooldownEffectClass ? CooldownEffectClass->GetDefaultObject<UGameplayEffect>()
	                           : Super::GetCooldownGameplayEffect();
}

void UReEchoPlayerGameplayAbility::ApplyCooldown(const FGameplayAbilitySpecHandle Handle,
                                                 const FGameplayAbilityActorInfo* ActorInfo,
                                                 const FGameplayAbilityActivationInfo ActivationInfo) const
{
	if (!CooldownEffectClass)
	{
		Super::ApplyCooldown(Handle, ActorInfo, ActivationInfo);
		return;
	}
	const IReEchoAttackHost* Host = ResolveHost();
	FGameplayEffectSpecHandle Spec =
	    MakeOutgoingGameplayEffectSpec(CooldownEffectClass, GetAbilityLevel(Handle, ActorInfo));
	if (Host && Spec.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(ReEchoGameplayTags::Data_Cooldown, GetCooldownDuration(*Host));
		ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, Spec);
	}
}

float UReEchoPlayerGameplayAbility::GetCooldownDuration(const IReEchoAttackHost& Host) const
{
	return 0.0f;
}

UReEchoBasicAttackAbility::UReEchoBasicAttackAbility()
{
	SetAssetTags(MakeTags({ReEchoGameplayTags::Ability_Attack_Basic}));
}

void UReEchoBasicAttackAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                                const FGameplayAbilityActorInfo* ActorInfo,
                                                const FGameplayAbilityActivationInfo ActivationInfo,
                                                const FGameplayEventData* TriggerEventData)
{
	UGameplayAbility::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	if (!ResolveHost())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	UAbilityTask_WaitInputRelease* ReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this, true);
	ReleaseTask->OnRelease.AddDynamic(this, &UReEchoBasicAttackAbility::HandleInputReleased);
	ReleaseTask->ReadyForActivation();
	AttemptOrWait();
}

void UReEchoBasicAttackAbility::AttemptOrWait()
{
	IReEchoAttackHost* Host = ResolveHost();
	if (!Host)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}
	const float WaitRemaining = Host->GetBasicAttackWaitRemaining();
	if (WaitRemaining > KINDA_SMALL_NUMBER)
	{
		ScheduleNextAttack(WaitRemaining);
		return;
	}
	switch (Host->TryCommitBasicAttack())
	{
		case EReEchoAttackAttempt::Committed:
		case EReEchoAttackAttempt::Waiting:
			ScheduleNextAttack(FMath::Max(0.01f, Host->GetBasicAttackWaitRemaining()));
			return;
		default:
			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
			return;
	}
}

void UReEchoBasicAttackAbility::ScheduleNextAttack(const float Delay)
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (UWorld* World = Avatar ? Avatar->GetWorld() : nullptr)
	{
		World->GetTimerManager().SetTimer(
		    RepeatTimerHandle, this, &UReEchoBasicAttackAbility::HandleRepeatDelay, FMath::Max(0.01f, Delay), false);
	}
	else
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UReEchoBasicAttackAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
                                           const FGameplayAbilityActorInfo* ActorInfo,
                                           const FGameplayAbilityActivationInfo ActivationInfo,
                                           const bool bReplicateEndAbility,
                                           const bool bWasCancelled)
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (UWorld* World = Avatar ? Avatar->GetWorld() : nullptr)
	{
		World->GetTimerManager().ClearTimer(RepeatTimerHandle);
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UReEchoBasicAttackAbility::HandleRepeatDelay()
{
	const FGameplayAbilitySpec* Spec = GetCurrentAbilitySpec();
	if (!Spec || !Spec->InputPressed)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}
	AttemptOrWait();
}

void UReEchoBasicAttackAbility::HandleInputReleased(const float TimeHeld)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

bool UReEchoBasicAttackAbility::ExecuteHostAbility(IReEchoAttackHost& Host) const
{
	return Host.TryCommitBasicAttack() == EReEchoAttackAttempt::Committed;
}

UReEchoActiveAttackAbility::UReEchoActiveAttackAbility()
{
	SetAssetTags(MakeTags({ReEchoGameplayTags::Ability_Attack_Active}));
	CooldownEffectClass = UReEchoActiveAttackCooldownEffect::StaticClass();
	CooldownTags.AddTag(ReEchoGameplayTags::Cooldown_Attack_Active);
}

bool UReEchoActiveAttackAbility::ExecuteHostAbility(IReEchoAttackHost& Host) const
{
	return Host.ExecuteActiveAttack();
}

float UReEchoActiveAttackAbility::GetCooldownDuration(const IReEchoAttackHost& Host) const
{
	return Host.GetActiveAttackCooldown();
}
