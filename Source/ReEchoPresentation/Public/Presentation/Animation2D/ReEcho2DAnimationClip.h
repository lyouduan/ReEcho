#pragma once

// Public API of the ReEchoPresentation runtime module.

#include "CoreMinimal.h"
#include "Presentation/Animation2D/ReEcho2DFrameCollisionTrack.h"
#include "ReEcho2DAnimationClip.generated.h"

class UPaperFlipbook;
/** Data-owned renderer and matching query-geometry policy for one semantic animation clip. */
USTRUCT(BlueprintType)

struct REECHOPRESENTATION_API FReEcho2DAnimationClip
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UPaperFlipbook> Flipbook;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UReEcho2DFrameCollisionTrack> CollisionTrack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bLooping = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bRestartOnRequest = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.01"))
	float PlayRate = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bUseNativeScale = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float WorldHeight = 224.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector LocalOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 TranslucentSortPriority = 10;

	bool IsValid() const;
};
