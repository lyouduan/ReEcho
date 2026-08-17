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
	FVector2D GetArenaCenter() const;
	bool HasValidConfiguration(FString* OutReason = nullptr) const;

	static FVector2D
	CalculateGroundFootprintHalfExtents(float OrthoWidth, float AspectRatio, const FRotator& CameraRotation);
	static FVector2D ClampCameraFocus(const FVector2D& DesiredFocus,
	                                  const FVector2D& MapCenter,
	                                  const FVector2D& MapHalfExtents,
	                                  const FVector2D& FootprintHalfExtents);
	static int32 CalculateFootpointSortPriority(const FVector2D& WorldFootpoint,
	                                            const FVector2D& WorldOrigin,
	                                            const FVector2D& SortAxis,
	                                            float WorldUnitsPerStep,
	                                            int32 BasePriority,
	                                            const FIntPoint& PriorityRange);
	int32 CalculateFootpointSortPriority(const FVector& WorldFootpoint) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena")
	TObjectPtr<USceneComponent> SceneRoot;
	/** Editor 中移动或缩放此节点只影响地图、碰撞和玩法范围，不影响相机 Transform。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena|Hierarchy")
	TObjectPtr<USceneComponent> MapRoot;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena|Hierarchy")
	TObjectPtr<USceneComponent> VisualRoot;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena|Hierarchy")
	TObjectPtr<USceneComponent> GameplayRoot;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena|Hierarchy|Visual")
	TObjectPtr<USceneComponent> GroundRoot;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena|Hierarchy|Visual")
	TObjectPtr<USceneComponent> GroundDetailRoot;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena|Hierarchy|Visual")
	TObjectPtr<USceneComponent> MidDecorationRoot;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena|Hierarchy|Visual")
	TObjectPtr<USceneComponent> ForegroundRoot;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena|Hierarchy|Visual")
	TObjectPtr<USceneComponent> AtmosphereRoot;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena|Hierarchy|Visual")
	TObjectPtr<USceneComponent> SceneEffectsRoot;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena|Hierarchy|Gameplay")
	TObjectPtr<USceneComponent> CollisionRoot;
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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Camera")
	bool bSmoothCameraFollow = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Camera", meta = (ClampMin = "0.0"))
	float CameraFollowSpeed = 8.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Camera")
	float GameplayPlaneZ = 0.0f;
	/** 关闭后 Backdrop 的位置、旋转和缩放完全采用 Editor 组件 Transform。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Layout")
	bool bAutoLayoutBackdrop = true;
	/** 关闭后 Floor 与四面墙的位置、旋转和缩放完全采用 Editor 组件 Transform。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Layout")
	bool bAutoLayoutCollision = true;

	/** 表现 Actor 以脚点消费该值；场景不直接持有或驱动 Flipbook。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Visual|DepthSort")
	FVector2D DepthSortAxis = FVector2D(-1.0f, 0.0f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Visual|DepthSort", meta = (ClampMin = "1.0"))
	float DepthSortWorldUnitsPerStep = 10.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Visual|DepthSort")
	int32 DepthSortBasePriority = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Visual|DepthSort")
	FIntPoint DepthSortPriorityRange = FIntPoint(-50, 50);

	/** 角色表现可选消费的默认接触阴影参数；不参与玩法碰撞。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Visual|ContactShadow")
	TSoftObjectPtr<UTexture2D> DefaultContactShadowTexture;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Visual|ContactShadow", meta = (ClampMin = "0.0"))
	FVector2D DefaultContactShadowSize = FVector2D(90.0f, 45.0f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Visual|ContactShadow", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DefaultContactShadowOpacity = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Visual|Parallax")
	bool bEnableParallax = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Visual|Parallax", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float MidDecorationParallaxFactor = 0.04f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Visual|Parallax", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float ForegroundParallaxFactor = 0.08f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Visual|Parallax", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float AtmosphereParallaxFactor = 0.02f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Visual|Parallax", meta = (ClampMin = "0.0"))
	float MaximumParallaxOffset = 120.0f;

private:
	FVector2D GetMapScale2D() const;
	void UpdateEditorLayout();
	void UpdateFollowCamera(float DeltaSeconds);
	void UpdateParallax();
	FVector GetCameraGroundFocus() const;
	void SetCameraGroundFocus(const FVector2D& Focus);

	UPROPERTY(Transient)
	TObjectPtr<AReEchoPlayerPawn> FollowTarget;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> BackdropMaterial;
	bool bLoggedUndersizedMap = false;
};
