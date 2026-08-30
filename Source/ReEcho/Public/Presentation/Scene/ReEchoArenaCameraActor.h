#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ReEchoArenaCameraActor.generated.h"

class AReEchoArenaSceneActor;
class AReEchoPlayerPawn;
class AActor;
class UCameraComponent;
class UCurveFloat;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class USceneComponent;

/** Independent Editor-authored camera. It never inherits the map Actor transform. */
UCLASS()

class REECHO_API AReEchoArenaCameraActor : public AActor
{
	GENERATED_BODY()

public:
	AReEchoArenaCameraActor();
	virtual void Tick(float DeltaSeconds) override;

	void Configure(AReEchoPlayerPawn* InFollowTarget, AReEchoArenaSceneActor* InArenaSource);
	static float CalculateEncounterCountdownPostProcessIntensity(float RemainingTime);
	void SetEncounterCountdownPostProcessIntensity(float Intensity);
	void ResetEncounterCountdownPostProcess();
	static float CalculateStage01To02CameraEaseAlpha(float LinearAlpha);
	static FVector2D CalculateStage01To02GroundFocus(const FVector& VisualCenterWorld,
	                                                 float GameplayPlaneWorldZ,
	                                                 const FVector& CameraForward);
	static bool TryResolveStage01To02VisualCenter(AActor* Target, FVector& OutVisualCenterWorld);
	void BeginStage01To02CameraSequence();
	bool FocusStage01To02Target(AActor* Target, float OrthoWidthRatio, float DurationSeconds);
	bool FocusStage01To02TargetAtStandardWidth(AActor* Target, float DurationSeconds);
	bool IsStage01To02CameraMoveComplete() const;
	void EndStage01To02CameraSequence();
	void CancelStage01To02CameraSequence();

	float GetStage01To02PlayerFocusDuration() const
	{
		return Stage01To02PlayerFocusDuration;
	}

	float GetStage01To02PlayerFocusRatio() const
	{
		return Stage01To02PlayerFocusOrthoWidthRatio;
	}

	float GetStage01To02PlayerHoldDuration() const
	{
		return Stage01To02PlayerHoldDuration;
	}

	float GetStage01To02EchoZoomOutDuration() const
	{
		return Stage01To02EchoZoomOutDuration;
	}

	float GetStage01To02EchoFocusRatio() const
	{
		return Stage01To02EchoFocusOrthoWidthRatio;
	}

	float GetStage01To02MoveToPlayerDuration() const
	{
		return Stage01To02MoveToPlayerDuration;
	}

