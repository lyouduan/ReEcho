#pragma once

#include "Combat/ReEchoCombatContracts.h"
#include "Components/ActorComponent.h"
#include "ReEchoCombatAudioAdapterComponent.generated.h"

/** Main-module adapter: translates Combat semantic events into ReEchoAudio requests. */
UCLASS(ClassGroup = (ReEcho))

class REECHO_API UReEchoCombatAudioAdapterComponent : public UActorComponent
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION() void HandleAttackCommitted(const FReEchoAttackCommittedEvent& Event);
	UFUNCTION() void HandleHit(const FReEchoDamageEvent& Event);
	UFUNCTION() void HandleHurt(const FReEchoDamageEvent& Event);
	UFUNCTION() void HandleKill(const FReEchoDamageEvent& Event);
	UFUNCTION() void HandleDeath(const FReEchoDamageEvent& Event);
	void PostEvent(FName EventId, const FReEchoDamageEvent& Event) const;

	TWeakObjectPtr<UReEchoCombatEventsComponent> BoundEvents;
};
