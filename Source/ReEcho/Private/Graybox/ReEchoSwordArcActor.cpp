#include "Graybox/ReEchoSwordArcActor.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace ReEchoSwordArc
{
constexpr int32 SegmentCount = 16;
constexpr float StartAngleDegrees = -62.0f;
constexpr float EndAngleDegrees = 62.0f;
constexpr float GlowRadius = 148.0f;
constexpr float CoreRadius = 152.0f;
constexpr float GlowWidth = 30.0f;
constexpr float CoreWidth = 11.0f;
constexpr float SegmentHeight = 3.0f;
constexpr float MeshSize = 100.0f;
}

AReEchoSwordArcActor::AReEchoSwordArcActor()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(
		nullptr,
		TEXT("/Engine/BasicShapes/Cube.Cube"));

	GlowSegments = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("GlowSegments"));
	GlowSegments->SetupAttachment(SceneRoot);
	GlowSegments->SetStaticMesh(CubeMesh);
	GlowSegments->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GlowSegments->SetCastShadow(false);
	GlowSegments->SetReceivesDecals(false);
	GlowSegments->SetTranslucentSortPriority(3);

	CoreSegments = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("CoreSegments"));
	CoreSegments->SetupAttachment(SceneRoot);
	CoreSegments->SetStaticMesh(CubeMesh);
	CoreSegments->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CoreSegments->SetCastShadow(false);
	CoreSegments->SetReceivesDecals(false);
	CoreSegments->SetTranslucentSortPriority(4);

	UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(
		nullptr,
		TEXT("/Game/ReEcho/Materials/M_EchoGhost.M_EchoGhost"));
	if (BaseMaterial)
	{
		GlowMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		GlowMaterial->SetVectorParameterValue(
			TEXT("EchoColor"),
			FLinearColor(1.0f, 0.55f, 0.12f));
		GlowSegments->SetMaterial(0, GlowMaterial);

		CoreMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		CoreMaterial->SetVectorParameterValue(
			TEXT("EchoColor"),
			FLinearColor(1.0f, 0.95f, 0.72f));
		CoreSegments->SetMaterial(0, CoreMaterial);
	}

	SetActorEnableCollision(false);
}

void AReEchoSwordArcActor::InitializeArc(const float SwingDirection)
{
	const float Direction = SwingDirection >= 0.0f ? 1.0f : -1.0f;
	BuildArc(
		GlowSegments,
		ReEchoSwordArc::GlowRadius,
		ReEchoSwordArc::GlowWidth,
		Direction);
	BuildArc(
		CoreSegments,
		ReEchoSwordArc::CoreRadius,
		ReEchoSwordArc::CoreWidth,
		Direction);
	UpdateOpacity(1.0f);
}

void AReEchoSwordArcActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	ElapsedTime += DeltaSeconds;
	const float Progress = FMath::Clamp(ElapsedTime / Lifetime, 0.0f, 1.0f);
	const float Opacity = FMath::Pow(1.0f - Progress, 1.35f);
	UpdateOpacity(Opacity);

	const float Expansion = FMath::Lerp(0.86f, 1.08f, Progress);
	SetActorScale3D(FVector(Expansion));

	if (Progress >= 1.0f)
	{
		Destroy();
	}
}

void AReEchoSwordArcActor::BuildArc(
	UInstancedStaticMeshComponent* SegmentComponent,
	const float Radius,
	const float Width,
	const float SwingDirection)
{
	SegmentComponent->ClearInstances();

	for (int32 Index = 0; Index < ReEchoSwordArc::SegmentCount; ++Index)
	{
		const float StartAlpha =
			static_cast<float>(Index) / ReEchoSwordArc::SegmentCount;
		const float EndAlpha =
			static_cast<float>(Index + 1) / ReEchoSwordArc::SegmentCount;
		const float StartAngle = FMath::DegreesToRadians(FMath::Lerp(
			ReEchoSwordArc::StartAngleDegrees,
			ReEchoSwordArc::EndAngleDegrees,
			StartAlpha));
		const float EndAngle = FMath::DegreesToRadians(FMath::Lerp(
			ReEchoSwordArc::StartAngleDegrees,
			ReEchoSwordArc::EndAngleDegrees,
			EndAlpha));

		const FVector StartPoint(
			FMath::Cos(StartAngle) * Radius,
			FMath::Sin(StartAngle) * Radius * SwingDirection,
			0.0f);
		const FVector EndPoint(
			FMath::Cos(EndAngle) * Radius,
			FMath::Sin(EndAngle) * Radius * SwingDirection,
			0.0f);
		const FVector SegmentDelta = EndPoint - StartPoint;
		const FVector Midpoint = (StartPoint + EndPoint) * 0.5f;
		const FVector Scale(
			SegmentDelta.Size() / ReEchoSwordArc::MeshSize,
			Width / ReEchoSwordArc::MeshSize,
			ReEchoSwordArc::SegmentHeight / ReEchoSwordArc::MeshSize);

		SegmentComponent->AddInstance(FTransform(
			SegmentDelta.Rotation(),
			Midpoint,
			Scale));
	}
}

void AReEchoSwordArcActor::UpdateOpacity(const float Opacity)
{
	if (GlowMaterial)
	{
		GlowMaterial->SetScalarParameterValue(TEXT("Opacity"), Opacity * 0.30f);
	}

	if (CoreMaterial)
	{
		CoreMaterial->SetScalarParameterValue(TEXT("Opacity"), Opacity * 0.82f);
	}
}