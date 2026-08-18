#pragma once

#include "Combat/ReEchoCombatContracts.h"
#include "Components/ActorComponent.h"
#include "Enemies/ReEchoEnemyEventsComponent.h"
#include "ReEchoCombatVfxComponent.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;

/** Read-only presentation adapter for Combat and Enemy semantic events. */
UCLASS(ClassGroup = (ReEcho), meta = (BlueprintSpawnableComponent))

class REECHO_API UReEchoCombatVfxComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UReEchoCombatVfxComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void BindEventSources(UReEchoCombatEventsComponent* InCombatEvents, UReEchoEnemyEventsComponent* InEnemyEvents);
	UNiagaraSystem* ResolveSystem(uint8 SemanticValue) const;
	UNiagaraComponent* SpawnWorld(uint8 SemanticValue,
	                              const FVector& Location,
	                              const FVector& Direction,
	                              bool bLocalYAxisForward = false,
	                              bool bAutoDestroy = true) const;
	UNiagaraComponent* SpawnAttached(uint8 SemanticValue, const FVector& Direction) const;
	void StopEffect(TObjectPtr<UNiagaraComponent>& Effect);
	void StopAllEffects();

	UFUNCTION()
	void HandleAttackCommitted(const FReEchoAttackCommittedEvent& Event);
	UFUNCTION()
	void HandleHurt(const FReEchoDamageEvent& Event);
	UFUNCTION()
	void HandleDeath(const FReEchoDamageEvent& Event);
	UFUNCTION()
	void HandleSpecialAction(const FReEchoEnemySpecialActionEvent& Event);
	UFUNCTION()
	void HandleProjectile(const FReEchoEnemyProjectileEvent& Event);

	UPROPERTY()
	TObjectPtr<UReEchoCombatEventsComponent> CombatEvents;

	UPROPERTY()
	TObjectPtr<UReEchoEnemyEventsComponent> EnemyEvents;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> ChargingEffect;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> DirectionEffect;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> DashEffect;

	UPROPERTY(Transient)
	TMap<int64, TObjectPtr<UNiagaraComponent>> ProjectileEffects;

	mutable TSet<uint8> MissingSystemWarnings;
};
