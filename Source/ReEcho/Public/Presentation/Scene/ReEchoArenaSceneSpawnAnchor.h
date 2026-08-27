#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ReEchoArenaSceneSpawnAnchor.generated.h"

class USceneComponent;

/** Persistent-Level transform where complete Arena Blueprint instances are prepared and committed. */
UCLASS()

class REECHO_API AReEchoArenaSceneSpawnAnchor : public AActor
{
	GENERATED_BODY()

public:
	AReEchoArenaSceneSpawnAnchor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena")
	TObjectPtr<USceneComponent> SceneRoot;
};
