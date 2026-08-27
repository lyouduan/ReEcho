#include "Presentation/Scene/ReEchoArenaSceneActor.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Player/ReEchoPlayerPawn.h"
#include "Presentation/Scene/ReEchoArenaSceneProfile.h"
#include "UObject/ConstructorHelpers.h"

namespace ReEchoArenaScene
{
constexpr float MeshSize = 100.0f;
constexpr float BackdropSurfaceZ = -0.5f;
constexpr float FloorCenterZ = -50.0f;
constexpr float WallCenterZ = 200.0f;
constexpr float WallHalfHeight = 200.0f;
constexpr float WallHalfThickness = 25.0f;
}

AReEchoArenaSceneActor::AReEchoArenaSceneActor()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
	auto CreateSceneRoot = [this](const TCHAR* Name, USceneComponent* Parent)
	{
		USceneComponent* Component = CreateDefaultSubobject<USceneComponent>(Name);
		Component->SetupAttachment(Parent);
		return Component;
	};
	MapRoot = CreateSceneRoot(TEXT("MapRoot"), SceneRoot);
	VisualRoot = CreateSceneRoot(TEXT("VisualRoot"), MapRoot);
	GameplayRoot = CreateSceneRoot(TEXT("GameplayRoot"), MapRoot);
	GroundRoot = CreateSceneRoot(TEXT("Ground"), VisualRoot);
	GroundDetailRoot = CreateSceneRoot(TEXT("GroundDetail"), VisualRoot);
	PlantRoot = CreateSceneRoot(TEXT("PlantRoot"), GroundDetailRoot);
	MidDecorationRoot = CreateSceneRoot(TEXT("MidDecoration"), VisualRoot);
	ForegroundRoot = CreateSceneRoot(TEXT("Foreground"), VisualRoot);
	AtmosphereRoot = CreateSceneRoot(TEXT("Atmosphere"), VisualRoot);
	SceneEffectsRoot = CreateSceneRoot(TEXT("SceneEffects"), VisualRoot);
	CollisionRoot = CreateSceneRoot(TEXT("Collision"), GameplayRoot);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneFinder(TEXT("/Engine/BasicShapes/Plane.Plane"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	Backdrop = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Backdrop"));
	Backdrop->SetupAttachment(MapRoot);
	Backdrop->SetStaticMesh(PlaneFinder.Object);
	Backdrop->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Backdrop->SetCastShadow(false);
	Backdrop->SetTranslucentSortPriority(-100);

	auto CreateCollisionComponent = [this, Mesh = CubeFinder.Object](const TCHAR* Name)
	{
		UStaticMeshComponent* Component = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Component->SetupAttachment(MapRoot);
		Component->SetStaticMesh(Mesh);
		Component->SetCollisionProfileName(TEXT("BlockAll"));
		Component->SetCastShadow(false);
		Component->SetHiddenInGame(true);
		return Component;
	};
	Floor = CreateCollisionComponent(TEXT("Floor"));
	// Characters are height-locked to GameplayPlaneZ and move with horizontal sweeps. A blocking floor exactly at
	// their footpoint can report initial contact/penetration and stall movement, so only the boundary walls block.
	Floor->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WallNorth = CreateCollisionComponent(TEXT("WallNorth"));
	WallSouth = CreateCollisionComponent(TEXT("WallSouth"));
	WallEast = CreateCollisionComponent(TEXT("WallEast"));
	WallWest = CreateCollisionComponent(TEXT("WallWest"));

	auto CreateBoundsVisualization = [this](const TCHAR* Name, const FColor Color)
	{
		UBoxComponent* Component = CreateDefaultSubobject<UBoxComponent>(Name);
		Component->SetupAttachment(GameplayRoot);
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Component->SetHiddenInGame(true);
		Component->ShapeColor = Color;
		return Component;
	};
	CameraClampBounds = CreateBoundsVisualization(TEXT("CameraClampBounds"), FColor::Cyan);
	PlayerBounds = CreateBoundsVisualization(TEXT("PlayerBounds"), FColor::Green);
	EnemySpawnBounds = CreateBoundsVisualization(TEXT("EnemySpawnBounds"), FColor::Yellow);
}

void AReEchoArenaSceneActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (!bUseEditorAuthoredSceneLayout)
	{
		UpdateEditorHierarchy();
		UpdateEditorLayout();
		ApplySceneProfile();
	}
}

FVector2D AReEchoArenaSceneActor::GetPlayerHalfExtents() const
{
	const FVector Extent = PlayerBounds ? PlayerBounds->GetScaledBoxExtent() : FVector::ZeroVector;
	return FVector2D(Extent.X, Extent.Y);
}

