#pragma once

#include "Combat/ReEchoCombatContracts.h"
#include "Components/ActorComponent.h"
#include "Enemies/ReEchoEnemyEventsComponent.h"
#include "Presentation/Combat/ReEchoCombatPresentationTypes.h"
#include "TimerManager.h"
#include "ReEchoCombatVfxComponent.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;
class UMaterialBillboardComponent;
class UMaterialInterface;
class UTexture2D;
class APlayerController;
class USceneComponent;
class UReEchoCombatPresentationCoordinator;

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
	/** Echo card auras are a dedicated background layer immediately below their owning character. */
	static int32 ResolveEchoAuraSortPriority(int32 OwnerSortPriority);
	/** Pure identity projection shared by runtime and automation. */
	static FReEchoProjectileVisualKey ResolveProjectileVisualKey(const FReEchoEnemyProjectileEvent& Event);
	/** Material sprites fill their quad, so the core diameter matches the authoritative collider exactly. */
	static float ResolveProjectileCoreDiameter(float CollisionRadiusCm);
	/** Additive glow extends beyond the core without changing gameplay collision. */
	static float ResolveProjectileGlowDiameter(float CollisionRadiusCm);
	/** Burn and Growth visuals follow authoritative status/attachment events instead of duplicating one-shots. */
	static bool IsElementReactionStateDriven(FName ReactionId);
	/** Resolves and directly previews one reaction VFX without mutating combat element state. */
	static bool TryResolveDebugElementReactionSemantic(FName ReactionName, uint8& OutSemanticValue);
	bool PlayElementReactionForDebug(uint8 SemanticValue, AActor* Target) const;
	static float ResolveConductLinkScheduledTime(int32 LinkIndex, float DelaySeconds);
	/** Conduct Niagara uses world-space endpoints on an identity component at the world origin. */
	static void ResolveConductLinkWorldEndpoints(const FVector& StartWorld,
	                                             const FVector& EndWorld,
	                                             FVector& OutStartParameter,
	                                             FVector& OutEndParameter);
	static float ResolveConductPropagationDelaySeconds(FName WeaponId);
	/** Converts the locked Boss beam contract into immutable world-space endpoints. */
	static void ResolveBossBeamWorldEndpoints(
	    const FVector& Origin, const FVector& LockedDirection, float LengthCm, FVector& OutStart, FVector& OutEnd);
	/** Projects the locked warning center onto the Arena gameplay plane without changing its authoritative XY. */
	static FVector ResolveBossBeamGroundOrigin(const FVector& LockedWarningCenter, float GameplayPlaneWorldZ);
	/** Converts desired semantic scale into an attached relative scale without inheriting owner size twice. */
	static FVector
	ResolveAttachedScale(const FVector& DesiredScale, const FVector& AttachmentWorldScale, bool bPreserveWorldSize);
	/** Applies the DA correction in effect-local space after aligning the authored effect to the attack direction. */
	static FRotator ComposeAttachedRotation(const FRotator& DirectionRotation, const FRotator& LocalRotation);
	/** Generic camera-plane convention: local X follows direction and local Z faces camera. */
	static FRotator ResolveCameraPlaneDirectionRotation(const FVector& Direction, const FVector& CameraFacingNormal);
	/** Delivered 0811_01 sword mesh: local X is its surface normal and local Y follows the projected attack direction.
	 */
	static FRotator ResolveSwordMeshDirectionRotation(const FVector& Direction, const FVector& CameraFacingNormal);
	/** Keeps the composed sword direction/DA correction but flips a culled local-X back face around its local-Y attack
	 * axis. */
	static FRotator EnsureSwordFrontFacesCamera(const FRotator& ComposedRotation, const FVector& CameraFacingNormal);
	/** Left side is forward (+1), right side is reverse (-1), in current camera screen space. */
	static float ResolveMeleePlayDirection(const FVector& AttackDirection, const FVector& CameraRight);
	/** Setting an absent Niagara user parameter is a silent no-op, so replacement assets are checked explicitly. */
	static bool HasMeleePlayDirectionParameter(const UNiagaraSystem* System);
#if WITH_DEV_AUTOMATION_TESTS
	/** Exposes live Fox windup effects solely for runtime placement, lifecycle and renderer automation. */
	UNiagaraComponent* GetDirectionEffectForTests() const
	{
		return DirectionEffect;
	}

	UNiagaraComponent* GetChargingEffectForTests() const
	{
		return ChargingEffect;
	}
