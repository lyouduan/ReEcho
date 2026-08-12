#pragma once

#include "CoreMinimal.h"
#include "ReEcho2DAnimationTypes.generated.h"

UENUM(BlueprintType)
enum class EReEcho2DAnimationState : uint8
{
	Default,
	Idle,
	Move,
	Attack,
	Hit,
	Death
};

UENUM(BlueprintType)
enum class EReEcho2DAnimationActivationResult : uint8
{
	Activated,
	Disabled,
	MissingProfile,
	MissingFlipbook
};
