#include "Presentation/Animation2D/ReEcho2DFrameCollisionTrack.h"

#include "PaperFlipbook.h"

namespace
{
constexpr int32 MinimumPolygonVertices = 3;
constexpr int32 MaximumPolygonVertices = 16;

bool ValidatePolygons(const TArray<FReEcho2DCollisionPolygon>& Polygons, FString& OutError)
{
	for (const FReEcho2DCollisionPolygon& Polygon : Polygons)
	{
		if (Polygon.Vertices.Num() < MinimumPolygonVertices || Polygon.Vertices.Num() > MaximumPolygonVertices)
		{
			OutError = FString::Printf(
			    TEXT("Collision polygon must contain %d-%d vertices"), MinimumPolygonVertices, MaximumPolygonVertices);
			return false;
		}
		float TwiceSignedArea = 0.0f;
		for (int32 Index = 0; Index < Polygon.Vertices.Num(); ++Index)
		{
			const FVector2D A = Polygon.Vertices[Index];
			const FVector2D B = Polygon.Vertices[(Index + 1) % Polygon.Vertices.Num()];
			if (!FMath::IsFinite(A.X) || !FMath::IsFinite(A.Y))
			{
				OutError = TEXT("Collision polygon contains a non-finite vertex");
				return false;
			}
			TwiceSignedArea += A.X * B.Y - B.X * A.Y;
		}
		if (FMath::Abs(TwiceSignedArea) <= KINDA_SMALL_NUMBER)
		{
			OutError = TEXT("Collision polygon area must be non-zero");
			return false;
		}
	}
	return true;
}
}

bool UReEcho2DFrameCollisionTrack::ValidateForFlipbook(const UPaperFlipbook* Flipbook, FString& OutError) const
{
	OutError.Reset();
	if (!Flipbook || SourceFlipbook != Flipbook)
	{
		OutError = TEXT("Collision track source Flipbook does not match the requested clip");
		return false;
	}
	if (SourceRevision.IsEmpty())
	{
		OutError = TEXT("Collision track source revision is empty");
		return false;
	}
	if (PixelsPerUnrealUnit <= 0.0f)
	{
		OutError = TEXT("Collision track PixelsPerUnrealUnit must be positive");
		return false;
	}
	if (Frames.Num() != Flipbook->GetNumFrames())
	{
		OutError = FString::Printf(TEXT("Collision track frame count %d does not match Flipbook frame count %d"),
		                           Frames.Num(),
		                           Flipbook->GetNumFrames());
		return false;
	}
	for (const FReEcho2DFrameCollision& Frame : Frames)
	{
		if (!ValidatePolygons(Frame.BodyHurtboxes, OutError) || !ValidatePolygons(Frame.WeaponAttackHitboxes, OutError))
		{
			return false;
		}
	}
	return true;
}
