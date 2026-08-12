#include "AbilitySystem/ReEchoPlayerAbilities.h"

#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "AbilitySystem/ReEchoGameplayEffects.h"
#include "AbilitySystem/ReEchoGameplayTags.h"
#include "Player/ReEchoPlayerPawn.h"

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

void UReEchoPlayerGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                                   const FGameplayAbilityActorInfo* ActorInfo,
                                                   const FGameplayAbilityActivationInfo ActivationInfo,
                                                   const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	AReEchoPlayerPawn* PlayerPawn = Cast<AReEchoPlayerPawn>(GetAvatarActorFromActorInfo());
	if (!PlayerPawn || !CommitAbility(Handle, ActorInfo, ActivationInfo) || !ExecutePlayerAbility(*PlayerPawn))
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
	const AReEchoPlayerPawn* PlayerPawn = Cast<AReEchoPlayerPawn>(GetAvatarActorFromActorInfo());
	FGameplayEffectSpecHandle Spec =
	    MakeOutgoingGameplayEffectSpec(CooldownEffectClass, GetAbilityLevel(Handle, ActorInfo));
	if (PlayerPawn && Spec.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(ReEchoGameplayTags::Data_Cooldown, GetCooldownDuration(*PlayerPawn));
		ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, Spec);
	}
}

float UReEchoPlayerGameplayAbility::GetCooldownDuration(const AReEchoPlayerPawn& PlayerPawn) const
{
	return 0.0f;
}

UReEchoBasicAttackAbility::UReEchoBasicAttackAbility()
{
	SetAssetTags(MakeTags({ReEchoGameplayTags::Ability_Attack_Basic}));
	CooldownEffectClass = UReEchoBasicAttackCooldownEffect::StaticClass();
	CooldownTags.AddTag(ReEchoGameplayTags::Cooldown_Attack_Basic);
}

void UReEchoBasicAttackAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                                const FGameplayAbilityActorInfo* ActorInfo,
                                                const FGameplayAbilityActivationInfo ActivationInfo,
                                                const FGameplayEventData* TriggerEventData)
{
	UGameplayAbility::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	if (!CommitAndExecuteCurrentAttack())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	UAbilityTask_WaitInputRelease* ReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this, true);
	ReleaseTask->OnRelease.AddDynamic(this, &UReEchoBasicAttackAbility::HandleInputReleased);
	ReleaseTask->ReadyForActivation();
	const AReEchoPlayerPawn* PlayerPawn = Cast<AReEchoPlayerPawn>(GetAvatarActorFromActorInfo());
	ScheduleNextAttack(PlayerPawn ? GetCooldownDuration(*PlayerPawn) : 0.1f);
}

bool UReEchoBasicAttackAbility::CommitAndExecuteCurrentAttack()
{
	AReEchoPlayerPawn* PlayerPawn = Cast<AReEchoPlayerPawn>(GetAvatarActorFromActorInfo());
	return PlayerPawn && CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo) &&
	       ExecutePlayerAbility(*PlayerPawn);
}

void UReEchoBasicAttackAbility::ScheduleNextAttack(const float Delay)
{
	UAbilityTask_WaitDelay* DelayTask = UAbilityTask_WaitDelay::WaitDelay(this, FMath::Max(0.01f, Delay));
	DelayTask->OnFinish.AddDynamic(this, &UReEchoBasicAttackAbility::HandleRepeatDelay);
	DelayTask->ReadyForActivation();
}

void UReEchoBasicAttackAbility::HandleRepeatDelay()
{
	const FGameplayAbilitySpec* Spec = GetCurrentAbilitySpec();
	if (!Spec || !Spec->InputPressed)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	AReEchoPlayerPawn* PlayerPawn = Cast<AReEchoPlayerPawn>(GetAvatarActorFromActorInfo());
	if (!PlayerPawn)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	// 临时武器动作锁（有序攻击步骤锁）不得永久终止仍被按住的 GAS 普攻循环。
	// 改为在锁解除后重试，使 held 普攻连续命中，而不是第一发被拒后立即结束。
	if (PlayerPawn->IsWeaponActionLocked())
	{
		ScheduleNextAttack(PlayerPawn->GetWeaponActionLockRemaining() + KINDA_SMALL_NUMBER);
		return;
	}

	if (!CommitAndExecuteCurrentAttack())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	ScheduleNextAttack(GetCooldownDuration(*PlayerPawn));
}

void UReEchoBasicAttackAbility::HandleInputReleased(const float TimeHeld)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

bool UReEchoBasicAttackAbility::ExecutePlayerAbility(AReEchoPlayerPawn& PlayerPawn) const
{
	return PlayerPawn.ExecuteBasicAttackAbility();
}

float UReEchoBasicAttackAbility::GetCooldownDuration(const AReEchoPlayerPawn& PlayerPawn) const
{
	return PlayerPawn.GetCurrentAttackInterval();
}

UReEchoActiveAttackAbility::UReEchoActiveAttackAbility()
{
	SetAssetTags(MakeTags({ReEchoGameplayTags::Ability_Attack_Active}));
	CooldownEffectClass = UReEchoActiveAttackCooldownEffect::StaticClass();
	CooldownTags.AddTag(ReEchoGameplayTags::Cooldown_Attack_Active);
}

bool UReEchoActiveAttackAbility::ExecutePlayerAbility(AReEchoPlayerPawn& PlayerPawn) const
{
	return PlayerPawn.ExecuteActiveAttackAbility();
}

float UReEchoActiveAttackAbility::GetCooldownDuration(const AReEchoPlayerPawn& PlayerPawn) const
{
	return PlayerPawn.GetCurrentAttackInterval();
}
