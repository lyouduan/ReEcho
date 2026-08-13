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