#endif
	/** Host-owned, Blueprint-editable scene anchors for outgoing and incoming combat effects. */
	void ConfigureAttachmentRoots(USceneComponent* InAttackVfxRoot,
	                              USceneComponent* InHurtVfxRoot,
	                              USceneComponent* InBossWeaponVfxRoot = nullptr);
	/** Player/Echo held-weapon release point, separate from the host-authored generic attack root. */
	void ConfigureWeaponAttackVfxRoot(USceneComponent* InWeaponAttackVfxRoot);
	void ConfigureEchoAuraRoot(USceneComponent* InEchoAuraVfxRoot);
	void PlayEchoCardAuraPulse(bool bPlayWater, bool bPlayGrass);
	/** Resolves the impact semantic recorded for one Boss attack without inferring from damage values. */
	bool TryResolveBossImpactSemantic(int64 AttackSequence, uint8& OutSemanticValue) const;
	/** Editor repair seam for attached Niagara systems that must follow their owning presentation root. */
	UFUNCTION(BlueprintCallable, Category = "ReEcho|VFX", meta = (DevelopmentOnly))
	static bool SetNiagaraSystemEmittersLocalSpace(UNiagaraSystem* System);
	/** Editor authoring seam for ground telegraphs whose sprite planes must use the owner's world-up axis. */
	UFUNCTION(BlueprintCallable, Category = "ReEcho|VFX", meta = (DevelopmentOnly))
	static bool SetNiagaraSystemSpriteFacingOwnerUp(UNiagaraSystem* System);
	/** Editor authoring seam for planar beam meshes that must remain camera-readable from every attack direction. */
	UFUNCTION(BlueprintCallable, Category = "ReEcho|VFX", meta = (DevelopmentOnly))
	static bool SetNiagaraSystemMeshFacingCameraPlane(UNiagaraSystem* System);
	/** Editor repair seam for melee mesh systems whose camera-facing renderer overrides component rotation. */
	UFUNCTION(BlueprintCallable, Category = "ReEcho|VFX", meta = (DevelopmentOnly))
	static bool ConfigureMeleeNiagaraComponentFacing(UNiagaraSystem* System);
	/** Editor repair seam for FaceCamera arrows whose image rotation must come from one explicit user parameter. */
	UFUNCTION(BlueprintCallable, Category = "ReEcho|VFX", meta = (DevelopmentOnly))
	static bool BindNiagaraSpriteRotationToDirectionParameter(UNiagaraSystem* System);
	/** Editor audit seam for the Fox Direction sprite pivots and optional pivot bindings. */
	UFUNCTION(BlueprintCallable, Category = "ReEcho|VFX", meta = (DevelopmentOnly))
	static bool AuditFoxDirectionSpritePivots(UNiagaraSystem* System);
	/** Editor authoring seam for per-layer Fox Direction pivots derived from source-texture alpha bounds. */
	UFUNCTION(BlueprintCallable, Category = "ReEcho|VFX", meta = (DevelopmentOnly))
	static bool SetFoxDirectionSpritePivots(UNiagaraSystem* System,
	                                        FVector2D KuangPivotInUvSpace,
	                                        FVector2D Kuang002PivotInUvSpace);
#if WITH_DEV_AUTOMATION_TESTS
	int32 GetProjectileVisualCountForTests() const;
	int32 GetBossProjectileEffectCountForTests() const;
	bool
	TryGetProjectileVisualLocationForTests(int64 AttackSequence, int32 VolleyBallIndex, FVector& OutLocation) const;
	void ScheduleConductLinksForTests(const FReEchoElementReactionResolvedEvent& Event, float DelaySeconds);

	void CancelConductPropagationForTests()
	{
		CancelConductPropagation();
	}

	int32 GetPendingConductTimerCountForTests() const
	{
		return ConductPropagationTimers.Num();
	}
