#include "Graybox/ReEchoTrajectoryActor.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Core/ReEchoTypes.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace ReEchoTrajectory
{
constexpr float MinimumPointSpacing = 35.0f;
constexpr float SegmentWidth = 14.0f;
constexpr float SegmentThickness = 2.0f;
constexpr float GroundClearance = 2.5f;
constexpr float TraceDistance = 800.0f;
constexpr float FallbackGroundOffset = 65.0f;
constexpr float MeshSize = 100.0f;
}

AReEchoTrajectoryActor::AReEchoTrajectoryActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	Segments = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("TrajectorySegments"));
	Segments->SetupAttachment(SceneRoot);
	Segments->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Segments->SetCastShadow(false);
	Segments->SetReceivesDecals(false);
	Segments->SetTranslucentSortPriority(-1);
	Segments->SetStaticMesh(
		LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));

	if (UMaterialInterface* BaseMaterial =
		LoadObject<UMaterialInterface>(
			nullptr,
			TEXT("/Game/ReEcho/Materials/M_EchoGhost.M_EchoGhost")))
	{
		TrajectoryMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		TrajectoryMaterial->SetVectorParameterValue(
			TEXT("EchoColor"),
			FLinearColor(0.58f, 0.78f, 1.0f));
		TrajectoryMaterial->SetScalarParameterValue(TEXT("Opacity"), 0.18f);
		Segments->SetMaterial(0, TrajectoryMaterial);
	}
}

void AReEchoTrajectoryActor::InitializeTrajectory(const FReEchoRecording& Recording)
{
	Segments->ClearInstances();

	if (Recording.Positions.Num() < 2)
	{
		return;
	}

	TArray<FVector> SimplifiedPoints;
	SimplifiedPoints.Reserve(Recording.Positions.Num());
	SimplifiedPoints.Add(Recording.Positions[0].Position);

	for (int32 Index = 1; Index < Recording.Positions.Num() - 1; ++Index)
	{
		const FVector& Candidate = Recording.Positions[Index].Position;
		if (FVector::DistSquared2D(SimplifiedPoints.Last(), Candidate) >=
			FMath::Square(ReEchoTrajectory::MinimumPointSpacing))
		{
			SimplifiedPoints.Add(Candidate);
		}
	}

	const FVector& LastRecordedPoint = Recording.Positions.Last().Position;
	if (!SimplifiedPoints.Last().Equals(LastRecordedPoint))
	{
		SimplifiedPoints.Add(LastRecordedPoint);
	}

	FVector SegmentStart = ProjectToGround(SimplifiedPoints[0]);
	for (int32 Index = 1; Index < SimplifiedPoints.Num(); ++Index)
	{
		const FVector SegmentEnd = ProjectToGround(SimplifiedPoints[Index]);
		const FVector SegmentDelta = SegmentEnd - SegmentStart;
		const float SegmentLength = SegmentDelta.Size2D();

		if (SegmentLength > KINDA_SMALL_NUMBER)
		{
			const FVector Midpoint = (SegmentStart + SegmentEnd) * 0.5f;
			const FRotator Rotation = SegmentDelta.GetSafeNormal2D().Rotation();
			const FVector Scale(
				SegmentLength / ReEchoTrajectory::MeshSize,
				ReEchoTrajectory::SegmentWidth / ReEchoTrajectory::MeshSize,
				ReEchoTrajectory::SegmentThickness / ReEchoTrajectory::MeshSize);

			Segments->AddInstance(FTransform(Rotation, Midpoint, Scale), true);
		}

		SegmentStart = SegmentEnd;
	}
}

FVector AReEchoTrajectoryActor::ProjectToGround(const FVector& RecordedPosition) const
{
	const FVector TraceStart =
		RecordedPosition + FVector::UpVector * ReEchoTrajectory::TraceDistance;
	const FVector TraceEnd =
		RecordedPosition - FVector::UpVector * ReEchoTrajectory::TraceDistance;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ReEchoTrajectoryGround), false);
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(GetOwner());

	FHitResult HitResult;
	if (GetWorld()->LineTraceSingleByChannel(
		HitResult,
		TraceStart,
		TraceEnd,
		ECC_Visibility,
		QueryParams))
	{
		return HitResult.ImpactPoint + FVector::UpVector * ReEchoTrajectory::GroundClearance;
	}

	return RecordedPosition -
		FVector::UpVector * ReEchoTrajectory::FallbackGroundOffset;
}