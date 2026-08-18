#pragma once

// Public API of the ReEchoPresentation runtime module.

#include "CoreMinimal.h"
#include "ReEcho2DAnimationTypes.generated.h"

UENUM(BlueprintType)
enum class EReEcho2DAnimationState : uint8
{
	Default,
	Idle,
	Walk,
	Attack,
	Hit,
	Death
};

/** Resolve the presentation state with Attack taking priority over locomotion. */
REECHOPRESENTATION_API EReEcho2DAnimationState ReEchoResolve2DAnimationState(bool bMoving, bool bAttacking);

UENUM(BlueprintType)
enum class EReEcho2DAnimationActivationResult : uint8
{
	Activated,
	Disabled,
	MissingProfile,
	MissingFlipbook
};
