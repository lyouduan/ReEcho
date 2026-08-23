#pragma once

#include "AbilitySystemInterface.h"
#include "Combat/ReEchoCombatContracts.h"
#include "Combat/ReEchoCombatTarget.h"
#include "Core/ReEchoTypes.h"
#include "GameFramework/Actor.h"
#include "ReEchoEnemyActor.generated.h"

class UAbilitySystemComponent;
class UBoxComponent;
class UReEcho2DAnimationComponent;
class UReEcho2DFrameCollisionDriver;
class UReEcho2DPresentationController;
class UReEcho2DPresentationCatalog;
class UReEcho2DSceneLightingComponent;
class UReEchoCombatAttributeSet;
class UReEchoCombatantComponent;
class UReEchoCombatAudioAdapterComponent;
class UReEchoCombatVfxComponent;
class UReEchoCombatEventsComponent;
class UReEchoCombatPresentationCoordinator;
class UReEchoEnemyEventsComponent;
class UReEchoEnemyLogicComponent;
class UReEchoEnemyPresentationComponent;
class UReEchoEnemyRosterComponent;
class USceneComponent;
class UStaticMeshComponent;
struct FReEchoEnemyActionIntent;
struct FReEchoEnemyAbilityDefinition;
struct FReEchoEnemyLogicSnapshot;
struct FReEchoEnemyPresentationSnapshot;
struct FReEchoEnemySenseSnapshot;
enum class EReEchoEnemyProjectileEventType : uint8;

UENUM(BlueprintType)
enum class EReEchoEnemyKind : uint8
{
	Grunt,
	Shield,
	Bomber,
	Boss,
	Slime,
	Ranged,
	Elite
};

/** Lightweight world host composing Enemy logic, Combat adjudication and read-only presentation. */
UCLASS()

