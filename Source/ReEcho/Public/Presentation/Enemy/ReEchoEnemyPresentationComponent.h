#pragma once

#include "Combat/ReEchoCombatContracts.h"
#include "Components/ActorComponent.h"
#include "Enemies/ReEchoEnemyEventsComponent.h"
#include "Enemies/ReEchoEnemyTypes.h"
#include "ReEchoEnemyPresentationComponent.generated.h"

class AReEchoHealthBarActor;
class UBillboardComponent;
class UCapsuleComponent;
class UPointLightComponent;
class UReEcho2DAnimationComponent;
class UReEcho2DCharacterPresentationProfile;
class UReEcho2DFrameCollisionDriver;
class UReEcho2DPresentationController;
class UReEchoCombatantComponent;
class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
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
	int32 AppearanceId = 0;

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
	                         USceneComponent* InFlipbookRoot,
	                         USceneComponent* InEffectsRoot,
	                         UBillboardComponent* InCharacterSprite,
	                         UReEcho2DAnimationComponent* InSequenceAnimation,
	                         UReEcho2DPresentationController* InPresentationController,
	                         UReEcho2DFrameCollisionDriver* InFrameCollisionDriver,
	                         UStaticMeshComponent* InGroundShadow,
	                         UTextRenderComponent* InElementAuraRing,
	                         UTextRenderComponent* InElementAttachmentLabel,
	                         UPointLightComponent* InElementAuraLight,
	                         UCapsuleComponent* InCollision);
	void BindEventSources(AActor* InHost,
	                      UReEchoCombatantComponent* InCombatant,
	                      UReEchoEnemyEventsComponent* InEnemyEvents,
	                      UReEchoCombatEventsComponent* InCombatEvents);
	void ConfigureAppearance(EReEchoEnemyArchetype Archetype, int32 AppearanceId);
	void Advance(const FReEchoEnemyPresentationSnapshot& Snapshot, float DeltaSeconds);
	void RefreshElementAttachmentVisual();

	UBillboardComponent* GetCharacterSprite() const
	{
		return CharacterSprite;
	}

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleActionCommitted(const FReEchoEnemyActionCommittedEvent& Event);
	UFUNCTION()
	void HandleBossIntent(const FReEchoBossIntent& Intent);
	UFUNCTION()
	void HandleFuseChanged(const FReEchoEnemyFuseEvent& Event);
	UFUNCTION()
	void HandleElementStateChanged(const FReEchoElementStateChangedEvent& Event);
	UFUNCTION()
	void HandleCombatHurt(const FReEchoDamageEvent& Event);
	UFUNCTION()
	void HandleCombatDeath(const FReEchoDamageEvent& Event);

	void ApplyVisual(EReEchoEnemyArchetype Archetype, int32 AppearanceId);
	void ApplyPresentationMotion(const FVector& Offset, const FVector& Scale);
	UReEcho2DCharacterPresentationProfile* ResolveEnemyPresentationProfile(int32 AppearanceId) const;
	void ResetTransientRoot();
	void UpdateCameraFacing(const FReEchoEnemyPresentationSnapshot& Snapshot);
	void UpdateElementAttachmentFacing();
	void UpdateHitReaction(const FReEchoEnemyPresentationSnapshot& Snapshot);
	void UpdateSpriteAnimation(const FReEchoEnemyPresentationSnapshot& Snapshot, float DeltaSeconds);
	void UpdateDeathAnimation(float DeltaSeconds);

	UPROPERTY()
	TObjectPtr<AActor> Host;
	UPROPERTY()
	TObjectPtr<UReEchoCombatantComponent> Combatant;
	UPROPERTY()
	TObjectPtr<UReEchoEnemyEventsComponent> EnemyEvents;
	UPROPERTY()
	TObjectPtr<UReEchoCombatEventsComponent> CombatEvents;
	UPROPERTY()
	TObjectPtr<USceneComponent> PresentationRoot;
	UPROPERTY()
	TObjectPtr<USceneComponent> VisualEffectRoot;
	UPROPERTY()
	TObjectPtr<USceneComponent> FlipbookRoot;
	UPROPERTY()
	TObjectPtr<USceneComponent> EffectsRoot;
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
	TObjectPtr<UTextRenderComponent> ElementAuraRing;
	UPROPERTY()
	TObjectPtr<UTextRenderComponent> ElementAttachmentLabel;
	UPROPERTY()
	TObjectPtr<UPointLightComponent> ElementAuraLight;
	UPROPERTY()
	TObjectPtr<UCapsuleComponent> Collision;
	UPROPERTY()
	TObjectPtr<UTexture2D> BossTexture;
	UPROPERTY()
	TObjectPtr<UReEcho2DCharacterPresentationProfile> GruntPresentationProfile;
	UPROPERTY()
	TObjectPtr<UReEcho2DCharacterPresentationProfile> RabbitDollPresentationProfile;
	UPROPERTY()
	TObjectPtr<UReEcho2DCharacterPresentationProfile> GoatPriestPresentationProfile;
	UPROPERTY()
	TObjectPtr<UReEcho2DCharacterPresentationProfile> FoxPresentationProfile;
	UPROPERTY()
	TObjectPtr<AReEchoHealthBarActor> HealthBar;

	FVector BaseVisualLocation = FVector::ZeroVector;
	FVector BaseVisualScale = FVector::OneVector;
	FVector BaseFlipbookLocation = FVector::ZeroVector;
	FVector BaseFlipbookScale = FVector::OneVector;
	FVector BaseEffectsLocation = FVector::ZeroVector;
	FVector BaseEffectsScale = FVector::OneVector;
	FVector ShakeDirection = FVector::ZeroVector;
	float VisualTime = 0.0f;
	float AttackVisualRemaining = 0.0f;
	float DeathVisualRemaining = 0.0f;
	float LastFuseRemaining = 0.0f;
	float LastFuseDuration = 0.0f;
	bool bHitVisualActive = false;
	bool bDeathVisualActive = false;
};