FVector2D AReEchoArenaSceneActor::GetCameraClampHalfExtents() const
{
	const FVector Extent = CameraClampBounds ? CameraClampBounds->GetScaledBoxExtent() : FVector::ZeroVector;
	return FVector2D(Extent.X, Extent.Y);
}

FVector2D AReEchoArenaSceneActor::GetEnemySpawnHalfExtents() const
{
	const FVector Extent = EnemySpawnBounds ? EnemySpawnBounds->GetScaledBoxExtent() : FVector::ZeroVector;
	return FVector2D(Extent.X, Extent.Y);
}

FVector2D AReEchoArenaSceneActor::GetArenaCenter() const
{
	const FVector Center = MapRoot ? MapRoot->GetComponentLocation() : GetActorLocation();
	return FVector2D(Center.X, Center.Y);
}

float AReEchoArenaSceneActor::GetGameplayPlaneWorldZ() const
{
	return CalculateGameplayPlaneWorldZ(MapRoot ? MapRoot->GetComponentTransform() : GetActorTransform(),
	                                    GameplayPlaneZ);
}

float AReEchoArenaSceneActor::CalculateGameplayPlaneWorldZ(const FTransform& MapTransform,
                                                           const float LocalGameplayPlaneZ)
{
	return MapTransform.TransformPosition(FVector(0.0f, 0.0f, LocalGameplayPlaneZ)).Z;
}

FVector2D AReEchoArenaSceneActor::GetMapScale2D() const
{
	const FVector Scale = MapRoot ? MapRoot->GetComponentScale() : FVector::OneVector;
	return FVector2D(FMath::Abs(Scale.X), FMath::Abs(Scale.Y));
}

int32 AReEchoArenaSceneActor::CalculateFootpointSortPriority(const FVector& WorldFootpoint) const
{
	return CalculateFootpointSortPriority(FVector2D(WorldFootpoint.X, WorldFootpoint.Y),
	                                      GetArenaCenter(),
	                                      DepthSortAxis,
	                                      DepthSortWorldUnitsPerStep,
	                                      DepthSortBasePriority,
	                                      DepthSortPriorityRange);
}

int32 AReEchoArenaSceneActor::CalculateFootpointSortPriority(const FVector2D& WorldFootpoint,
                                                             const FVector2D& WorldOrigin,
                                                             const FVector2D& SortAxis,
                                                             const float WorldUnitsPerStep,
                                                             const int32 BasePriority,
                                                             const FIntPoint& PriorityRange)
{
	const FVector2D SafeAxis = SortAxis.GetSafeNormal();
	const float UnitsPerStep = FMath::Max(WorldUnitsPerStep, 1.0f);
	const int32 Offset =
	    FMath::RoundToInt(FVector2D::DotProduct(WorldFootpoint - WorldOrigin, SafeAxis) / UnitsPerStep);
	const int32 MinimumPriority = FMath::Min(PriorityRange.X, PriorityRange.Y);
	const int32 MaximumPriority = FMath::Max(PriorityRange.X, PriorityRange.Y);
	return BasePriority + FMath::Clamp(Offset, MinimumPriority, MaximumPriority);
}

bool AReEchoArenaSceneActor::HasValidConfiguration(FString* OutReason) const
{
	auto Fail = [OutReason](const TCHAR* Reason)
	{
		if (OutReason)
		{
			*OutReason = Reason;
		}
		return false;
	};
	if (!MapRoot || !VisualRoot || !GameplayRoot || !GroundRoot || !GroundDetailRoot || !PlantRoot ||
	    !MidDecorationRoot || !ForegroundRoot || !AtmosphereRoot || !SceneEffectsRoot || !CollisionRoot || !Backdrop ||
	    !Floor || !WallNorth || !WallSouth || !WallEast || !WallWest || !CameraClampBounds || !PlayerBounds ||
	    !EnemySpawnBounds)
	{
		return Fail(TEXT("Required Arena Scene components are missing."));
	}
	if (!Backdrop->GetMaterial(0))
	{
		return Fail(TEXT("Backdrop Material Slot 0 must provide the arena map material."));
	}
	if (GetCameraClampHalfExtents().GetMin() < 100.0f || GetPlayerHalfExtents().GetMin() < 100.0f ||
	    GetEnemySpawnHalfExtents().GetMin() < 100.0f)
	{
		return Fail(TEXT("Arena bounds are degenerate."));
	}
	if (DepthSortAxis.IsNearlyZero() || DepthSortWorldUnitsPerStep < 1.0f)
	{
		return Fail(TEXT("Arena depth-sort configuration is degenerate."));
	}
	if (GetMapScale2D().GetMin() <= KINDA_SMALL_NUMBER)
	{
		return Fail(TEXT("MapRoot scale must be positive on the gameplay axes."));
	}
	return true;
}