	bool ShouldStage01To02AllowExactCenter() const
	{
		return bStage01To02AllowExactCenter;
	}

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena Camera")
	TObjectPtr<USceneComponent> CameraRoot;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena Camera")
	TObjectPtr<UCameraComponent> ArenaCamera;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena Camera|Follow")
	bool bLockPlayerToCameraCenter = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena Camera|Follow")
	bool bSmoothCameraFollow = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena Camera|Follow", meta = (ClampMin = "0.0"))
	float CameraFollowSpeed = 8.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena Camera|Follow")
	bool bClampToArenaBounds = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena Camera|Edge Limits", meta = (ClampMin = "0.0"))
	float LeftEdgeInset = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena Camera|Edge Limits", meta = (ClampMin = "0.0"))
	float RightEdgeInset = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena Camera|Edge Limits", meta = (ClampMin = "0.0"))
	float BottomEdgeInset = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena Camera|Edge Limits", meta = (ClampMin = "0.0"))
	float TopEdgeInset = 0.0f;
	UPROPERTY(EditAnywhere,
	          BlueprintReadOnly,
	          Category = "Arena Camera|Encounter Countdown Post Process",
	          meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CountdownClearCenterRadius = 0.28f;
	UPROPERTY(EditAnywhere,
	          BlueprintReadOnly,
	          Category = "Arena Camera|Encounter Countdown Post Process",
	          meta = (ClampMin = "0.0", ClampMax = "1.5"))
	float CountdownEdgeBlurRadius = 0.62f;
	UPROPERTY(EditAnywhere,
	          BlueprintReadOnly,
	          Category = "Arena Camera|Encounter Countdown Post Process",
	          meta = (ClampMin = "0.1", ClampMax = "4.0"))
	float CountdownEdgeMaskPower = 1.4f;
	UPROPERTY(EditAnywhere,
	          BlueprintReadOnly,
	          Category = "Arena Camera|Encounter Countdown Post Process",
	          meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CountdownGhostStrength = 0.8f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena Camera|Encounter Countdown Post Process")
	bool bCountdownEnableRGBSeparation = true;
	UPROPERTY(EditAnywhere,
	          BlueprintReadOnly,
	          Category = "Arena Camera|Encounter Countdown Post Process",
	          meta = (EditCondition = "bCountdownEnableRGBSeparation",
	                  EditConditionHides,
	                  ClampMin = "0.0",
	                  ClampMax = "1.0"))
	float CountdownRGBSeparationStrength = 1.0f;
	UPROPERTY(EditAnywhere,
	          BlueprintReadOnly,
	          Category = "Arena Camera|Encounter Transition|Stage01To02",
	          meta = (ClampMin = "0.0"))
	float Stage01To02PlayerFocusDuration = 1.0f;
	UPROPERTY(EditAnywhere,
	          BlueprintReadOnly,
	          Category = "Arena Camera|Encounter Transition|Stage01To02",
	          meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float Stage01To02PlayerFocusOrthoWidthRatio = 0.325f;
	UPROPERTY(EditAnywhere,
	          BlueprintReadOnly,
	          Category = "Arena Camera|Encounter Transition|Stage01To02",
	          meta = (ClampMin = "0.0"))
	float Stage01To02PlayerHoldDuration = 0.5f;
	UPROPERTY(EditAnywhere,
	          BlueprintReadOnly,
	          Category = "Arena Camera|Encounter Transition|Stage01To02",
	          meta = (ClampMin = "0.0"))
	float Stage01To02EchoZoomOutDuration = 1.0f;
	UPROPERTY(EditAnywhere,
	          BlueprintReadOnly,
	          Category = "Arena Camera|Encounter Transition|Stage01To02",
	          meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float Stage01To02EchoFocusOrthoWidthRatio = 0.325f;
	UPROPERTY(EditAnywhere,
	          BlueprintReadOnly,
	          Category = "Arena Camera|Encounter Transition|Stage01To02",
	          meta = (ClampMin = "0.0"))
	float Stage01To02MoveToPlayerDuration = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena Camera|Encounter Transition|Stage01To02")
	TObjectPtr<UCurveFloat> Stage01To02CameraEaseCurve;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena Camera|Encounter Transition|Stage01To02")
	bool bStage01To02AllowExactCenter = true;

private:
	void EnsureEncounterCountdownPostProcess();
	void UpdateEncounterCountdownPostProcessParameters();
	void UpdateFollow(float DeltaSeconds);
	void UpdateStage01To02CameraMove(float DeltaSeconds);
	FVector2D ResolveTransitionTargetFocus() const;
	FVector GetGroundFocus() const;
	void SetGroundFocus(const FVector2D& Focus);

	UPROPERTY(Transient)
	TObjectPtr<AReEchoPlayerPawn> FollowTarget;
	UPROPERTY(Transient)
	TObjectPtr<AReEchoArenaSceneActor> ArenaSource;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> EncounterCountdownPostProcessMaterial;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> EncounterCountdownPostProcessMID;
	float EncounterCountdownPostProcessIntensity = 0.0f;
	float EncounterCountdownPostProcessPhase = 0.0f;
	bool bEncounterCountdownPostProcessBound = false;
	UPROPERTY(Transient)
	TObjectPtr<AActor> Stage01To02CameraTarget;
	FVector2D Stage01To02MoveStartFocus = FVector2D::ZeroVector;
	FVector2D Stage01To02MoveFallbackTargetFocus = FVector2D::ZeroVector;
	float Stage01To02SequenceStandardOrthoWidth = 0.0f;
	float Stage01To02MoveStartOrthoWidth = 0.0f;
	float Stage01To02MoveTargetOrthoWidth = 0.0f;
	float Stage01To02MoveElapsedSeconds = 0.0f;
	float Stage01To02MoveDurationSeconds = 0.0f;
	bool bStage01To02CameraSequenceActive = false;
	bool bStage01To02CameraMoveActive = false;
};
