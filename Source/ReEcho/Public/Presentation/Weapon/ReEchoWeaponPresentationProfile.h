#pragma once

#include "Engine/DataAsset.h"
#include "ReEchoWeaponPresentationProfile.generated.h"

class UNiagaraSystem;
class UTexture2D;

UENUM(BlueprintType)
enum class EReEchoWeaponMotionMode : uint8
{
	None,
	FullSpin,
	TripleSwing60
};

/** Texture axis normalized to the character reference height for held-weapon layout. */
UENUM(BlueprintType)
enum class EReEchoHeldWeaponSizeAxis : uint8
{
	Width,
	Height
};

UENUM(BlueprintType)
enum class EReEchoHeldWeaponMirrorRule : uint8
{
	Never,
	WhenFacingLeft,
	WhenFacingRight
};

UENUM(BlueprintType)
enum class EReEchoWeaponVfxSpawnMode : uint8
{
	AttachToAttackRoot,
	AttachToCarrier,
	AttachToDamagedTarget,
	SpawnAtWorldLocation
};

USTRUCT(BlueprintType)

struct REECHO_API FReEchoWeaponVfxSlot
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX") bool bEnabled = false;
	UPROPERTY(EditAnywhere,
	          BlueprintReadOnly,
	          Category = "VFX",
	          meta = (EditCondition = "bEnabled", EditConditionHides))
	TSoftObjectPtr<UNiagaraSystem> System;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX", meta = (EditCondition = "bEnabled"))
	EReEchoWeaponVfxSpawnMode SpawnMode = EReEchoWeaponVfxSpawnMode::SpawnAtWorldLocation;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX", meta = (EditCondition = "bEnabled"))
	FName AttachPoint = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX", meta = (EditCondition = "bEnabled"))
	FTransform Offset = FTransform::Identity;
	/** Cancel attachment-root scale while retaining Offset.Scale as the authored world-size multiplier. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX", meta = (EditCondition = "bEnabled"))
	bool bPreserveWorldSize = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX", meta = (EditCondition = "bEnabled"))
	int32 SortPriorityOffset = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX", meta = (EditCondition = "bEnabled"))
	bool bStopWhenPhaseEnds = true;
	/** One-shot duration used when presentation reverses this Niagara through Desired Age. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX", meta = (EditCondition = "bEnabled", ClampMin = "0.01"))
	float PlaybackDurationSeconds = 0.6f;

	bool IsConfigured() const
	{
		return bEnabled && !System.IsNull();
	}
};

/** Editor-authored presentation only. Gameplay cadence, damage, geometry and carrier remain in weapon definitions. */
UCLASS(BlueprintType)

class REECHO_API UReEchoWeaponPresentationProfile : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity") FName WeaponVisualKey = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Held Visual") TSoftObjectPtr<UTexture2D> HeldTexture;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Held Visual") TSoftObjectPtr<UTexture2D> LegacyAttackTexture;
	/** Held weapon length as a ratio of the owning character profile's stable WorldHeight. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Held Visual", meta = (ClampMin = "0.01"))
	float HeldLengthRatio = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Held Visual")
	EReEchoHeldWeaponSizeAxis HeldSizeAxis = EReEchoHeldWeaponSizeAxis::Height;
	/** World-space visual offset normalized to the owning character's final reference height. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Held Visual")
	FVector HeldOffsetRatio = FVector::ZeroVector;
	/** Additional right-facing offset from an authored host mount, normalized to character WorldHeight. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Held Visual|Facing Offset")
	FVector HeldRightFacingOffsetRatio = FVector::ZeroVector;
	/** Additional left-facing offset from an authored host mount, normalized to character WorldHeight. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Held Visual|Facing Offset")
	FVector HeldLeftFacingOffsetRatio = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Held Visual")
	FRotator HeldRotationOffset = FRotator::ZeroRotator;
	/** Additional rotation around the camera-facing weapon plane; positive values turn counter-clockwise on screen. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Held Visual")
	float HeldPlanarAngleOffsetDegrees = 0.0f;
	/** Horizontal UV mirror rule for camera-facing held weapon billboards. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Held Visual")
	EReEchoHeldWeaponMirrorRule HeldMirrorRule = EReEchoHeldWeaponMirrorRule::Never;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Held Visual")
	bool bOverrideHeldLength = false;
	UPROPERTY(EditAnywhere,
	          BlueprintReadOnly,
	          Category = "Held Visual",
	          meta = (ClampMin = "1.0", EditCondition = "bOverrideHeldLength", EditConditionHides))
	float HeldLengthOverrideCm = 100.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Motion")
	EReEchoWeaponMotionMode MotionMode = EReEchoWeaponMotionMode::None;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Motion", meta = (ClampMin = "0.0"))
	float MotionDurationSeconds = 0.18f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX") FReEchoWeaponVfxSlot Charge;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX") FReEchoWeaponVfxSlot Travel;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX") FReEchoWeaponVfxSlot DamageApplied;
	/** Existing committed slash semantics are explicit instead of being mislabeled as target damage. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX") FReEchoWeaponVfxSlot AttackCommitted;
	/** Visual-only BFS link cadence. Damage and ReactionLinks resolve immediately before this delay is consumed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX|Conduct", meta = (ClampMin = "0.0"))
	float ConductLinkPropagationDelaySeconds = 0.0f;
	UFUNCTION(BlueprintPure, Category = "Validation") bool IsSlotValid(const FReEchoWeaponVfxSlot& Slot) const;
};