bool AReEchoArenaSceneActor::BuildSceneRegistry(const TArray<FReEchoArenaSceneRegistration>& Registrations,
                                                TMap<FName, TSubclassOf<AReEchoArenaSceneActor>>& OutRegistry,
                                                FString& OutError)
{
	return UReEchoArenaSceneCatalog::BuildRegistry(Registrations, OutRegistry, OutError);
}

FVector2D AReEchoArenaSceneActor::CalculateGroundFootprintHalfExtents(const float OrthoWidth,
                                                                      const float AspectRatio,
                                                                      const FRotator& CameraRotation)
{
	const float HalfWidth = FMath::Max(0.0f, OrthoWidth) * 0.5f;
	const float HalfHeight = HalfWidth / FMath::Max(AspectRatio, KINDA_SMALL_NUMBER);
	const FVector Right = FRotationMatrix(CameraRotation).GetUnitAxis(EAxis::Y);
	const FVector Up = FRotationMatrix(CameraRotation).GetUnitAxis(EAxis::Z);
	const FVector Forward = FRotationMatrix(CameraRotation).GetUnitAxis(EAxis::X);
	if (FMath::Abs(Forward.Z) <= KINDA_SMALL_NUMBER)
	{
		return FVector2D(TNumericLimits<float>::Max());
	}
	FVector2D Result = FVector2D::ZeroVector;
	for (const float HorizontalSign : {-1.0f, 1.0f})
	{
		for (const float VerticalSign : {-1.0f, 1.0f})
		{
			const FVector ViewPlaneOffset = Right * HalfWidth * HorizontalSign + Up * HalfHeight * VerticalSign;
			const FVector GroundOffset = ViewPlaneOffset - Forward * (ViewPlaneOffset.Z / Forward.Z);
			Result.X = FMath::Max(Result.X, FMath::Abs(GroundOffset.X));
			Result.Y = FMath::Max(Result.Y, FMath::Abs(GroundOffset.Y));
		}
	}
	return Result;
}

FVector2D AReEchoArenaSceneActor::ClampCameraFocus(const FVector2D& DesiredFocus,
                                                   const FVector2D& MapCenter,
                                                   const FVector2D& MapHalfExtents,
                                                   const FVector2D& FootprintHalfExtents)
{
	return ClampCameraFocusWithInsets(
	    DesiredFocus, MapCenter, MapHalfExtents, FVector2D::ZeroVector, FVector2D::ZeroVector, FootprintHalfExtents);
}

FVector2D AReEchoArenaSceneActor::ClampCameraFocusWithInsets(const FVector2D& DesiredFocus,
                                                             const FVector2D& MapCenter,
                                                             const FVector2D& MapHalfExtents,
                                                             const FVector2D& NegativeAxisInsets,
                                                             const FVector2D& PositiveAxisInsets,
                                                             const FVector2D& FootprintHalfExtents)
{
	FVector2D Result = MapCenter;
	for (int32 Axis = 0; Axis < 2; ++Axis)
	{
		const float MinimumFocus = MapCenter[Axis] - MapHalfExtents[Axis] + FMath::Max(0.0f, NegativeAxisInsets[Axis]) +
		                           FootprintHalfExtents[Axis];
		const float MaximumFocus = MapCenter[Axis] + MapHalfExtents[Axis] - FMath::Max(0.0f, PositiveAxisInsets[Axis]) -
		                           FootprintHalfExtents[Axis];
		Result[Axis] = MinimumFocus <= MaximumFocus ? FMath::Clamp(DesiredFocus[Axis], MinimumFocus, MaximumFocus)
		                                            : (MinimumFocus + MaximumFocus) * 0.5f;
	}
	return Result;
}

void AReEchoArenaSceneActor::UpdateEditorHierarchy()
{
	auto AttachDirectlyToMapRoot = [this](USceneComponent* Component)
	{
		if (Component && Component->GetAttachParent() != MapRoot)
		{
			Component->AttachToComponent(MapRoot, FAttachmentTransformRules::KeepRelativeTransform);
		}
	};
	AttachDirectlyToMapRoot(Backdrop);
	AttachDirectlyToMapRoot(Floor);
	AttachDirectlyToMapRoot(WallNorth);
	AttachDirectlyToMapRoot(WallSouth);
	AttachDirectlyToMapRoot(WallEast);
	AttachDirectlyToMapRoot(WallWest);
}

