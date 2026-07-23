#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ReEchoSwordArcActor.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class USceneComponent;

UCLASS()
class REECHO_API AReEchoSwordArcActor : public AActor
{
	GENERATED_BODY()

public:
	AReEchoSwordArcActor();

	virtual void Tick(float DeltaSeconds) override;

	void InitializeArc(float SwingDirection);

private:
	void BuildArc(
		UInstancedStaticMeshComponent* SegmentComponent,
		float Radius,
		float Width,
		float SwingDirection);
	void UpdateOpacity(float Opacity);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UInstancedStaticMeshComponent> GlowSegments;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UInstancedStaticMeshComponent> CoreSegments;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> GlowMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> CoreMaterial;

	float ElapsedTime = 0.0f;
	float Lifetime = 0.30f;
};