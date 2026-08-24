#pragma once

#include "CoreMinimal.h"
#include "Combat/ReEchoCombatContracts.h"
#include "Combat/ReEchoCombatTarget.h"
#include "GameFramework/Actor.h"
#include "ReEchoVfxPreviewActor.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;
class USceneComponent;
class SWidget;
class AReEchoArenaCameraActor;

UENUM(BlueprintType)
enum class EReEchoVfxPreviewMode : uint8
{
	RawNiagara,
	CombatSemantic,
	ElementSemantic,
	WeaponProfile
};

UENUM(BlueprintType)
enum class EReEchoVfxPreviewWeaponSlot : uint8
{
	Charge,
	Travel,
	DamageApplied,
	AttackCommitted
};

UENUM(BlueprintType)
enum class EReEchoVfxScenarioPreset : uint8
{
	SingleHit,
	Projectile,
	Conduct2Targets,
	ConductChain,
	RadiusBoundary,
	BurnState,
	Vaporize,
	CrowdStress
};

/** Editable endpoint pair; conversion preserves author order and performs no chain discovery. */
USTRUCT(BlueprintType)

struct FReEchoVfxScenarioLink
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scenario") TObjectPtr<AActor> Source = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scenario") TObjectPtr<AActor> Target = nullptr;
};

/** Editor-only target anchor that satisfies the production presentation target contract without gameplay state. */
UCLASS(Blueprintable)

class REECHO_API AReEchoVfxScenarioTargetActor : public AActor, public IReEchoCombatTarget
{
	GENERATED_BODY()

public:
	AReEchoVfxScenarioTargetActor();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scenario") FName TargetId = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scenario")
	EReEchoElement AttachedElement = EReEchoElement::None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scenario") bool bInsideRadius = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scenario") int32 ConductOrder = INDEX_NONE;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Scenario|Debug") FString ResolvedAssetPath;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Scenario|Debug") float LinkLengthCm = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Scenario|Debug")
	FVector LinkDirection = FVector::ZeroVector;

	virtual bool IsCombatTargetAlive() const override
	{
		return true;
	}

	virtual FVector GetCombatTargetLocation() const override
	{
		return GetActorLocation();
	}

	virtual int32 GetCombatTargetTieBreakIndex() const override
	{
		return ConductOrder;
	}

	virtual class UReEchoCombatantComponent* GetCombatTargetCombatant() const override
	{
		return nullptr;
	}

	virtual bool IntersectsCombatPath(const FVector&, const FVector&, float) const override
	{
		return false;
	}
};

/** Editor-only, disposable VFX harness. It reads production mappings but never publishes gameplay events. */
UCLASS(Blueprintable)

