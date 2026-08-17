#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ReEchoArenaSceneActor.generated.h"

class AReEchoPlayerPawn;
class UBoxComponent;
class UCameraComponent;
class UMaterialInterface;
class UStaticMeshComponent;
class UTexture2D;

/** Editor-authored first-arena scene and camera contract. Gameplay consumes its bounds but never owns its layout. */
UCLASS()

class REECHO_API AReEchoArenaSceneActor : public AActor
{
	GENERATED_BODY()

public:
	AReEchoArenaSceneActor();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

	void SetFollowTarget(AReEchoPlayerPawn* Target);

	UCameraComponent* GetArenaCamera() const
	{
		return ArenaCamera;
	}

	FVector2D GetPlayerHalfExtents() const;
	FVector2D GetEnemySpawnHalfExtents() const;
	bool HasValidConfiguration(FString* OutReason = nullptr) const;

	static FVector2D
	CalculateGroundFootprintHalfExtents(float OrthoWidth, float AspectRatio, const FRotator& CameraRotation);
	static FVector2D ClampCameraFocus(const FVector2D& DesiredFocus,
	                                  const FVector2D& MapCenter,
	                                  const FVector2D& MapHalfExtents,
	                                  const FVector2D& FootprintHalfExtents);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena")
	TObjectPtr<USceneComponent> SceneRoot;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena|Camera")
	TObjectPtr<UCameraComponent> ArenaCamera;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena|Visual")
	TObjectPtr<UStaticMeshComponent> Backdrop;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena|Collision")
	TObjectPtr<UStaticMeshComponent> Floor;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena|Collision")
	TObjectPtr<UStaticMeshComponent> WallNorth;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena|Collision")
	TObjectPtr<UStaticMeshComponent> WallSouth;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena|Collision")
	TObjectPtr<UStaticMeshComponent> WallEast;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena|Collision")
	TObjectPtr<UStaticMeshComponent> WallWest;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena|Bounds")
	TObjectPtr<UBoxComponent> CameraClampBounds;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena|Bounds")
	TObjectPtr<UBoxComponent> PlayerBounds;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena|Bounds")
	TObjectPtr<UBoxComponent> EnemySpawnBounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Visual")
	TObjectPtr<UTexture2D> MapTexture;
	/** 独立的背景、相机安全区、玩家活动区和敌人出生区；X/Y 对应世界 X/Y。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Bounds", meta = (ClampMin = "100.0"))
	FVector2D BackdropHalfExtents = FVector2D(1250.0f, 2240.0f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Bounds", meta = (ClampMin = "100.0"))
	FVector2D CameraClampHalfExtents = FVector2D(1250.0f, 2240.0f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Bounds", meta = (ClampMin = "100.0"))
	FVector2D PlayerHalfExtents = FVector2D(1200.0f, 2190.0f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Bounds", meta = (ClampMin = "100.0"))
	FVector2D EnemySpawnHalfExtents = FVector2D(1100.0f, 2090.0f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Camera", meta = (ClampMin = "100.0"))
	float CameraOrthoWidth = 2800.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Camera", meta = (ClampMin = "0.1"))
	float CameraAspectRatio = 1376.0f / 768.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Camera")
	bool bSmoothCameraFollow = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Camera", meta = (ClampMin = "0.0"))
	float CameraFollowSpeed = 8.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Camera")
	float GameplayPlaneZ = 0.0f;

private:
	void UpdateEditorLayout();
	void UpdateFollowCamera(float DeltaSeconds);
	FVector GetCameraGroundFocus() const;
	void SetCameraGroundFocus(const FVector2D& Focus);

	UPROPERTY(Transient)
	TObjectPtr<AReEchoPlayerPawn> FollowTarget;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> BackdropMaterial;
	bool bLoggedUndersizedMap = false;
};
