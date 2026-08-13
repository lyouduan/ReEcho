#include "Presentation/Animation2D/ReEcho2DFrameCollisionDriver.h"

#include "Presentation/Animation2D/ReEcho2DAnimationComponent.h"

void FReEcho2DFrameCollisionSnapshot::Reset()
{
	BodyHurtboxes.Reset();
	WeaponAttackHitboxes.Reset();
	FrameIndex = INDEX_NONE;
	AttackInstanceId = INDEX_NONE;
	bAttackActive = false;
}

UReEcho2DFrameCollisionDriver::UReEcho2DFrameCollisionDriver()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UReEcho2DFrameCollisionDriver::TickComponent(const float DeltaTime, const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	RefreshSnapshot();
}

void UReEcho2DFrameCollisionDriver::BindRenderer(UReEcho2DAnimationComponent* InRenderer)
{
	Renderer = InRenderer;
	ValidatedTrack.Reset();
	RefreshSnapshot();
}

void UReEcho2DFrameCollisionDriver::BeginAttackInstance(const int64 InAttackInstanceId)
{
	CommittedAttackInstanceId = InAttackInstanceId >= 0 ? InAttackInstanceId : INDEX_NONE;
	RefreshSnapshot();
}

void UReEcho2DFrameCollisionDriver::EndAttackInstance(const int64 InAttackInstanceId)
{
	if (CommittedAttackInstanceId == InAttackInstanceId)
	{
		CommittedAttackInstanceId = INDEX_NONE;
		RefreshSnapshot();
	}
}

bool UReEcho2DFrameCollisionDriver::IsLocalPointInsideBody(const FVector2D LocalPoint) const
{
	return Snapshot.BodyHurtboxes.ContainsByPredicate(
	    [LocalPoint](const FReEcho2DCollisionPolygon& Polygon) { return ContainsPoint(Polygon, LocalPoint); });
}

bool UReEcho2DFrameCollisionDriver::IsLocalPointInsideActiveAttack(const FVector2D LocalPoint,
	const int64 AttackInstanceId) const
{
	return Snapshot.bAttackActive && Snapshot.AttackInstanceId == AttackInstanceId &&
	       Snapshot.WeaponAttackHitboxes.ContainsByPredicate(
	           [LocalPoint](const FReEcho2DCollisionPolygon& Polygon) { return ContainsPoint(Polygon, LocalPoint); });
}

void UReEcho2DFrameCollisionDriver::RefreshSnapshot()
{
	Snapshot.Reset();
	if (!Renderer || !Renderer->IsAnimationActive())
	{
		Invalidate(TEXT("Animation renderer is inactive"));
		return;
	}

	const FReEcho2DAnimationClip& Clip = Renderer->GetActiveClip();
	const UReEcho2DFrameCollisionTrack* Track = Clip.CollisionTrack;
	if (!Track)
	{
		Invalidate(TEXT("Active clip has no collision track"));
		return;
	}
	if (ValidatedTrack.Get() != Track)
	{
		FString Error;
		if (!Track->ValidateForFlipbook(Clip.Flipbook, Error))
		{
			Invalidate(MoveTemp(Error));
			return;
		}
		ValidatedTrack = Track;
	}

	const int32 FrameIndex = Renderer->GetCurrentKeyFrameIndex();
	if (!Track->Frames.IsValidIndex(FrameIndex))
	{
		Invalidate(TEXT("Renderer frame is outside the collision track"));
		return;
	}

	bTrackValid = true;
	FallbackReason.Reset();
	Snapshot.FrameIndex = FrameIndex;
	const FReEcho2DFrameCollision& Frame = Track->Frames[FrameIndex];
	for (const FReEcho2DCollisionPolygon& Polygon : Frame.BodyHurtboxes)
	{
		Snapshot.BodyHurtboxes.Add(TransformPolygon(Polygon, *Track, Renderer->GetFacingSign()));
	}
	Snapshot.bAttackActive = Frame.bAttackActive && CommittedAttackInstanceId >= 0;
	if (Snapshot.bAttackActive)
	{
		Snapshot.AttackInstanceId = CommittedAttackInstanceId;
		for (const FReEcho2DCollisionPolygon& Polygon : Frame.WeaponAttackHitboxes)
		{
			Snapshot.WeaponAttackHitboxes.Add(TransformPolygon(Polygon, *Track, Renderer->GetFacingSign()));
		}
	}
}

FReEcho2DCollisionPolygon UReEcho2DFrameCollisionDriver::TransformPolygon(
	const FReEcho2DCollisionPolygon& Source, const UReEcho2DFrameCollisionTrack& Track, const float FacingSign)
{
	FReEcho2DCollisionPolygon Result;
	Result.Vertices.Reserve(Source.Vertices.Num());
	const float Mirror = FacingSign < 0.0f ? -1.0f : 1.0f;
	for (const FVector2D Vertex : Source.Vertices)
	{
		const FVector2D Centered = (Vertex - Track.PivotPixels) / Track.PixelsPerUnrealUnit;
		Result.Vertices.Add(FVector2D(Centered.X * Mirror, Centered.Y));
	}
	return Result;
}

bool UReEcho2DFrameCollisionDriver::ContainsPoint(const FReEcho2DCollisionPolygon& Polygon, const FVector2D Point)
{
	bool bInside = false;
	for (int32 Index = 0, Previous = Polygon.Vertices.Num() - 1; Index < Polygon.Vertices.Num(); Previous = Index++)
	{
		const FVector2D A = Polygon.Vertices[Index];
		const FVector2D B = Polygon.Vertices[Previous];
		const bool bCrosses = (A.Y > Point.Y) != (B.Y > Point.Y);
		if (bCrosses && Point.X < (B.X - A.X) * (Point.Y - A.Y) / (B.Y - A.Y) + A.X)
		{
			bInside = !bInside;
		}
	}
	return bInside;
}

void UReEcho2DFrameCollisionDriver::Invalidate(FString Reason)
{
	bTrackValid = false;
	FallbackReason = MoveTemp(Reason);
	ValidatedTrack.Reset();
}
