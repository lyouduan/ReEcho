#include "Presentation/Scene/ReEchoArenaSceneSpawnAnchor.h"

#include "Components/SceneComponent.h"

AReEchoArenaSceneSpawnAnchor::AReEchoArenaSceneSpawnAnchor()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}
