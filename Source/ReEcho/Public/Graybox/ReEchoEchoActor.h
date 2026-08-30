#pragma once

#include "CoreMinimal.h"
#include "Combat/ReEchoCombatTarget.h"
#include "Core/ReEchoTypes.h"
#include "Combat/ReEchoCombatTarget.h"
#include "GameFramework/Actor.h"
#include "ReEchoEchoActor.generated.h"

class AReEchoWeaponActor;
class AReEchoPlayerPawn;
class UBillboardComponent;
class UReEcho2DAnimationComponent;
class UReEcho2DCharacterPresentationProfile;
class UReEcho2DFrameCollisionDriver;
class UReEcho2DPresentationCatalog;
class UReEcho2DPresentationController;
class UReEcho2DSceneLightingComponent;
class UReEchoCombatantComponent;
class UReEchoCombatEventsComponent;
class UReEchoCombatAudioAdapterComponent;
class UReEchoCombatVfxComponent;
class UReEchoPlaybackComponent;
class USceneComponent;
class UStaticMeshComponent;
class UTexture2D;
struct FReEchoCsvDataSnapshot;

#if WITH_DEV_AUTOMATION_TESTS
struct FReEchoEchoSpatialPresentationSnapshot
{
	FTransform ActorTransform;
	FTransform PresentationRootTransform;
	FTransform FootRootTransform;
	FTransform MotionRootTransform;
	FTransform FlipbookRootTransform;
	FTransform EffectsRootTransform;
	FTransform AttackVfxRootTransform;
	FTransform HurtVfxRootTransform;
	FTransform GroundRootTransform;
	FTransform GroundShadowTransform;
	FTransform RendererTransform;
	float NormalizedCharacterHeight = 0.0f;
	float PresentedCharacterWidth = 0.0f;
	float ShadowReferenceWidth = 0.0f;
};
#endif

/** 回响分身：使用录制构筑中的锁定武器，按固定时间轴重放历史位置与技能。 */
UCLASS()

class REECHO_API AReEchoEchoActor : public AActor, public IReEchoCombatTarget, public IReEchoCombatAffiliation
{
	GENERATED_BODY()

public:
	float GetVisualFacingSign() const
	{
		return VisualFacingSign;
	}

	AReEchoEchoActor();

	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 装载一份不可变录制，并设置回响造成伤害的效率倍率。 */
	bool InitializeEcho(const FReEchoRecording& Recording,
	                    float Efficiency,
	                    TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot);
	/** 将回响推进到指定遭遇时间，补播期间跨过的所有事件。 */
	void AdvanceEcho(float EncounterTime);
	/** 根据录制角色 ID 选择对应的回响形态。 */
	bool ConfigureEchoAppearance(FName CharacterId);
	/** Presentation-owned icon for this Echo appearance on the combat minimap. */
	UTexture2D* GetMinimapIconTexture() const;
	/** 返回效率修正后的当前回响战斗属性。 */
	const FReEchoStatBlock& GetCurrentStats() const;
	float GetCurrentHealth() const;
	void ConfigureCardRules(const FReEchoCardRuleSnapshot& Rules, const FReEchoStatBlock& PlayerStats);
	void PlayCardAuraPulse(const FReEchoCardRuleSnapshot& Rules);
	/** Replays only the one-shot birth presentation at this Echo's current GroundRoot. */
	bool PlayBornVfx();
	bool IsBornVfxPlaying() const;
	/** Freezes replay and attacks during presentation-only stage transitions while leaving visual Tick active. */
	void SetTransitionGameplaySuspended(bool bSuspended);
	bool IsTransitionGameplaySuspended() const
	{
		return bTransitionGameplaySuspended;
	}
	/** Positions this Echo for a paused transition while withholding its actor/weapon presentation and birth VFX. */
	void PrepareDeferredBornReveal(float EncounterTime);
	/** Hides this Echo at its current playback position and arms a fresh birth reveal. */
	void PrepareBornRevealAtCurrentLocation();
	/** Consumes the pending birth VFX while the Echo remains hidden. */
	bool BeginDeferredBornReveal();
	/** Makes the prepared Echo and weapon visible; safe to call as transition fail-open cleanup. */
	void CompleteDeferredBornReveal();
	virtual bool IsCombatTargetAlive() const override;

	virtual FVector GetCombatTargetLocation() const override
	{
		return GetActorLocation();
	}

	virtual int32 GetCombatTargetTieBreakIndex() const override
	{
		return GetUniqueID();
	}

	virtual UReEchoCombatantComponent* GetCombatTargetCombatant() const override
	{
		return Combatant;
	}

	virtual bool
	IntersectsCombatPath(const FVector& PathStart, const FVector& PathEnd, float CarrierRadius) const override;
	virtual void ModifyOutgoingHit(FReEchoHitIntent& Intent) const override;
	virtual void NotifyReactionResolved(FName ReactionId) const override;
	virtual float GetReactionDamageMultiplier(FName ReactionId) const override;
	virtual bool HasInfiniteStackingBurn() const override;
	virtual void NotifyKillResolved(FName TargetDefinitionId) const override;
	virtual void NotifyHitResolved(const FReEchoHitResolved& Result) const override;
	virtual void NotifyNegativeStatusApplied(FName StatusId) const override;
	virtual void NotifyDefeated(EReEchoDamageSource DamageSource) const override;
	FString GetPinnedWeaponDomainRevision() const;
	FName GetEquippedWeaponId() const;
	FVector EvaluateRecordedPosition(float EncounterTime) const;

#if WITH_DEV_AUTOMATION_TESTS
	void ApplyPlayerSpatialAuthoringForTests(const AReEchoPlayerPawn& Player);
	void RefreshSpatialPresentationForTests();
	FReEchoEchoSpatialPresentationSnapshot CaptureSpatialPresentationForTests() const;
	const UReEcho2DCharacterPresentationProfile* GetSpatialProfileForTests() const;

