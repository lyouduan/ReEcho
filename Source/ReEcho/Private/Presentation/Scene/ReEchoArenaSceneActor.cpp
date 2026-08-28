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

bool AReEchoArenaSceneActor::CalculateWallDerivedSpawnBounds(const FBox2D& WallBoundsA,
                                                              const FBox2D& WallBoundsB,
                                                              const FBox2D& WallBoundsC,
                                                              const FBox2D& WallBoundsD,
                                                              const float Padding,
                                                              FBox2D& OutBounds,
                                                              FString* OutReason)
{
	OutBounds = FBox2D(ForceInit);
	const TArray<FBox2D, TInlineAllocator<4>> WallBounds = {
	    WallBoundsA, WallBoundsB, WallBoundsC, WallBoundsD};
	auto FormatBounds = [&WallBounds]()
	{
		FString Result;
		for (int32 Index = 0; Index < WallBounds.Num(); ++Index)
		{
			const FBox2D& Bounds = WallBounds[Index];
			Result += FString::Printf(TEXT("%s%d=[(%.2f,%.2f)-(%.2f,%.2f)]"),
			                          Index > 0 ? TEXT(" ") : TEXT(""),
			                          Index,
			                          Bounds.Min.X,
			                          Bounds.Min.Y,
			                          Bounds.Max.X,
			                          Bounds.Max.Y);
		}
		return Result;
	};
	auto Fail = [OutReason, &FormatBounds](const FString& Reason)
	{
		if (OutReason)
		{
			*OutReason = FString::Printf(TEXT("%s Walls: %s"), *Reason, *FormatBounds());
		}
		return false;
	};
	if (Padding < 0.0f || WallBounds.ContainsByPredicate([](const FBox2D& Bounds) { return !Bounds.bIsValid; }))
	{
		return Fail(TEXT("Wall bounds or spawn padding are invalid."));
	}

	// Exactly three unique ways exist to split four walls into two opposite pairs. Evaluate all of them because
	// Blueprint component names and transformed long axes do not reliably identify world left/right/top/bottom.
	constexpr int32 Pairings[3][4] = {{0, 1, 2, 3}, {0, 2, 1, 3}, {0, 3, 1, 2}};
	float BestArea = -1.0f;
	FBox2D BestBounds(ForceInit);
	FString CandidateDiagnostics;
	for (int32 PairingIndex = 0; PairingIndex < UE_ARRAY_COUNT(Pairings); ++PairingIndex)
	{
		for (int32 Orientation = 0; Orientation < 2; ++Orientation)
		{
			const int32 XOffset = Orientation == 0 ? 0 : 2;
			const int32 YOffset = Orientation == 0 ? 2 : 0;
			const FBox2D& FirstX = WallBounds[Pairings[PairingIndex][XOffset]];
			const FBox2D& SecondX = WallBounds[Pairings[PairingIndex][XOffset + 1]];
			const FBox2D& LeftWall = FirstX.GetCenter().X < SecondX.GetCenter().X ? FirstX : SecondX;
			const FBox2D& RightWall = &LeftWall == &FirstX ? SecondX : FirstX;
			const FBox2D& FirstY = WallBounds[Pairings[PairingIndex][YOffset]];
			const FBox2D& SecondY = WallBounds[Pairings[PairingIndex][YOffset + 1]];
			const FBox2D& BottomWall = FirstY.GetCenter().Y < SecondY.GetCenter().Y ? FirstY : SecondY;
			const FBox2D& TopWall = &BottomWall == &FirstY ? SecondY : FirstY;
			const FVector2D Minimum(LeftWall.Max.X + Padding, BottomWall.Max.Y + Padding);
			const FVector2D Maximum(RightWall.Min.X - Padding, TopWall.Min.Y - Padding);
			const FVector2D Size = Maximum - Minimum;
			const FVector2D XCenterDelta = FirstX.GetCenter() - SecondX.GetCenter();
			const FVector2D YCenterDelta = FirstY.GetCenter() - SecondY.GetCenter();
			const bool bAxisSeparationValid = FMath::Abs(XCenterDelta.X) >= FMath::Abs(XCenterDelta.Y) &&
			                                      FMath::Abs(YCenterDelta.Y) >= FMath::Abs(YCenterDelta.X);
			const float Area = bAxisSeparationValid && Size.X > 0.0f && Size.Y > 0.0f ? Size.X * Size.Y : -1.0f;
			CandidateDiagnostics += FString::Printf(TEXT("%sP%d%c=[(%.2f,%.2f)-(%.2f,%.2f)]"),
			                                        CandidateDiagnostics.IsEmpty() ? TEXT("") : TEXT(" "),
			                                        PairingIndex,
			                                        Orientation == 0 ? TEXT('A') : TEXT('B'),
			                                        Minimum.X,
			                                        Minimum.Y,
			                                        Maximum.X,
			                                        Maximum.Y);
			if (Area > BestArea)
			{
				BestArea = Area;
				BestBounds = FBox2D(Minimum, Maximum);
			}
		}
	}
	if (BestArea <= 0.0f || !BestBounds.bIsValid)
	{
		return Fail(FString::Printf(TEXT("No opposite-wall pairing produces usable padded bounds. Candidates: %s"),
		                            *CandidateDiagnostics));
	}
	OutBounds = BestBounds;
	return true;
}