void AReEchoArenaSceneActor::UpdateEditorLayout()
{
	if (bAutoLayoutBackdrop)
	{
		Backdrop->SetRelativeLocation(FVector(0.0f, 0.0f, ReEchoArenaScene::BackdropSurfaceZ));
		// Arena map U is screen-right (world +Y); V is screen-down (world -X).
		Backdrop->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
		Backdrop->SetRelativeScale3D(FVector(BackdropHalfExtents.Y * 2.0f / ReEchoArenaScene::MeshSize,
		                                     BackdropHalfExtents.X * 2.0f / ReEchoArenaScene::MeshSize,
		                                     1.0f));
	}
	if (MapMaterial && !SceneProfile)
	{
		Backdrop->SetMaterial(0, MapMaterial);
	}
	if (bAutoLayoutCollision)
	{
		Floor->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Floor->SetRelativeLocation(FVector(0.0f, 0.0f, ReEchoArenaScene::FloorCenterZ));
		Floor->SetRelativeScale3D(FVector(PlayerHalfExtents.X / 50.0f, PlayerHalfExtents.Y / 50.0f, 1.0f));
		const float WallScaleX = PlayerHalfExtents.X / 50.0f;
		const float WallScaleY = PlayerHalfExtents.Y / 50.0f;
		const float WallThickness = ReEchoArenaScene::WallHalfThickness / 50.0f;
		const float WallHeight = ReEchoArenaScene::WallHalfHeight / 50.0f;
		WallNorth->SetRelativeLocation(FVector(0.0f, PlayerHalfExtents.Y, ReEchoArenaScene::WallCenterZ));
		WallNorth->SetRelativeScale3D(FVector(WallScaleX, WallThickness, WallHeight));
		WallSouth->SetRelativeLocation(FVector(0.0f, -PlayerHalfExtents.Y, ReEchoArenaScene::WallCenterZ));
		WallSouth->SetRelativeScale3D(FVector(WallScaleX, WallThickness, WallHeight));
		WallEast->SetRelativeLocation(FVector(PlayerHalfExtents.X, 0.0f, ReEchoArenaScene::WallCenterZ));
		WallEast->SetRelativeScale3D(FVector(WallThickness, WallScaleY, WallHeight));
		WallWest->SetRelativeLocation(FVector(-PlayerHalfExtents.X, 0.0f, ReEchoArenaScene::WallCenterZ));
		WallWest->SetRelativeScale3D(FVector(WallThickness, WallScaleY, WallHeight));
	}
	CameraClampBounds->SetRelativeLocation(FVector(0.0f, 0.0f, GameplayPlaneZ + 5.0f));
	CameraClampBounds->SetBoxExtent(FVector(CameraClampHalfExtents.X, CameraClampHalfExtents.Y, 5.0f));
	PlayerBounds->SetRelativeLocation(FVector(0.0f, 0.0f, GameplayPlaneZ + 10.0f));
	PlayerBounds->SetBoxExtent(FVector(PlayerHalfExtents.X, PlayerHalfExtents.Y, 5.0f));
	EnemySpawnBounds->SetRelativeLocation(FVector(0.0f, 0.0f, GameplayPlaneZ + 15.0f));
	EnemySpawnBounds->SetBoxExtent(FVector(EnemySpawnHalfExtents.X, EnemySpawnHalfExtents.Y, 5.0f));
}

void AReEchoArenaSceneActor::ApplySceneProfile()
{
	if (!Backdrop || !SceneProfile || !SceneProfile->MapMaterial)
	{
		return;
	}
	UMaterialInstanceDynamic* MaterialInstance = Backdrop->CreateDynamicMaterialInstance(0, SceneProfile->MapMaterial);
	if (!MaterialInstance)
	{
		Backdrop->SetMaterial(0, SceneProfile->MapMaterial);
		return;
	}
	MaterialInstance->SetVectorParameterValue(TEXT("GroundTint"), SceneProfile->GroundTint);
	MaterialInstance->SetScalarParameterValue(TEXT("GroundBrightness"), SceneProfile->GroundBrightness);
	MaterialInstance->SetScalarParameterValue(TEXT("GroundSaturation"), SceneProfile->GroundSaturation);
	MaterialInstance->SetScalarParameterValue(TEXT("GroundContrast"), SceneProfile->GroundContrast);
}
