#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ReEchoArenaSceneCatalog.generated.h"

class AReEchoArenaSceneActor;

USTRUCT(BlueprintType)

struct FReEchoArenaSceneRegistration
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arena|Scene Switching")
	FName SceneId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arena|Scene Switching")
	TSubclassOf<AReEchoArenaSceneActor> ArenaClass;
};

/** Single runtime authority that resolves Stage-authored SceneIds to complete Arena Blueprint classes. */
UCLASS(BlueprintType)

class REECHO_API UReEchoArenaSceneCatalog : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena")
	TArray<FReEchoArenaSceneRegistration> Scenes;

	bool BuildRegistry(TMap<FName, TSubclassOf<AReEchoArenaSceneActor>>& OutRegistry, FString& OutError) const;

	static bool BuildRegistry(const TArray<FReEchoArenaSceneRegistration>& Registrations,
	                          TMap<FName, TSubclassOf<AReEchoArenaSceneActor>>& OutRegistry,
	                          FString& OutError);
};
