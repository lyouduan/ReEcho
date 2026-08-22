#pragma once

#include "Combat/ReEchoCombatContracts.h"
#include "Components/ActorComponent.h"
#include "Enemies/ReEchoEnemyEventsComponent.h"
#include "ReEchoCombatVfxComponent.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;
class UMaterialBillboardComponent;
class UMaterialInterface;
class UTexture2D;
class APlayerController;
class USceneComponent;

/** Per-owner stable key for one visible projectile in a committed enemy volley. */
USTRUCT()

struct FReEchoProjectileVisualKey
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<AActor> Source;

	UPROPERTY()
	int64 Sequence = 0;

	UPROPERTY()
	int32 VolleyBallIndex = INDEX_NONE;

	bool operator==(const FReEchoProjectileVisualKey& Other) const
	{
		return Source.HasSameIndexAndSerialNumber(Other.Source) && Sequence == Other.Sequence &&
		       VolleyBallIndex == Other.VolleyBallIndex;
	}
};

FORCEINLINE uint32 GetTypeHash(const FReEchoProjectileVisualKey& Key)
{
	return HashCombineFast(HashCombineFast(GetTypeHash(Key.Source), GetTypeHash(Key.Sequence)),
	                       GetTypeHash(Key.VolleyBallIndex));
}

/** Read-only presentation adapter for Combat and Enemy semantic events. */
UCLASS(ClassGroup = (ReEcho), meta = (BlueprintSpawnableComponent))

class REECHO_API UReEchoCombatVfxComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UReEchoCombatVfxComponent();
	/** Pure layer policy shared by runtime and automation. */
	static int32 ResolveCombatEffectSortPriority(int32 OwnerSortPriority);
	/** Pure identity projection shared by runtime and automation. */
	static FReEchoProjectileVisualKey ResolveProjectileVisualKey(const FReEchoEnemyProjectileEvent& Event);
	/** Material sprites fill their quad, so the core diameter matches the authoritative collider exactly. */
	static float ResolveProjectileCoreDiameter(float CollisionRadiusCm);
	/** Additive glow extends beyond the core without changing gameplay collision. */
	static float ResolveProjectileGlowDiameter(float CollisionRadiusCm);
	/** Burn and Growth visuals follow authoritative status/attachment events instead of duplicating one-shots. */
	static bool IsElementReactionStateDriven(FName ReactionId);
	/** Host-owned, Blueprint-editable scene anchors for outgoing and incoming combat effects. */
	void ConfigureAttachmentRoots(USceneComponent* InAttackVfxRoot, USceneComponent* InHurtVfxRoot);
#if WITH_DEV_AUTOMATION_TESTS
	int32 GetProjectileVisualCountForTests() const;
	bool
	TryGetProjectileVisualLocationForTests(int64 AttackSequence, int32 VolleyBallIndex, FVector& OutLocation) const;
#endif

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void BindEventSources(UReEchoCombatEventsComponent* InCombatEvents, UReEchoEnemyEventsComponent* InEnemyEvents);
	UNiagaraSystem* ResolveSystem(uint8 SemanticValue) const;
	UTexture2D* ResolveRabbitProjectileTexture() const;
	UMaterialInterface* ResolveRabbitProjectileMaterial() const;
	TArray<UMaterialInterface*> ResolveRabbitProjectileGlowMaterials() const;
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
	void StopProjectileVisual(UMaterialBillboardComponent* Visual) const;
	void StopAllEffects();
	FName ResolveElementVfxTargetId(AActor* Target) const;
	UNiagaraSystem* ResolveElementSystem(uint8 SemanticValue, AActor* Target) const;
	void RefreshElementEffects(const FReEchoElementState& State);
	void RefreshElementAttachment(EReEchoElement Element);
	void RefreshBurnStatus(bool bBurnActive);
	void SpawnElementReactionAt(uint8 SemanticValue, AActor* Target) const;
	void SpawnConductLink(const FReEchoElementReactionLink& Link) const;

	UFUNCTION()
	void HandleAttackCommitted(const FReEchoAttackCommittedEvent& Event);
	UFUNCTION()
	void HandleHurt(const FReEchoDamageEvent& Event);
	UFUNCTION()
	void HandleDeath(const FReEchoDamageEvent& Event);
	UFUNCTION()
	void HandleElementStateChanged(const FReEchoElementStateChangedEvent& Event);
	UFUNCTION()
	void HandleElementReactionResolved(const FReEchoElementReactionResolvedEvent& Event);
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
	TObjectPtr<UNiagaraComponent> ElementAttachmentEffect;

	EReEchoElement ActiveAttachmentElement = EReEchoElement::None;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> BurnStatusEffect;

	UPROPERTY(Transient)
	TMap<FReEchoProjectileVisualKey, TObjectPtr<UMaterialBillboardComponent>> ProjectileVisuals;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> AttackVfxRoot;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> HurtVfxRoot;

	mutable TSet<uint8> MissingSystemWarnings;
	mutable TSet<FString> MissingElementSystemWarnings;
	mutable bool bMissingRabbitProjectileTextureWarned = false;
	mutable bool bMissingRabbitProjectileMaterialWarned = false;
	mutable bool bMissingRabbitProjectileGlowMaterialWarned = false;
};
