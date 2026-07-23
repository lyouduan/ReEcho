#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ReEchoTrajectoryActor.generated.h"

struct FReEchoRecording;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class USceneComponent;

UCLASS()
class REECHO_API AReEchoTrajectoryActor : public AActor
{
	GENERATED_BODY()

public:
	AReEchoTrajectoryActor();

	void InitializeTrajectory(const FReEchoRecording& Recording);

private:
	FVector ProjectToGround(const FVector& RecordedPosition) const;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UInstancedStaticMeshComponent> Segments;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> TrajectoryMaterial;
};