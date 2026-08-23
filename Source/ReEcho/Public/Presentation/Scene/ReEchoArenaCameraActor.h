#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ReEchoArenaCameraActor.generated.h"

class AReEchoArenaSceneActor;
class AReEchoPlayerPawn;
class UCameraComponent;
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

private:
	void UpdateFollow(float DeltaSeconds);
	FVector GetGroundFocus() const;
	void SetGroundFocus(const FVector2D& Focus);

	UPROPERTY(Transient)
	TObjectPtr<AReEchoPlayerPawn> FollowTarget;
	UPROPERTY(Transient)
	TObjectPtr<AReEchoArenaSceneActor> ArenaSource;
};
