#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ReEchoPlayerController.generated.h"

/** Owns the gameplay camera explicitly; possession must not replace the Arena view target. */
UCLASS()
class REECHO_API AReEchoPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AReEchoPlayerController();
};
