#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ReEchoArenaCameraActor.generated.h"

class AReEchoArenaSceneActor;
class AReEchoPlayerPawn;
class UCameraComponent;
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

private:
	void EnsureEncounterCountdownPostProcess();
	void UpdateEncounterCountdownPostProcessParameters();
	void UpdateFollow(float DeltaSeconds);
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
};