class REECHO_API AReEchoVfxPreviewActor : public AActor
{
	GENERATED_BODY()

public:
	AReEchoVfxPreviewActor();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
#if WITH_EDITOR
	virtual bool ShouldTickIfViewportsOnly() const override
	{
		return true;
	}
#endif

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Preview") TObjectPtr<USceneComponent> PreviewRoot;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Preview") TObjectPtr<USceneComponent> SourceAnchor;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Preview") TObjectPtr<USceneComponent> TargetAnchor;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Preview") TObjectPtr<UNiagaraComponent> PreviewEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview|Selection")
	EReEchoVfxPreviewMode Mode = EReEchoVfxPreviewMode::CombatSemantic;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview|Selection")
	TSoftObjectPtr<UNiagaraSystem> RawSystem;
	/** Numeric value of the production EReEchoCombatVfxSemantic. Displayed separately to avoid a duplicate enum. */
	UPROPERTY(EditAnywhere,
	          BlueprintReadWrite,
	          Category = "Preview|Selection",
	          meta = (ClampMin = "0", ClampMax = "12"))
	uint8 CombatSemantic = 0;
	/** Numeric value of the production EReEchoElementReactionVfxSemantic. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview|Selection", meta = (ClampMin = "0", ClampMax = "7"))
	uint8 ElementSemantic = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview|Selection")
	FName ElementTargetId = TEXT("Enemy.Slime");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview|Selection") FName WeaponVisualKey = TEXT("Bow");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview|Selection")
	EReEchoVfxPreviewWeaponSlot WeaponSlot = EReEchoVfxPreviewWeaponSlot::Travel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview|Sandbox") FTransform SandboxTransform;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview|Sandbox") float ProjectileDistanceCm = 600.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview|Sandbox") float ProjectileSpeedCmPerSecond = 500.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview|Sandbox") int32 SortPriority = 1000;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Preview|Read Only") FString ResolvedAssetPath;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Preview|Read Only")
	FVector AuthoredForwardAxis = FVector::ForwardVector;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Preview|Read Only")
	FString SupportStatus = TEXT("Missing");
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Preview|Read Only")
	FString AuthorityNotice = TEXT("PREVIEW ONLY - Sandbox values are not applied to production assets or gameplay.");
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Preview|Read Only") FString CalibrationReport;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scenario")
	EReEchoVfxScenarioPreset ScenarioPreset = EReEchoVfxScenarioPreset::SingleHit;
	/** Sandbox-only endpoint list. It is visible calibration data and never enters Production Simulation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scenario|Visual Calibration")
	TArray<FReEchoVfxScenarioLink> AuthoredReactionLinks;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scenario|Production Simulation")
	bool bRunProductionConductInPie = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scenario|Production Simulation")
	bool bAutoReleaseOnBeginPlay = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scenario|Production Simulation")
	TArray<FVector> ProductionTargetOffsets;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scenario|Production Simulation")
	FName ProductionWeaponId = NAME_None;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Scenario|Production Simulation")
	float ResolvedConductDelaySeconds = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scenario|Visual Calibration", meta = (ClampMin = "0.0"))
	float CalibrationConductDelayOverrideSeconds = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Scenario|Production Simulation")
	FReEchoElementReactionResolvedEvent ResolvedReactionEvent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Scenario|Production Simulation")
	int32 ReleaseCount = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Scenario|Production Simulation")
	FString ProductionStatus = TEXT("Ready");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scenario|Camera")
	FVector CameraPan = FVector(-900.0f, 0.0f, 900.0f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scenario|Camera")
	FRotator CameraRotation = FRotator(-45.0f, 0.0f, 0.0f);
	UPROPERTY(EditAnywhere,
	          BlueprintReadWrite,
	          Category = "Scenario|Camera",
	          meta = (ClampMin = "320.0", ClampMax = "10000.0"))
	float CameraOrthoWidth = 2560.0f;
	UPROPERTY(EditAnywhere,
	          BlueprintReadWrite,
	          Category = "Scenario|Camera",
	          meta = (ClampMin = "10.0", ClampMax = "5000.0"))
	float CameraMoveSpeed = 1200.0f;
	UPROPERTY(EditAnywhere,
	          BlueprintReadWrite,
	          Category = "Scenario|Camera",
	          meta = (ClampMin = "10.0", ClampMax = "5000.0"))
	float CameraZoomSpeed = 300.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scenario|Visual Calibration")
	float ScenarioRadiusCm = 500.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scenario|Visual Calibration")
	float PreviewStepSeconds = 1.0f / 30.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Scenario|Visual Calibration")
	bool bRadiusVisible = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Scenario|Visual Calibration")
	bool bDirectionVisible = true;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Scenario|Read Only")
	FString ScenarioAuthorityNotice =
	    TEXT("PRODUCTION SIMULATION READS RESOLVED EVENTS; VISUAL CALIBRATION IS TRANSIENT - NOT APPLIED");
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Scenario|Read Only") FString ScenarioReport;

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Preview") void PlayPreview();
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Preview") void StopPreview();
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Preview") void RestartPreview();
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Preview") void ClearPreview();
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Preview") void FaceTarget();
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Preview") void SwapSourceAndTarget();
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Preview") void FourDirections();
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Preview") void EightDirections();
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Preview") void ProjectilePreview();
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Preview") void GenerateCalibrationReport();
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Preview") void ResetSandbox();
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Scenario") void RunScenario();
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Scenario") void PauseScenario();
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Scenario") void StepScenario();
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Scenario") void RestartScenario();
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Scenario") void ResetScenario();
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Scenario") void AddScenarioTarget();
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Scenario") void RemoveScenarioTarget();
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Scenario") void ShowRadius();
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Scenario") void ShowDirection();
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Scenario") void GenerateScenarioReport();
	UFUNCTION(BlueprintCallable, Category = "Scenario|PIE") void ReleaseProductionVfx();
	UFUNCTION(BlueprintCallable, Category = "Scenario|PIE") void RestartProductionScenario();
	UFUNCTION(BlueprintCallable, Category = "Scenario|PIE") void ResetProductionTargets();
	UFUNCTION(BlueprintCallable, Category = "Scenario|PIE") void ClearProductionScenario();
	UFUNCTION(BlueprintCallable, Category = "Scenario|PIE") void ResetScenarioCamera();

	static float ClampScenarioOrthoWidth(float Value)
	{
		return FMath::Clamp(Value, 320.0f, 10000.0f);
	}

	static TArray<FReEchoElementReactionLink>
	ConsumeResolvedReactionLinks(const FReEchoElementReactionResolvedEvent& Event);
	static TArray<FReEchoElementReactionLink>
	ConvertAuthoredReactionLinks(const TArray<FReEchoVfxScenarioLink>& AuthoredLinks);
#if WITH_DEV_AUTOMATION_TESTS
	int32 GetCalibrationTargetCountForTests() const
	{
		return CalibrationTargets.Num();
	}
#endif

	static FVector ResolveSafeDirection(const FVector& Source, const FVector& Target);
	static FTransform ResolvePreviewTransform(const FVector& Source,
	                                          const FVector& Target,
	                                          const FVector& AuthoredAxis,
	                                          const FTransform& Sandbox);
	static FString ResolveProductionPath(EReEchoVfxPreviewMode PreviewMode,
	                                     uint8 CombatSemanticValue,
	                                     uint8 ElementSemanticValue,
	                                     FName TargetId,
	                                     FName VisualKey,
	                                     EReEchoVfxPreviewWeaponSlot Slot,
	                                     const TSoftObjectPtr<UNiagaraSystem>& RawNiagara);

private:
	void RunProductionConductPie();
	void InitializeProductionScenario();
	void CreateProductionControls();
	void RemoveProductionControls();
	void UpdateScenarioCamera(float DeltaSeconds);
	void DrawScenarioDebug() const;
	UFUNCTION() void CaptureProductionReaction(const FReEchoElementReactionResolvedEvent& Event);
	void RefreshResolvedSelection();
	void SpawnDirectionSet(int32 DirectionCount);
	UPROPERTY(Transient) TArray<TObjectPtr<UNiagaraComponent>> AdditionalPreviewEffects;
	UPROPERTY(Transient) TArray<TObjectPtr<AReEchoVfxScenarioTargetActor>> CalibrationTargets;
	UPROPERTY(Transient) TArray<TObjectPtr<AActor>> ProductionScenarioActors;
	UPROPERTY(Transient) TObjectPtr<AReEchoArenaCameraActor> ScenarioCamera;
	TSharedPtr<SWidget> ProductionControls;
};
