#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "ReEcho2DSceneLightingComponent.generated.h"

class AReEchoArenaSceneActor;
class UPaperFlipbookComponent;
class UPrimitiveComponent;

/** Presentation-only adapter that applies deterministic footpoint sorting to the Flipbook renderer. */
UCLASS(ClassGroup = (ReEcho), meta = (BlueprintSpawnableComponent))

class REECHO_API UReEcho2DSceneLightingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UReEcho2DSceneLightingComponent();
	virtual void BeginPlay() override;
	virtual void
	TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void Configure(UPaperFlipbookComponent* InAnimationRenderer, UPrimitiveComponent* InGroundShadow = nullptr);

private:
	void ResolveArenaScene();
	void ApplyFootpointSorting();

	TObjectPtr<UPaperFlipbookComponent> AnimationRenderer;
	TObjectPtr<UPrimitiveComponent> GroundShadow;
	UPROPERTY(Transient)
	TObjectPtr<AReEchoArenaSceneActor> ArenaScene;
};
