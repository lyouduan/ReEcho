#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ReEchoHitImpactActor.generated.h"

class UStaticMeshComponent;
class USceneComponent;

/** 短生命周期的2D受击星芒，始终面向相机并在命中点快速弹出。 */
UCLASS()
class REECHO_API AReEchoHitImpactActor : public AActor
{
	GENERATED_BODY()

public:
	AReEchoHitImpactActor();
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> ImpactSprite;

	float ElapsedTime = 0.0f;
	float Lifetime = 0.2f;
};