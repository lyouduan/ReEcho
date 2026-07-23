#include "AbilitySystem/ReEchoPlayerAbilities.h"

#include "Player/ReEchoPlayerPawn.h"

UReEchoPlayerGameplayAbility::UReEchoPlayerGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UReEchoPlayerGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                                   const FGameplayAbilityActorInfo* ActorInfo,
                                                   const FGameplayAbilityActivationInfo ActivationInfo,
                                                   const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AReEchoPlayerPawn* PlayerPawn = Cast<AReEchoPlayerPawn>(GetAvatarActorFromActorInfo());
	const bool bExecuted = PlayerPawn && ExecutePlayerAbility(*PlayerPawn);

	EndAbility(Handle, ActorInfo, ActivationInfo, true, !bExecuted);
}

bool UReEchoBasicAttackAbility::ExecutePlayerAbility(AReEchoPlayerPawn& PlayerPawn) const
{
	return PlayerPawn.ExecuteBasicAttackAbility();
}

bool UReEchoActiveAttackAbility::ExecutePlayerAbility(AReEchoPlayerPawn& PlayerPawn) const
{
	return PlayerPawn.ExecuteActiveAttackAbility();
}

bool UReEchoSelectWeaponSlot1Ability::ExecutePlayerAbility(AReEchoPlayerPawn& PlayerPawn) const
{
	return PlayerPawn.ExecuteSelectWeaponSlot1Ability();
}

bool UReEchoSelectWeaponSlot2Ability::ExecutePlayerAbility(AReEchoPlayerPawn& PlayerPawn) const
{
	return PlayerPawn.ExecuteSelectWeaponSlot2Ability();
}

bool UReEchoSelectWeaponSlot3Ability::ExecutePlayerAbility(AReEchoPlayerPawn& PlayerPawn) const
{
	return PlayerPawn.ExecuteSelectWeaponSlot3Ability();
}