#include "Presentation/Scene/ReEchoArenaSceneActor.h"

#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Player/ReEchoPlayerPawn.h"
#include "UObject/ConstructorHelpers.h"

namespace ReEchoArenaScene
{
constexpr float MeshSize = 100.0f;
constexpr float FloorCenterZ = -55.0f;
constexpr float WallCenterZ = 100.0f;
constexpr float WallHalfHeight = 200.0f;
constexpr float WallHalfThickness = 25.0f;
}

AReEchoArenaSceneActor::AReEchoArenaSceneActor()
{
	PrimaryActorTick.bCanEverTick = true;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
	auto CreateSceneRoot = [this](const TCHAR* Name, USceneComponent* Parent)
	{
		USceneComponent* Component = CreateDefaultSubobject<USceneComponent>(Name);
		Component->SetupAttachment(Parent);
		return Component;
	};
	ArenaContentRoot = CreateSceneRoot(TEXT("ArenaContentRoot"), SceneRoot);
	VisualRoot = CreateSceneRoot(TEXT("VisualRoot"), ArenaContentRoot);
	GameplayRoot = CreateSceneRoot(TEXT("GameplayRoot"), ArenaContentRoot);
	GroundRoot = CreateSceneRoot(TEXT("Ground"), VisualRoot);
	GroundDetailRoot = CreateSceneRoot(TEXT("GroundDetail"), VisualRoot);
	MidDecorationRoot = CreateSceneRoot(TEXT("MidDecoration"), VisualRoot);
	ForegroundRoot = CreateSceneRoot(TEXT("Foreground"), VisualRoot);
	AtmosphereRoot = CreateSceneRoot(TEXT("Atmosphere"), VisualRoot);
	SceneEffectsRoot = CreateSceneRoot(TEXT("SceneEffects"), VisualRoot);
	CollisionRoot = CreateSceneRoot(TEXT("Collision"), GameplayRoot);
	ArenaCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ArenaCamera"));
	ArenaCamera->SetupAttachment(GameplayRoot);
	ArenaCamera->SetProjectionMode(ECameraProjectionMode::Orthographic);
	ArenaCamera->SetOrthoWidth(2800.0f);
	ArenaCamera->SetAspectRatio(1376.0f / 768.0f);
	ArenaCamera->SetConstraintAspectRatio(true);
	ArenaCamera->SetRelativeLocation(FVector(-630.0f, 0.0f, 900.0f));
	ArenaCamera->SetRelativeRotation(FRotator(-55.0f, 0.0f, 0.0f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneFinder(TEXT("/Engine/BasicShapes/Plane.Plane"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> SpriteMaterialFinder(
	    TEXT("/Paper2D/TranslucentUnlitSpriteMaterial.TranslucentUnlitSpriteMaterial"));
	BackdropMaterial = SpriteMaterialFinder.Object;
	Backdrop = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Backdrop"));
	Backdrop->SetupAttachment(GroundRoot);
	Backdrop->SetStaticMesh(PlaneFinder.Object);
	Backdrop->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Backdrop->SetCastShadow(false);
	Backdrop->SetTranslucentSortPriority(-100);

	auto CreateCollisionComponent = [this, Mesh = CubeFinder.Object](const TCHAR* Name)
	{
		UStaticMeshComponent* Component = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Component->SetupAttachment(CollisionRoot);
		Component->SetStaticMesh(Mesh);
		Component->SetCollisionProfileName(TEXT("BlockAll"));
		Component->SetCastShadow(false);
		Component->SetHiddenInGame(true);
		return Component;
	};
	Floor = CreateCollisionComponent(TEXT("Floor"));
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
	UpdateEditorLayout();
}

void AReEchoArenaSceneActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateFollowCamera(DeltaSeconds);
	UpdateParallax();
}

void AReEchoArenaSceneActor::SetFollowTarget(AReEchoPlayerPawn* Target)
{
	FollowTarget = Target;
	if (FollowTarget)
	{
		UpdateFollowCamera(0.0f);
	}
}

FVector2D AReEchoArenaSceneActor::GetPlayerHalfExtents() const
{
	return PlayerHalfExtents;
}

FVector2D AReEchoArenaSceneActor::GetEnemySpawnHalfExtents() const
{
	return EnemySpawnHalfExtents;
}

FVector2D AReEchoArenaSceneActor::GetArenaCenter() const
{
	const FVector Center = ArenaContentRoot ? ArenaContentRoot->GetComponentLocation() : GetActorLocation();
	return FVector2D(Center.X, Center.Y);
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
	const int32 Offset = FMath::RoundToInt(FVector2D::DotProduct(WorldFootpoint - WorldOrigin, SafeAxis) / UnitsPerStep);
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
	if (!ArenaContentRoot || !VisualRoot || !GameplayRoot || !GroundRoot || !GroundDetailRoot || !MidDecorationRoot ||
	    !ForegroundRoot || !AtmosphereRoot || !SceneEffectsRoot || !CollisionRoot || !ArenaCamera || !Backdrop ||
	    !Floor || !WallNorth || !WallSouth || !WallEast || !WallWest ||
	    !CameraClampBounds || !PlayerBounds || !EnemySpawnBounds)
	{
		return Fail(TEXT("Required Arena Scene components are missing."));
	}
	if (!MapTexture)
	{
		return Fail(TEXT("MapTexture is not assigned."));
	}
	if (BackdropHalfExtents.GetMin() < 100.0f || CameraClampHalfExtents.GetMin() < 100.0f ||
	    PlayerHalfExtents.GetMin() < 100.0f || EnemySpawnHalfExtents.GetMin() < 100.0f ||
	    ArenaCamera->OrthoWidth < 100.0f || ArenaCamera->AspectRatio <= KINDA_SMALL_NUMBER)
	{
		return Fail(TEXT("Arena bounds or camera projection are degenerate."));
	}
	if (ArenaCamera->GetForwardVector().Z >= -KINDA_SMALL_NUMBER)
	{
		return Fail(TEXT("Arena camera must face the gameplay plane."));
	}
	if (DepthSortAxis.IsNearlyZero() || DepthSortWorldUnitsPerStep < 1.0f)
	{
		return Fail(TEXT("Arena depth-sort configuration is degenerate."));
	}
	return true;
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
	FVector2D Result = MapCenter;
	for (int32 Axis = 0; Axis < 2; ++Axis)
	{
		const float SafeHalfExtent = MapHalfExtents[Axis] - FootprintHalfExtents[Axis];
		Result[Axis] =
		    SafeHalfExtent > 0.0f
		        ? FMath::Clamp(DesiredFocus[Axis], MapCenter[Axis] - SafeHalfExtent, MapCenter[Axis] + SafeHalfExtent)
		        : MapCenter[Axis];
	}
	return Result;
}

void AReEchoArenaSceneActor::UpdateEditorLayout()
{
	ArenaCamera->SetProjectionMode(ECameraProjectionMode::Orthographic);
	if (bAutoLayoutBackdrop)
	{
		Backdrop->SetRelativeLocation(FVector(0.0f, 0.0f, ReEchoArenaScene::FloorCenterZ + 1.0f));
		// map01 的 U 轴对应画面横向（世界 +Y），V 轴对应画面向下（世界 -X）。
		Backdrop->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
		Backdrop->SetRelativeScale3D(FVector(BackdropHalfExtents.Y * 2.0f / ReEchoArenaScene::MeshSize,
		                                     BackdropHalfExtents.X * 2.0f / ReEchoArenaScene::MeshSize,
		                                     1.0f));
	}
	if (BackdropMaterial && MapTexture)
	{
		UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(BackdropMaterial, this);
		Material->SetTextureParameterValue(TEXT("SpriteTexture"), MapTexture);
		Backdrop->SetMaterial(0, Material);
	}
	if (bAutoLayoutCollision)
	{
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

void AReEchoArenaSceneActor::UpdateFollowCamera(const float DeltaSeconds)
{
	if (!FollowTarget || !HasValidConfiguration())
	{
		return;
	}
	const FVector2D Footprint = CalculateGroundFootprintHalfExtents(
	    ArenaCamera->OrthoWidth, ArenaCamera->AspectRatio, ArenaCamera->GetComponentRotation());
	const FVector2D MapCenter = GetArenaCenter();
	const FVector2D Desired(FollowTarget->GetActorLocation().X, FollowTarget->GetActorLocation().Y);
	const FVector2D Clamped = ClampCameraFocus(Desired, MapCenter, CameraClampHalfExtents, Footprint);
	const FVector CurrentFocus3D = GetCameraGroundFocus();
	const FVector2D CurrentFocus(CurrentFocus3D.X, CurrentFocus3D.Y);
	const FVector2D NewFocus = bSmoothCameraFollow && DeltaSeconds > 0.0f
	                               ? FMath::Vector2DInterpTo(CurrentFocus, Clamped, DeltaSeconds, CameraFollowSpeed)
	                               : Clamped;
	SetCameraGroundFocus(NewFocus);
	if (!bLoggedUndersizedMap && (Footprint.X >= CameraClampHalfExtents.X || Footprint.Y >= CameraClampHalfExtents.Y))
	{
		UE_LOG(LogTemp,
		       Warning,
		       TEXT("[ArenaScene] Camera footprint exceeds map bounds; affected axes are center-locked."));
		bLoggedUndersizedMap = true;
	}
}

FVector AReEchoArenaSceneActor::GetCameraGroundFocus() const
{
	const FVector Origin = ArenaCamera->GetComponentLocation();
	const FVector Forward = ArenaCamera->GetForwardVector();
	if (FMath::Abs(Forward.Z) <= KINDA_SMALL_NUMBER)
	{
		return FVector(Origin.X, Origin.Y, GameplayPlaneZ);
	}
	return Origin + Forward * ((GameplayPlaneZ - Origin.Z) / Forward.Z);
}

void AReEchoArenaSceneActor::SetCameraGroundFocus(const FVector2D& Focus)
{
	const FVector Current = GetCameraGroundFocus();
	ArenaCamera->AddWorldOffset(FVector(Focus.X - Current.X, Focus.Y - Current.Y, 0.0f));
}

void AReEchoArenaSceneActor::UpdateParallax()
{
	auto SetLayerOffset = [this](USceneComponent* Layer, const float Factor, const FVector2D& CameraDelta)
	{
		if (!Layer)
		{
			return;
		}
		const FVector2D Offset = (CameraDelta * Factor).GetClampedToMaxSize(MaximumParallaxOffset);
		Layer->SetRelativeLocation(FVector(Offset.X, Offset.Y, 0.0f));
	};
	const FVector Focus = GetCameraGroundFocus();
	const FVector2D CameraDelta = FVector2D(Focus.X, Focus.Y) - GetArenaCenter();
	SetLayerOffset(MidDecorationRoot, bEnableParallax ? MidDecorationParallaxFactor : 0.0f, CameraDelta);
	SetLayerOffset(ForegroundRoot, bEnableParallax ? ForegroundParallaxFactor : 0.0f, CameraDelta);
	SetLayerOffset(AtmosphereRoot, bEnableParallax ? AtmosphereParallaxFactor : 0.0f, CameraDelta);
}