class REECHO_API AReEchoEnemyActor : public AActor,
                                     public IAbilitySystemInterface,
                                     public IReEchoCombatTarget,
                                     public IReEchoCombatAffiliation
{
	GENERATED_BODY()

public:
	AReEchoEnemyActor();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	void Configure(EReEchoEnemyKind InKind, int32 SpawnIndex);
	void SetPresentationCatalog(UReEcho2DPresentationCatalog* InPresentationCatalog);
	bool ConfigureFromDefinition(const FReEchoEnemyDefinition& Definition, int32 SpawnIndex);

	void SetEnemyId(FName InEnemyId)
	{
		EnemyId = InEnemyId;
	}

	FName GetEnemyId() const
	{
		return EnemyId;
	}

	/** Stable presentation identity from the active compiled enemy definition. */
	FName GetPresentationId() const;

	void ConfigureGameplayPlane(float InGameplayPlaneWorldZ);
	void SetEnemyRoster(UReEchoEnemyRosterComponent* InRoster);
	float ReceiveGrayboxDamage(float Damage,
	                           const FVector& SourceLocation,
	                           const FLinearColor& DamageNumberColor = FLinearColor::White,
	                           FReEchoAttackIdentity Attack = {});
	float ReceiveElementalDamage(float Damage,
	                             EReEchoElement Element,
	                             const FVector& SourceLocation,
	                             float ReactionEfficiency = 1.0f,
	                             FReEchoAttackIdentity Attack = {});
	EReEchoElement GetAttachedElement() const;
	const FReEchoElementState& GetElementState() const;
#if WITH_DEV_AUTOMATION_TESTS
	FReEchoElementState& EditElementState();
	bool IsIgnoringEnemyMovementForTests(const AActor* Other) const;
#endif

	UReEchoCombatantComponent* GetCombatantComponent() const
	{
		return Combatant;
	}

	UReEchoEnemyLogicComponent* GetEnemyLogicComponent() const
	{
		return EnemyLogic;
	}

	UReEchoEnemyEventsComponent* GetEnemyEventsComponent() const
	{
		return EnemyEvents;
	}

	bool IsAlive() const;
	void ApplyCardStun(float DurationSeconds);
	void SetCardMovementMultiplier(float Multiplier);
	/** Freezes this host across a same-Stage intermission while preserving its long-lived runtime state. */
	void SetEncounterSimulationSuspended(bool bSuspended);

	bool IsEncounterSimulationSuspended() const
	{
		return bEncounterSimulationSuspended;
	}

	virtual bool IsCombatTargetAlive() const override
	{
		return IsAlive();
	}

	virtual FVector GetCombatTargetLocation() const override
	{
		return GetActorLocation();
	}

	virtual int32 GetCombatTargetTieBreakIndex() const override
	{
		return GetSpawnIndex();
	}

	virtual UReEchoCombatantComponent* GetCombatTargetCombatant() const override
	{
		return Combatant;
	}

	virtual EReEchoCombatFaction GetCombatFaction() const override
	{
		return EReEchoCombatFaction::EnemySide;
	}

	virtual EReEchoDamageSource GetCombatDamageSource() const override
	{
		return EReEchoDamageSource::Enemy;
	}

	virtual float ModifyIncomingRawDamage(const FReEchoHitIntent& Intent) const override;

	virtual bool
	IntersectsCombatPath(const FVector& PathStart, const FVector& PathEnd, float CarrierRadius) const override
	{
		return IntersectsProjectilePath(PathStart, PathEnd, CarrierRadius);
	}

	int32 GetSpawnIndex() const;
	EReEchoEnemyKind GetKind() const;

	FVector GetFacingDirection() const
	{
		return ResolveFacingDirection();
	}

	bool IntersectsProjectilePath(const FVector& PathStart, const FVector& PathEnd, float ProjectileRadius) const;
	FReEchoEnemyRuntimeState CaptureRuntimeState() const;
	void RestoreRuntimeState(const FReEchoEnemyRuntimeState& SavedState);
#if WITH_DEV_AUTOMATION_TESTS
	FReEchoEnemyActionIntent AdvanceBehaviorForTests(const FReEchoEnemySenseSnapshot& Sense, float DeltaSeconds);
	void AdvanceEnemyProjectilesForTests(float DeltaSeconds);
#endif

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	FName EnemyId = NAME_None;
	void BindComposedComponents();
	void RefreshPresentationHierarchy();
	void RefreshFootRoot();
	void AlignToGameplayPlane();
	FReEchoEnemyActionIntent AdvanceBehavior(const FReEchoEnemySenseSnapshot& Sense, float DeltaSeconds);
	void ApplyActionIntent(const FReEchoEnemyActionIntent& Intent);
	FVector ResolveCrowdMovement(const FVector& DesiredMovementDelta, const FVector& TargetLocation) const;
	void RefreshCrowdCollisionIgnores();
	void ClearCrowdCollisionIgnores();
	void ApplyBossIntent(const struct FReEchoBossIntent& Intent);
	void ApplyBossHit(const struct FReEchoBossIntent& Intent, AActor* Target, const FVector& HitLocation);
	void AdvanceEnemyProjectiles(float DeltaSeconds);
	void PublishSpecialActionTransition(const FReEchoEnemyLogicSnapshot& PreviousSnapshot,
	                                    const FReEchoEnemyActionIntent& Intent);
	void PublishProjectileEvent(EReEchoEnemyProjectileEventType Type,
	                            const FReEchoEnemyProjectileRuntimeState& Projectile) const;
	void HandlePhaseTransitionIntent(const FReEchoEnemyActionIntent& Intent);
	// WS4 (Plan 68): after a blood-depleted second-phase transition, resize this boss to its phase-two maximum health
	// (from BossPhases.PhaseMaxHealth) and refill it there. No-op unless the phase is RefillToMaximum.
	void ApplyBloodDepletedPhase2MaxHealth();
	const FReEchoEnemyAbilityDefinition* FindAbility(FName AbilityId) const;
	FVector ResolveFacingDirection() const;
	FVector ResolveBossTeleportDestination(const FVector& TargetLocation);
	FReEchoEnemyPresentationSnapshot BuildPresentationSnapshot(bool bMoving) const;

	UFUNCTION()
	void HandleCombatDeath(const FReEchoDamageEvent& Event);

	UPROPERTY(VisibleAnywhere, Category = "Abilities")
	TObjectPtr<UAbilitySystemComponent> AbilitySystem;
	UPROPERTY(VisibleAnywhere, Category = "Abilities")
	TObjectPtr<UReEchoCombatAttributeSet> CombatAttributes;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBoxComponent> Collision;
	UPROPERTY(VisibleAnywhere,
	          BlueprintReadOnly,
	          Category = "Character Scene|Presentation",
	          meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> PresentationRoot;
	UPROPERTY(VisibleAnywhere,
	          BlueprintReadOnly,
	          Category = "Character Scene|Ground",
	          meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> FootRoot;
	UPROPERTY(VisibleAnywhere,
	          BlueprintReadOnly,
	          Category = "Character Scene|Presentation",
	          meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> PresentationMotionRoot;
	UPROPERTY(VisibleAnywhere,
	          BlueprintReadOnly,
	          Category = "Character Scene|Flipbook",
	          meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> FlipbookRoot;
	UPROPERTY(VisibleAnywhere,
	          BlueprintReadOnly,
	          Category = "Character Scene|Ground",
	          meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> GroundRoot;
	UPROPERTY(VisibleAnywhere,
	          BlueprintReadOnly,
	          Category = "Character Scene|Effects",
	          meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> EffectsRoot;
	/** Blueprint-editable origin for outgoing attack, windup and dash effects. */
	UPROPERTY(VisibleAnywhere,
	          BlueprintReadOnly,
	          Category = "Character Scene|Effects",
	          meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> AttackVfxRoot;
	/** Blueprint-editable origin for effects played when this enemy is hurt. */
	UPROPERTY(VisibleAnywhere,
	          BlueprintReadOnly,
	          Category = "Character Scene|Effects",
	          meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> HurtVfxRoot;
	UPROPERTY(VisibleAnywhere,
	          BlueprintReadOnly,
	          Category = "Character Scene|Ground",
	          meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> GroundShadow;
	UPROPERTY(VisibleAnywhere,
	          BlueprintReadOnly,
	          Category = "Character Scene|Flipbook",
	          meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UReEcho2DAnimationComponent> SequenceAnimation;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UReEcho2DPresentationController> PresentationController;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UReEcho2DFrameCollisionDriver> FrameCollisionDriver;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UReEcho2DSceneLightingComponent> SceneLighting;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UReEchoCombatantComponent> Combatant;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UReEchoCombatEventsComponent> CombatEvents;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UReEchoCombatAudioAdapterComponent> CombatAudioAdapter;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UReEchoCombatVfxComponent> CombatVfx;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UReEchoCombatPresentationCoordinator> CombatPresentationCoordinator;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UReEchoEnemyLogicComponent> EnemyLogic;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UReEchoEnemyEventsComponent> EnemyEvents;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UReEchoEnemyPresentationComponent> EnemyPresentation;
	UPROPERTY()
	TObjectPtr<UReEchoEnemyRosterComponent> EnemyRoster;

	/** Uniform Blueprint-authored scale for collision, Flipbook, shadow, effects and presentation anchors. */
	UPROPERTY(EditDefaultsOnly,
	          BlueprintReadOnly,
	          Category = "Character Scene|Scale",
	          meta = (AllowPrivateAccess = "true", ClampMin = "0.01"))
	float CharacterScale = 1.0f;

	UPROPERTY()
	TArray<FReEchoEnemyProjectileRuntimeState> BossProjectiles;

	bool bVisualPlacementApplied = false;
	bool bAudioSpawnPosted = false;
	bool bEncounterSimulationSuspended = false;
	float CardStunnedUntilWorldTime = 0.0f;
	float CardMovementMultiplier = 1.0f;
	float GameplayPlaneWorldZ = 0.0f;
	float CrowdBlockedSeconds = 0.0f;
};
