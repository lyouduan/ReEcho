#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ReEchoEnemyGameplayClassRegistry.generated.h"

class AReEchoEnemyActor;

USTRUCT(BlueprintType)
struct REECHO_API FReEchoEnemyGameplayClassEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName PresentationId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<AReEchoEnemyActor> GameplayClass;
};

/** Gameplay-owned mapping from stable presentation identity to an enemy host Blueprint class. */
UCLASS(BlueprintType)
class REECHO_API UReEchoEnemyGameplayClassRegistry : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FReEchoEnemyGameplayClassEntry> Entries;

	TSubclassOf<AReEchoEnemyActor> ResolveGameplayClass(FName PresentationId) const;
};
