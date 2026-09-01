#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Enemies/ReEchoEnemyTypes.h"
#include "ReEchoBossPhase3Config.generated.h"

USTRUCT(BlueprintType)

struct FReEchoBossTransformPresentation
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.1"))
	float DurationSeconds = 2.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float BurstSeconds = 1.15f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.01"))
	float CameraMoveSeconds = 0.8f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.01"))
	float CameraPushSeconds = 1.3f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float CameraWidthRatio = 0.85f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "0.6"))
	float DarkenAmount = 0.15f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float ShakeAmplitudeCm = 16.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float EffectScale = 1.0f;
};

/** Programmer-authored hidden Sheep Boss phase data. This deliberately bypasses the designer XLSX/CSV pipeline. */
UCLASS(BlueprintType)

class REECHO_API UReEchoBossPhase3Config : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transformation")
	FReEchoBossTransformPresentation Phase2Presentation;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transformation")
	FReEchoBossTransformPresentation Phase3Presentation = MakePhase3Presentation();
	/** Presentation only: never kills, heals, moves gameplay actors or awards drops. Phase1 -> Phase2 only. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transformation|Sacrifice")
	bool bEnablePhase2Sacrifice = true;
	UPROPERTY(EditAnywhere,
	          BlueprintReadOnly,
	          Category = "Transformation|Sacrifice",
	          meta = (ClampMin = "0", ClampMax = "8"))
	int32 SacrificeMaxEnemies = 8;
	/** Fixed cinematic summons, independent of nearby enemies; normal waves still count these hosts. */
	UPROPERTY(EditAnywhere,
	          BlueprintReadOnly,
	          Category = "Transformation|Sacrifice",
	          meta = (ClampMin = "0", ClampMax = "20"))
	int32 SacrificeSummonCount = 20;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transformation|Sacrifice")
	TArray<FName> SacrificeSummonTypes = {TEXT("M_SLIME"), TEXT("M_RABBIT"), TEXT("M_FOX")};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transformation|Sacrifice", meta = (ClampMin = "0.0"))
	float SacrificeRadiusCm = 800.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transformation|Sacrifice", meta = (ClampMin = "0.0"))
	float SacrificeMarkSeconds = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transformation|Sacrifice", meta = (ClampMin = "0.01"))
	float SacrificeRiseSeconds = 1.2f;
	/** Serialized name retained for existing DAs; now visible airborne hold, not hiding. */
	UPROPERTY(EditAnywhere,
	          BlueprintReadOnly,
	          Category = "Transformation|Sacrifice",
	          meta = (ClampMin = "0.0", DisplayName = "Sacrifice Hover Seconds"))
	float SacrificeHiddenSeconds = 1.5f;
	/** Serialized name retained; now accelerated descent, not fade-in. */
	UPROPERTY(EditAnywhere,
	          BlueprintReadOnly,
	          Category = "Transformation|Sacrifice",
	          meta = (ClampMin = "0.01", DisplayName = "Sacrifice Fall Seconds"))
	float SacrificeReappearSeconds = 0.4f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transformation|Sacrifice", meta = (ClampMin = "0.0"))
	float SacrificeHoverShakeCm = 6.0f;

	static FReEchoBossTransformPresentation MakePhase3Presentation()
	{
		FReEchoBossTransformPresentation Result;
		Result.DurationSeconds = 2.4f;
		Result.BurstSeconds = 1.3f;
		Result.CameraWidthRatio = 0.7f;
		Result.DarkenAmount = 0.25f;
		Result.ShakeAmplitudeCm = 28.0f;
		Result.EffectScale = 1.3f;
		return Result;
	}
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FName BossEnemyId = TEXT("M_SHEEP");

	/** Highest phase in which the existing phase-one/two abilities remain selectable. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase3", meta = (ClampMin = "1"))
	int32 ExistingAbilityMaxPhaseIndex = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase3", meta = (ClampMin = "0.0"))
	float TriggerSeconds = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase3", meta = (ClampMin = "1.0"))
	float PhaseMaxHealth = 500.0f;

	/**
	 * Egao Party: phase three resolves to (phase1 + phase2) * this multiplier, so 4/3 lands the
	 * sheep exactly on the authored 200000 (100000 + 50000). The shipped build used 5.0.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase3", meta = (ClampMin = "1.0"))
	float PreviousPhasesHealthMultiplier = 4.0f / 3.0f;

	/** Repeat original ordinary waves, keeping each batch count unchanged. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase3", meta = (ClampMin = "1", ClampMax = "20"))
	int32 SpawnPlanRepetitions = 5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase3|Opening", meta = (ClampMin = "1", ClampMax = "3"))
	int32 OpeningStrikeCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase3|Opening", meta = (ClampMin = "0.0"))
	float OpeningRepulseDistanceCm = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase3|Opening", meta = (ClampMin = "0.01"))
	float OpeningRepulseSeconds = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill03")
	FName AbilityId = TEXT("M_SHEEP_BlinkSlam");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill03", meta = (ClampMin = "0.0"))
	float Damage = 24.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill03", meta = (ClampMin = "0.0"))
	float WindupSeconds = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill03", meta = (ClampMin = "0.0"))
	float ActiveSeconds = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill03", meta = (ClampMin = "0.0"))
	float RecoverySeconds = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill03", meta = (ClampMin = "0.0"))
	float CooldownSeconds = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill03", meta = (ClampMin = "0.0"))
	float MaxRangeCm = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill03", meta = (ClampMin = "1.0"))
	float RadiusCm = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill03", meta = (ClampMin = "1.0"))
	float TeleportOffsetCm = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill03", meta = (ClampMin = "1", ClampMax = "3"))
	int32 ComboMin = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill03", meta = (ClampMin = "1", ClampMax = "3"))
	int32 ComboMax = 3;

	/** Applies this optional programmer DA as a narrow overlay on the normal compiled enemy definition. */
	bool ApplyTo(FName EnemyId, FReEchoEnemyDefinition& InOutDefinition) const;
};
