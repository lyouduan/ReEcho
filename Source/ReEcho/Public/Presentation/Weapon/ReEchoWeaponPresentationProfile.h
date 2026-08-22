#pragma once

#include "Engine/DataAsset.h"
#include "ReEchoWeaponPresentationProfile.generated.h"

class UNiagaraSystem;
class UTexture2D;

UENUM(BlueprintType)
enum class EReEchoWeaponMotionMode : uint8
{
	None,
	FullSpin
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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX", meta = (EditCondition = "bEnabled"))
	int32 SortPriorityOffset = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX", meta = (EditCondition = "bEnabled"))
	bool bStopWhenPhaseEnds = true;

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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Motion")
	EReEchoWeaponMotionMode MotionMode = EReEchoWeaponMotionMode::None;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Motion", meta = (ClampMin = "0.0"))
	float MotionDurationSeconds = 0.18f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX") FReEchoWeaponVfxSlot Charge;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX") FReEchoWeaponVfxSlot Travel;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX") FReEchoWeaponVfxSlot DamageApplied;
	/** Existing committed slash semantics are explicit instead of being mislabeled as target damage. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX") FReEchoWeaponVfxSlot AttackCommitted;
	UFUNCTION(BlueprintPure, Category = "Validation") bool IsSlotValid(const FReEchoWeaponVfxSlot& Slot) const;
};
