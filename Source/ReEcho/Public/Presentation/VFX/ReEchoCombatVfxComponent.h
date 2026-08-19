#pragma once

#include "Combat/ReEchoCombatContracts.h"
#include "Components/ActorComponent.h"
#include "Enemies/ReEchoEnemyEventsComponent.h"
#include "ReEchoCombatVfxComponent.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;
class APlayerController;
class USceneComponent;

/** Read-only presentation adapter for Combat and Enemy semantic events. */
UCLASS(ClassGroup = (ReEcho), meta = (BlueprintSpawnableComponent))

class REECHO_API UReEchoCombatVfxComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UReEchoCombatVfxComponent();
	/** Pure layer policy shared by runtime and automation. */
	static int32 ResolveCombatEffectSortPriority(int32 OwnerSortPriority);
	/** Host-owned, Blueprint-editable scene anchors for outgoing and incoming combat effects. */
	void ConfigureAttachmentRoots(USceneComponent* InAttackVfxRoot, USceneComponent* InHurtVfxRoot);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void BindEventSources(UReEchoCombatEventsComponent* InCombatEvents, UReEchoEnemyEventsComponent* InEnemyEvents);
	UNiagaraSystem* ResolveSystem(uint8 SemanticValue) const;
	UNiagaraComponent*
	SpawnWorld(uint8 SemanticValue, const FVector& Location, const FVector& Direction, bool bAutoDestroy = true) const;
	UNiagaraComponent* SpawnAttached(uint8 SemanticValue,
	                                 const FVector& Direction,
	                                 USceneComponent* AttachmentRoot,
	                                 bool bAutoDestroy = true) const;
	USceneComponent* ResolveAttackVfxRoot() const;
	USceneComponent* ResolveHurtVfxRoot() const;
	/** Every character combat effect uses the global foreground band and remains above its owning presentation. */
	int32 ResolveOwnerSortPriority() const;
	void StopEffect(TObjectPtr<UNiagaraComponent>& Effect);
	void StopAllEffects();
	void LogRabbitProjectileTrajectory(const FReEchoEnemyProjectileEvent& Event,
	                                   const UNiagaraComponent* Effect,
	                                   const TCHAR* Phase);
	void LogRabbitParticleState(const UNiagaraComponent* Effect,
	                            APlayerController* PlayerController,
	                            const FVector2D& PlayerScreen,
	                            int64 AttackSequence,
	                            int32 EventCount) const;

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
	TMap<int64, FVector> ProjectileVisualOffsets;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> AttackVfxRoot;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> HurtVfxRoot;

	TMap<int64, int32> ProjectileTrajectoryEventCounts;

	mutable TSet<uint8> MissingSystemWarnings;
};
