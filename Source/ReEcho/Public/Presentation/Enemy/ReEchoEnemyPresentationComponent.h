#pragma once

#include "Combat/ReEchoCombatContracts.h"
#include "Components/ActorComponent.h"
#include "Enemies/ReEchoEnemyEventsComponent.h"
#include "Enemies/ReEchoEnemyTypes.h"
#include "Presentation/Combat/ReEchoCombatPresentationTypes.h"
#include "ReEchoEnemyPresentationComponent.generated.h"

class UBillboardComponent;
class UBoxComponent;
class UReEcho2DAnimationComponent;
class UReEcho2DCharacterPresentationProfile;
class UReEcho2DFrameCollisionDriver;
class UReEcho2DPresentationController;
class UReEcho2DPresentationCatalog;
class UReEchoCombatPresentationCoordinator;
class USceneComponent;
class UStaticMeshComponent;
class UTexture2D;

/** Host-aggregated, read-only input for enemy presentation. */
USTRUCT(BlueprintType)

struct REECHO_API FReEchoEnemyPresentationSnapshot
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EReEchoEnemyArchetype Archetype = EReEchoEnemyArchetype::Grunt;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EReEchoEnemyBehaviorPhase Phase = EReEchoEnemyBehaviorPhase::Idle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName PresentationId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 SpawnIndex = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector FacingDirection = FVector::ForwardVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector KnockbackVelocity = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float HealthRatio = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float FuseRemainingSeconds = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float FuseDurationSeconds = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float HitReactionRemainingSeconds = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float HitReactionDurationSeconds = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EReEchoElement AttachedElement = EReEchoElement::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bMoving = false;

	/** Gameplay-owned action disable sampled by the Host; presentation only pauses/resumes the current frame. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bStunned = false;
};

/**
 * Enemy-only presentation adapter. It owns visual state and consumes Enemy/Combat results, but has
 * no command path back into AI, damage, cooldown, movement or encounter flow.
 */
UCLASS(ClassGroup = (ReEcho), meta = (BlueprintSpawnableComponent))

class REECHO_API UReEchoEnemyPresentationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UReEchoEnemyPresentationComponent();

	void ConfigureComponents(USceneComponent* InPresentationRoot,
	                         USceneComponent* InVisualEffectRoot,
	                         USceneComponent* InFootRoot,
	                         USceneComponent* InFlipbookRoot,
	                         USceneComponent* InEffectsRoot,
	                         USceneComponent* InBossWeaponRoot,
	                         USceneComponent* InBossWeaponFacingRoot,
	                         USceneComponent* InBossWeaponTipRoot,
	                         UBillboardComponent* InBossWeaponSprite,
	                         UBillboardComponent* InCharacterSprite,
	                         UReEcho2DAnimationComponent* InSequenceAnimation,
	                         UReEcho2DPresentationController* InPresentationController,
	                         UReEcho2DFrameCollisionDriver* InFrameCollisionDriver,
	                         UStaticMeshComponent* InGroundShadow,
	                         UBoxComponent* InCollision);
	void BindEventSources(AActor* InHost,
	                      UReEchoEnemyEventsComponent* InEnemyEvents,
	                      UReEchoCombatEventsComponent* InCombatEvents);
	void SetPresentationCatalog(UReEcho2DPresentationCatalog* InPresentationCatalog);
	void ConfigureAppearance(FName PresentationId);
	/** Try the optional Born presentation without affecting the committed gameplay spawn. */
	bool TryPlayBorn();
	/** Read-only presentation state used by the Host to delay only the start of Phase2. */
	bool IsBornPlaying() const;
	/** Enter the only visible death presentation. Returns false when no valid Death clip exists. */
	bool BeginTerminalDeath(FSimpleDelegate OnCompleted, float& OutExpectedDurationSeconds);
	/** Freezes the current animation before stun-driven gameplay cancellation events are published. */
	void SetStunPaused(bool bPaused);
	void Advance(const FReEchoEnemyPresentationSnapshot& Snapshot, float DeltaSeconds);
	/** Skill03 keeps gameplay at the locked impact point while its presentation descends into that point. */
	static FVector ResolveBlinkSlamVisualOffset(float RemainingSeconds, float DurationSeconds, float StartHeightCm);
	/** MoonStaff billboard pivot is centered, so its stable local tip is half the authored world length upward. */
	static FVector ResolveBossWeaponTipOffset(float HeldLengthCm);
#if WITH_DEV_AUTOMATION_TESTS
	void ConsumePresentationActionForTests(const FReEchoPresentationActionEvent& Event);
#endif

	UBillboardComponent* GetCharacterSprite() const
	{
		return CharacterSprite;
	}

#if WITH_DEV_AUTOMATION_TESTS
	static FVector ResolveBossWeaponFacingOffsetForTests(float FacingSign,
	                                                     const FVector& RightFacingOffset,
	                                                     const FVector& LeftFacingOffset);