bool AReEchoArenaSceneActor::GetEnemySpawnWorldBounds(FBox2D& OutBounds, FString* OutReason) const
{
	auto ComponentBounds = [](const UStaticMeshComponent* Component)
	{
		return Component && Component->GetStaticMesh()
		           ? CalculateWorldXYBounds(Component->GetStaticMesh()->GetBoundingBox(), Component->GetComponentTransform())
		           : FBox2D(ForceInit);
	};
	const FBox2D WestBounds = ComponentBounds(WallWest);
	const FBox2D EastBounds = ComponentBounds(WallEast);
	const FBox2D SouthBounds = ComponentBounds(WallSouth);
	const FBox2D NorthBounds = ComponentBounds(WallNorth);
	FString SolverReason;
	const bool bResolved = CalculateWallDerivedSpawnBounds(WestBounds,
	                                                      EastBounds,
	                                                      SouthBounds,
	                                                      NorthBounds,
	                                                      EnemySpawnWallPadding,
	                                                      OutBounds,
	                                                      &SolverReason);
	if (!bResolved && OutReason)
	{
		auto FormatBox = [](const FBox2D& Bounds)
		{
			return FString::Printf(TEXT("[(%.2f,%.2f)-(%.2f,%.2f)]"),
			                       Bounds.Min.X,
			                       Bounds.Min.Y,
			                       Bounds.Max.X,
			                       Bounds.Max.Y);
		};
		*OutReason = FString::Printf(TEXT("WallWest=%s WallEast=%s WallSouth=%s WallNorth=%s Solver=%s"),
		                             *FormatBox(WestBounds),
		                             *FormatBox(EastBounds),
		                             *FormatBox(SouthBounds),
		                             *FormatBox(NorthBounds),
		                             *SolverReason);
	}
	return bResolved;
}

FVector2D AReEchoArenaSceneActor::GetArenaCenter() const
{
	const FVector Center = MapRoot ? MapRoot->GetComponentLocation() : GetActorLocation();
	return FVector2D(Center.X, Center.Y);
}

float AReEchoArenaSceneActor::GetGameplayPlaneWorldZ() const
{
	return CalculateGameplayPlaneWorldZ(GetActorTransform(), GameplayPlaneZ);
}

float AReEchoArenaSceneActor::CalculateGameplayPlaneWorldZ(const FTransform& MapTransform,
                                                           const float LocalGameplayPlaneZ)
{
	return MapTransform.TransformPosition(FVector(0.0f, 0.0f, LocalGameplayPlaneZ)).Z;
}

FBox2D AReEchoArenaSceneActor::CalculateWorldXYBounds(const FBox& LocalBounds, const FTransform& LocalToWorld)
{
	FBox2D Result(ForceInit);
	for (int32 CornerIndex = 0; CornerIndex < 8; ++CornerIndex)
	{
		const FVector LocalCorner((CornerIndex & 1) ? LocalBounds.Max.X : LocalBounds.Min.X,
		                          (CornerIndex & 2) ? LocalBounds.Max.Y : LocalBounds.Min.Y,
		                          (CornerIndex & 4) ? LocalBounds.Max.Z : LocalBounds.Min.Z);
		const FVector WorldCorner = LocalToWorld.TransformPosition(LocalCorner);
		Result += FVector2D(WorldCorner.X, WorldCorner.Y);
	}
	return Result;
}

bool AReEchoArenaSceneActor::DoesBackdropCoverCameraBounds(const float Tolerance) const
{
	if (!Backdrop || !Backdrop->GetStaticMesh() || !CameraClampBounds)
	{
		return false;
	}
	const FBox2D BackdropBounds =
	    CalculateWorldXYBounds(Backdrop->GetStaticMesh()->GetBoundingBox(), Backdrop->GetComponentTransform());
	const FVector CameraExtent = CameraClampBounds->GetUnscaledBoxExtent();
	const FBox2D CameraBounds = CalculateWorldXYBounds(FBox(-CameraExtent, CameraExtent),
	                                                     CameraClampBounds->GetComponentTransform());
	const float SafeTolerance = FMath::Max(0.0f, Tolerance);
	return BackdropBounds.bIsValid && CameraBounds.bIsValid &&
	       BackdropBounds.Min.X <= CameraBounds.Min.X + SafeTolerance &&
	       BackdropBounds.Min.Y <= CameraBounds.Min.Y + SafeTolerance &&
	       BackdropBounds.Max.X >= CameraBounds.Max.X - SafeTolerance &&
	       BackdropBounds.Max.Y >= CameraBounds.Max.Y - SafeTolerance;
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
	if (GetCameraClampHalfExtents().GetMin() < 100.0f || GetPlayerHalfExtents().GetMin() < 100.0f)
	{
		return Fail(TEXT("Arena bounds are degenerate."));
	}
	FBox2D SpawnBounds(ForceInit);
	FString SpawnBoundsReason;
	if (!GetEnemySpawnWorldBounds(SpawnBounds, &SpawnBoundsReason))
	{
		if (OutReason)
		{
			*OutReason = FString::Printf(TEXT("Arena wall-derived spawn bounds are invalid: %s"), *SpawnBoundsReason);
		}
		return false;
	}
	if (DepthSortAxis.IsNearlyZero() || DepthSortWorldUnitsPerStep < 1.0f)
	{
		return Fail(TEXT("Arena depth-sort configuration is degenerate."));
	}
	if (GetMapScale2D().GetMin() <= KINDA_SMALL_NUMBER)
	{
		return Fail(TEXT("MapRoot scale must be positive on the gameplay axes."));
	}
	if (!DoesBackdropCoverCameraBounds())
	{
		return Fail(TEXT("Backdrop mesh world XY footprint must cover CameraClampBounds."));
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