	bool IsDeferredBornRevealPreparedForTests() const
	{
		return bDeferredBornReveal;
	}

	bool IsBornVfxPendingForTests() const
	{
		return bBornVfxPending;
	}

	void QueueBornVfxForTests()
	{
		bBornVfxPending = true;
	}
#endif

	FVector GetAttackAimDirection() const
	{
		return AttackAimDirection;
	}

	/** 返回录制的历史位置（世界 XY 平面），供右上角小地图绘制回响轨迹。 */
	const TArray<FVector2D>& GetRecordedPath() const
	{
		return RecordedPath;
	}

	virtual EReEchoCombatFaction GetCombatFaction() const override
	{
		return EReEchoCombatFaction::PlayerSide;
	}

	virtual EReEchoDamageSource GetCombatDamageSource() const override
	{
		return EReEchoDamageSource::Echo;
	}

private:
	bool bBornVfxPending = false;
	bool bDeferredBornReveal = false;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> PresentationRoot;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> FootRoot;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> PresentationMotionRoot;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> FlipbookRoot;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> GroundRoot;
	UPROPERTY(VisibleAnywhere,
	          BlueprintReadOnly,
	          Category = "Character Scene|Effects",
	          meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> EffectsRoot;
	UPROPERTY(VisibleAnywhere,
	          BlueprintReadOnly,
	          Category = "Character Scene|Effects",
	          meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> AttackVfxRoot;
	UPROPERTY(VisibleAnywhere,
	          BlueprintReadOnly,
	          Category = "Character Scene|Effects",
	          meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> HurtVfxRoot;
	UPROPERTY(VisibleAnywhere,
	          BlueprintReadOnly,
	          Category = "Character Scene|Effects",
	          meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> EchoAuraVfxRoot;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> GroundShadow;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBillboardComponent> CharacterSprite;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UReEcho2DAnimationComponent> EchoAnimation;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UReEcho2DPresentationController> PresentationController;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UReEcho2DFrameCollisionDriver> FrameCollisionDriver;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UReEcho2DSceneLightingComponent> SceneLighting;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UReEchoPlaybackComponent> Playback;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UReEchoCombatantComponent> Combatant;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UReEchoCombatEventsComponent> CombatEvents;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UReEchoCombatAudioAdapterComponent> CombatAudioAdapter;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UReEchoCombatVfxComponent> CombatVfx;

	UPROPERTY()
	TObjectPtr<AReEchoWeaponActor> Weapon;

	UPROPERTY()
	TArray<FVector2D> RecordedPath;

	UPROPERTY()
	TMap<FName, TObjectPtr<UTexture2D>> EchoTextures;

	UPROPERTY()
	TObjectPtr<UReEcho2DPresentationCatalog> EchoPresentationCatalog;
	UPROPERTY()
	TObjectPtr<UReEcho2DPresentationCatalog> PlayerPresentationCatalog;
	UPROPERTY(Transient)
	TObjectPtr<UReEcho2DCharacterPresentationProfile> ComposedPresentationProfile;
	UPROPERTY()
	TObjectPtr<UReEcho2DCharacterPresentationProfile> ActivePresentationProfile;
	UPROPERTY()
	TObjectPtr<UReEcho2DCharacterPresentationProfile> ActiveSpatialProfile;

	void RefreshPresentationProfile();
	void ApplyPlayerSpatialAuthoring(const AReEchoPlayerPawn& Player);
	void UpdatePresentationState();
	void RefreshFootpointAlignment();
	void RefreshGroundShadowFromFlipbook();
	void RefreshEchoAuraCenter();
	float CalculateSpatialShadowWidth() const;
	void UpdateFacingSign(const FVector& AimDirection);
	FName ConfiguredCharacterId = NAME_None;
	FVector BaseVisualLocation = FVector::ZeroVector;
	FVector BaseVisualScale = FVector::OneVector;
	FVector AuthoredMotionLocation = FVector::ZeroVector;
	FVector BaseEffectsLocation = FVector::ZeroVector;
	FVector BaseEffectsScale = FVector::OneVector;
	FVector CalculatedFootAlignmentOffset = FVector::ZeroVector;
	FVector AuthoredGroundRootLocation = FVector::ZeroVector;
	FVector AuthoredGroundShadowScale = FVector::OneVector;
	FVector LastPresentationLocation = FVector::ZeroVector;
	FVector AttackAimDirection = FVector::ForwardVector;
	float VisualFacingSign = 1.0f;
	int64 NextPresentationAttackInstanceId = 1;
	float AutoTargetRange = 1600.0f;
	float DamageEfficiency = 1.0f;
	bool bHasPresentationLocation = false;
	bool bAudioLifecycleStarted = false;
	bool bCanAttack = true;
	bool bTransitionGameplaySuspended = false;
};
