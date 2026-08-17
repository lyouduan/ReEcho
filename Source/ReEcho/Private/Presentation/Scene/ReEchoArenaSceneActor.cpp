#include "Presentation/Scene/ReEchoArenaSceneActor.h"

#include "Camera/CameraComponent.h"
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
	ArenaCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ArenaCamera"));
	ArenaCamera->SetupAttachment(SceneRoot);
	ArenaCamera->SetProjectionMode(ECameraProjectionMode::Orthographic);
	ArenaCamera->SetRelativeLocation(FVector(-630.0f, 0.0f, 900.0f));
	ArenaCamera->SetRelativeRotation(FRotator(-55.0f, 0.0f, 0.0f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneFinder(TEXT("/Engine/BasicShapes/Plane.Plane"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> SpriteMaterialFinder(
	    TEXT("/Paper2D/TranslucentUnlitSpriteMaterial.TranslucentUnlitSpriteMaterial"));
	BackdropMaterial = SpriteMaterialFinder.Object;
	Backdrop = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Backdrop"));
	Backdrop->SetupAttachment(SceneRoot);
	Backdrop->SetStaticMesh(PlaneFinder.Object);
	Backdrop->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Backdrop->SetCastShadow(false);
	Backdrop->SetTranslucentSortPriority(-100);

	auto CreateCollisionComponent = [this, Mesh = CubeFinder.Object](const TCHAR* Name)
	{
		UStaticMeshComponent* Component = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Component->SetupAttachment(SceneRoot);
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
	if (!ArenaCamera || !Backdrop || !Floor || !WallNorth || !WallSouth || !WallEast || !WallWest)
	{
		return Fail(TEXT("Required Arena Scene components are missing."));
	}
	if (!MapTexture)
	{
		return Fail(TEXT("MapTexture is not assigned."));
	}
	if (BackdropHalfExtents.GetMin() < 100.0f || CameraClampHalfExtents.GetMin() < 100.0f ||
	    PlayerHalfExtents.GetMin() < 100.0f || EnemySpawnHalfExtents.GetMin() < 100.0f || CameraOrthoWidth < 100.0f ||
	    CameraAspectRatio <= KINDA_SMALL_NUMBER)
	{
		return Fail(TEXT("Arena bounds or camera projection are degenerate."));
	}
	if (ArenaCamera->GetForwardVector().Z >= -KINDA_SMALL_NUMBER)
	{
		return Fail(TEXT("Arena camera must face the gameplay plane."));
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
	ArenaCamera->SetOrthoWidth(CameraOrthoWidth);
	ArenaCamera->SetAspectRatio(CameraAspectRatio);
	ArenaCamera->SetConstraintAspectRatio(true);
	Backdrop->SetRelativeLocation(FVector(0.0f, 0.0f, ReEchoArenaScene::FloorCenterZ + 1.0f));
	Backdrop->SetRelativeRotation(FRotator::ZeroRotator);
	Backdrop->SetRelativeScale3D(FVector(BackdropHalfExtents.X * 2.0f / ReEchoArenaScene::MeshSize,
	                                     BackdropHalfExtents.Y * 2.0f / ReEchoArenaScene::MeshSize,
	                                     1.0f));
	if (BackdropMaterial && MapTexture)
	{
		UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(BackdropMaterial, this);
		Material->SetTextureParameterValue(TEXT("SpriteTexture"), MapTexture);
		Backdrop->SetMaterial(0, Material);
	}
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

void AReEchoArenaSceneActor::UpdateFollowCamera(const float DeltaSeconds)
{
	if (!FollowTarget || !HasValidConfiguration())
	{
		return;
	}
	const FVector2D Footprint = CalculateGroundFootprintHalfExtents(
	    ArenaCamera->OrthoWidth, ArenaCamera->AspectRatio, ArenaCamera->GetComponentRotation());
	const FVector2D MapCenter(GetActorLocation().X, GetActorLocation().Y);
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