#endif

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void
	TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void BindEventSources(UReEchoCombatEventsComponent* InCombatEvents, UReEchoEnemyEventsComponent* InEnemyEvents);
	UNiagaraSystem* ResolveSystem(uint8 SemanticValue) const;
	UTexture2D* ResolveRabbitProjectileTexture() const;
	UMaterialInterface* ResolveRabbitProjectileMaterial() const;
	TArray<UMaterialInterface*> ResolveRabbitProjectileGlowMaterials() const;
	UNiagaraComponent*
	SpawnWorld(uint8 SemanticValue, const FVector& Location, const FVector& Direction, bool bAutoDestroy = true) const;
	UNiagaraComponent* SpawnBossBeam(const FReEchoBossIntent& Intent, const FVector& GroundOrigin) const;
	UNiagaraComponent* SpawnAttached(uint8 SemanticValue,
	                                 const FVector& Direction,
	                                 USceneComponent* AttachmentRoot,
	                                 bool bAutoDestroy = true) const;
	USceneComponent* ResolveBossWeaponVfxRoot() const;
	USceneComponent* ResolveWeaponAttackVfxRoot() const;
	USceneComponent* ResolveAttackVfxRoot() const;
	USceneComponent* ResolveHurtVfxRoot() const;
	USceneComponent* ResolveEchoAuraVfxRoot() const;
	FVector ResolveBossTargetGroundLocation(const FReEchoBossIntent& Intent) const;
	/** Every character combat effect uses the global foreground band and remains above its owning presentation. */
	int32 ResolveOwnerSortPriority() const;
	int32 ResolveOwnerAuraSortPriority() const;
	void StopEffect(TObjectPtr<UNiagaraComponent>& Effect);
	void StopProjectileVisual(UMaterialBillboardComponent* Visual) const;
	void StopNiagaraEffect(UNiagaraComponent* Effect) const;
	void StopAllEffects();
	void StopBossActionEffects();
	void RememberBossAbility(int64 AttackSequence, FName AbilityId);
	FName ResolveElementVfxTargetId(AActor* Target) const;
	UNiagaraSystem* ResolveElementSystem(uint8 SemanticValue, AActor* Target) const;
	void RefreshElementEffects(const FReEchoElementState& State);
	void RefreshElementAttachment(EReEchoElement Element);
	void RefreshBurnStatus(bool bBurnActive);
	UNiagaraComponent* SpawnElementReactionAt(uint8 SemanticValue, AActor* Target) const;
	void SpawnConductLink(const FReEchoElementReactionLink& Link) const;
	void CancelConductPropagation();
	void ScheduleConductLinks(const FReEchoElementReactionResolvedEvent& Event);
	void ScheduleConductLinksWithDelay(const FReEchoElementReactionResolvedEvent& Event, float DelaySeconds);

	UFUNCTION()
	void HandleAttackCommitted(const FReEchoAttackCommittedEvent& Event);
	UFUNCTION()
	void HandleHit(const FReEchoDamageEvent& Event);
	UFUNCTION()
	void HandleHurt(const FReEchoDamageEvent& Event);
	UFUNCTION()
	void HandleDeath(const FReEchoDamageEvent& Event);
	UFUNCTION()
	void HandleElementStateChanged(const FReEchoElementStateChangedEvent& Event);
	UFUNCTION()
	void HandleElementReactionResolved(const FReEchoElementReactionResolvedEvent& Event);
	UFUNCTION()
	void HandlePresentationAction(const FReEchoPresentationActionEvent& Event);
	UFUNCTION()
	void HandleProjectile(const FReEchoEnemyProjectileEvent& Event);
	UFUNCTION()
	void HandleBossIntent(const FReEchoBossIntent& Intent);

	UPROPERTY()
	TObjectPtr<UReEchoCombatEventsComponent> CombatEvents;

	UPROPERTY()
	TObjectPtr<UReEchoEnemyEventsComponent> EnemyEvents;
	UPROPERTY()
	TObjectPtr<UReEchoCombatPresentationCoordinator> CombatPresentationCoordinator;

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
	TMap<FReEchoProjectileVisualKey, TObjectPtr<UNiagaraComponent>> BossProjectileEffects;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> BossChargingEffect;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> BossTelegraphEffect;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> BossActiveEffect;
	float BossActiveEffectRemainingSeconds = 0.0f;

	TMap<int64, FName> BossAbilityByAttackSequence;
	TMap<int64, FVector> BossGroundLocationByAttackSequence;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> AttackVfxRoot;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> HurtVfxRoot;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> BossWeaponVfxRoot;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> WeaponAttackVfxRoot;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> EchoAuraVfxRoot;

	mutable TSet<uint8> MissingSystemWarnings;
	mutable TSet<FString> MissingElementSystemWarnings;
	mutable bool bMissingRabbitProjectileTextureWarned = false;
	mutable bool bMissingRabbitProjectileMaterialWarned = false;
	mutable bool bMissingRabbitProjectileGlowMaterialWarned = false;
	mutable bool bMissingMeleePlayDirectionWarned = false;
	TArray<FTimerHandle> ConductPropagationTimers;

	struct FReverseMeleePlayback
	{
		TWeakObjectPtr<UNiagaraComponent> Effect;
		float RemainingSeconds = 0.0f;
	};

	mutable TArray<FReverseMeleePlayback> ReverseMeleePlaybacks;

	uint64 ConductBatchSerial = 0;
};