#endif

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleBossIntent(const FReEchoBossIntent& Intent);
	UFUNCTION()
	void HandlePresentationAction(const FReEchoPresentationActionEvent& Event);
	UFUNCTION()
	void HandlePhaseTransition(const FReEchoEnemyPhaseTransitionEvent& Event);
	UFUNCTION()
	void HandleFuseChanged(const FReEchoEnemyFuseEvent& Event);
	UFUNCTION()
	void HandleCombatHurt(const FReEchoDamageEvent& Event);
	UFUNCTION()
	void HandleElementReactionResolved(const FReEchoElementReactionResolvedEvent& Event);

	void ApplyVisual(FName PresentationId);
	void ApplyPresentationMotion(const FVector& Offset, const FVector& Scale);
	void RefreshFootpointAlignment();
	void RefreshGroundShadowFromFlipbook();
	void ResetTransientRoot();
	void UpdateCameraFacing(const FReEchoEnemyPresentationSnapshot& Snapshot);
	void UpdateHitReaction(const FReEchoEnemyPresentationSnapshot& Snapshot);
	void UpdateSpriteAnimation(const FReEchoEnemyPresentationSnapshot& Snapshot, float DeltaSeconds);
	void ConfigureBossWeapon(FName PresentationId);
	void UpdateBossWeaponMotion(float DeltaSeconds);
	void UpdateBossBlinkSlamMotion(float DeltaSeconds);
	void RefreshBossWeaponFacingOffset(float FacingSign);

	UPROPERTY()
	TObjectPtr<AActor> Host;
	UPROPERTY()
	TObjectPtr<UReEchoEnemyEventsComponent> EnemyEvents;
	UPROPERTY()
	TObjectPtr<UReEchoCombatEventsComponent> CombatEvents;
	UPROPERTY()
	TObjectPtr<UReEchoCombatPresentationCoordinator> CombatPresentationCoordinator;
	UPROPERTY()
	TObjectPtr<USceneComponent> PresentationRoot;
	UPROPERTY()
	TObjectPtr<USceneComponent> VisualEffectRoot;
	UPROPERTY()
	TObjectPtr<USceneComponent> FootRoot;
	UPROPERTY()
	TObjectPtr<USceneComponent> GroundRoot;
	UPROPERTY()
	TObjectPtr<USceneComponent> FlipbookRoot;
	UPROPERTY()
	TObjectPtr<USceneComponent> EffectsRoot;
	UPROPERTY()
	TObjectPtr<USceneComponent> BossWeaponRoot;
	UPROPERTY()
	TObjectPtr<USceneComponent> BossWeaponFacingRoot;
	UPROPERTY()
	TObjectPtr<USceneComponent> BossWeaponTipRoot;
	UPROPERTY()
	TObjectPtr<UBillboardComponent> BossWeaponSprite;
	UPROPERTY()
	TObjectPtr<UBillboardComponent> CharacterSprite;
	UPROPERTY()
	TObjectPtr<UReEcho2DAnimationComponent> SequenceAnimation;
	UPROPERTY()
	TObjectPtr<UReEcho2DPresentationController> PresentationController;
	UPROPERTY()
	TObjectPtr<UReEcho2DFrameCollisionDriver> FrameCollisionDriver;
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> GroundShadow;
	UPROPERTY()
	TObjectPtr<UBoxComponent> Collision;
	UPROPERTY()
	TObjectPtr<UReEcho2DPresentationCatalog> PresentationCatalog;
	UPROPERTY()
	TObjectPtr<UReEcho2DCharacterPresentationProfile> ActiveProfile;

	FVector AuthoredMotionLocation = FVector::ZeroVector;
	FVector BaseVisualScale = FVector::OneVector;
	FVector CalculatedFootAlignmentOffset = FVector::ZeroVector;
	FVector BaseFlipbookLocation = FVector::ZeroVector;
	FVector BaseFlipbookScale = FVector::OneVector;
	FVector BaseEffectsLocation = FVector::ZeroVector;
	FVector BaseEffectsScale = FVector::OneVector;
	FVector AuthoredGroundRootLocation = FVector::ZeroVector;
	FVector AuthoredGroundShadowScale = FVector::OneVector;
	FVector ShakeDirection = FVector::ZeroVector;
	float VisualTime = 0.0f;
	float AttackVisualRemaining = 0.0f;
	float LastFuseRemaining = 0.0f;
	float LastFuseDuration = 0.0f;
	float BossWeaponSwingRemaining = 0.0f;
	float BossWeaponSwingDuration = 0.0f;
	float BossBlinkSlamRemaining = 0.0f;
	float BossBlinkSlamDuration = 0.0f;
	float BossBlinkSlamStartHeightCm = 0.0f;
	FRotator BossWeaponRestRotation = FRotator::ZeroRotator;
	FVector BossWeaponRightFacingOffset = FVector::ZeroVector;
	FVector BossWeaponLeftFacingOffset = FVector::ZeroVector;
	bool bHitVisualActive = false;
	bool bDeathVisualActive = false;
	bool bStunPaused = false;
	bool bCancelAttackWhenStunClears = false;
};
