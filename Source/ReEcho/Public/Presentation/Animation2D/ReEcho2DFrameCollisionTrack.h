#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ReEcho2DFrameCollisionTrack.generated.h"

class UPaperFlipbook;

USTRUCT(BlueprintType)

struct REECHO_API FReEcho2DCollisionPolygon
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FVector2D> Vertices;
};

USTRUCT(BlueprintType)

struct REECHO_API FReEcho2DFrameCollision
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FReEcho2DCollisionPolygon> BodyHurtboxes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FReEcho2DCollisionPolygon> WeaponAttackHitboxes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bAttackActive = false;
};

/** Pre-authored Query-only body/weapon geometry paired with one source Flipbook revision. */
UCLASS(BlueprintType)

class REECHO_API UReEcho2DFrameCollisionTrack : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UPaperFlipbook> SourceFlipbook;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString SourceRevision;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.01"))
	float PixelsPerUnrealUnit = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector2D PivotPixels = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FReEcho2DFrameCollision> Frames;

	bool ValidateForFlipbook(const UPaperFlipbook* Flipbook, FString& OutError) const;
};
