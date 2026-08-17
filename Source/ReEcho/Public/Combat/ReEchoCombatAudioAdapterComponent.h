#pragma once

#include "Combat/ReEchoCombatContracts.h"
#include "Components/ActorComponent.h"
#include "ReEchoCombatAudioAdapterComponent.generated.h"

UENUM()
enum class EReEchoCombatAudioSource : uint8
{
	Player,
	Enemy,
	Boss,
	Echo
};

/** Main-module adapter: translates Combat semantic events into ReEchoAudio requests. */
UCLASS(ClassGroup = (ReEcho))

class REECHO_API UReEchoCombatAudioAdapterComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UReEchoCombatAudioAdapterComponent();
	void ConfigureRouting(EReEchoCombatAudioSource InSource, FName InAttackEventId, FName InDeathEventId);
	void PostConfiguredAttack(const FVector& WorldLocation, FName VariantId = NAME_None) const;
	void PostConfiguredEvent(FName EventId, const FVector& WorldLocation, FName VariantId = NAME_None) const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION() void HandleAttackCommitted(const FReEchoAttackCommittedEvent& Event);
	UFUNCTION() void HandleHit(const FReEchoDamageEvent& Event);
	UFUNCTION() void HandleHurt(const FReEchoDamageEvent& Event);
	UFUNCTION() void HandleKill(const FReEchoDamageEvent& Event);
	UFUNCTION() void HandleDeath(const FReEchoDamageEvent& Event);
	void PostDamageEvent(FName EventId, const FReEchoDamageEvent& Event) const;

	TWeakObjectPtr<UReEchoCombatEventsComponent> BoundEvents;
	EReEchoCombatAudioSource Source = EReEchoCombatAudioSource::Player;
	FName AttackEventId;
	FName DeathEventId;
};
