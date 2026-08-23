#include "Presentation/Scene/ReEchoArenaCameraActor.h"

#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Player/ReEchoPlayerPawn.h"
#include "Presentation/Scene/ReEchoArenaSceneActor.h"

AReEchoArenaCameraActor::AReEchoArenaCameraActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics;
	CameraRoot = CreateDefaultSubobject<USceneComponent>(TEXT("CameraRoot"));
	SetRootComponent(CameraRoot);
	ArenaCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ArenaCamera"));
	ArenaCamera->SetupAttachment(CameraRoot);
	ArenaCamera->SetProjectionMode(ECameraProjectionMode::Orthographic);
	ArenaCamera->SetOrthoWidth(2560.0f);
	ArenaCamera->SetAspectRatio(1376.0f / 768.0f);
	ArenaCamera->SetConstraintAspectRatio(true);
	SetActorLocation(FVector(-900.0f, 0.0f, 900.0f));
	SetActorRotation(FRotator(-45.0f, 0.0f, 0.0f));
}

void AReEchoArenaCameraActor::Configure(AReEchoPlayerPawn* InFollowTarget, AReEchoArenaSceneActor* InArenaSource)
{
	if (FollowTarget && FollowTarget != InFollowTarget)
	{
		RemoveTickPrerequisiteActor(FollowTarget);
	}
	FollowTarget = InFollowTarget;
	ArenaSource = InArenaSource;
	if (FollowTarget)
	{
		AddTickPrerequisiteActor(FollowTarget);
		UpdateFollow(0.0f);
	}
}

void AReEchoArenaCameraActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!IsValid(FollowTarget))
	{
		Configure(Cast<AReEchoPlayerPawn>(UGameplayStatics::GetPlayerPawn(this, 0)), ArenaSource);
	}
	UpdateFollow(DeltaSeconds);
}

void AReEchoArenaCameraActor::UpdateFollow(const float DeltaSeconds)
{
	if (!FollowTarget || !ArenaCamera || !ArenaSource)
	{
		return;
	}
	const FVector2D Desired(FollowTarget->GetActorLocation().X, FollowTarget->GetActorLocation().Y);
	FVector2D Target = Desired;
	if (bClampToArenaBounds)
	{
		const FVector2D Footprint = AReEchoArenaSceneActor::CalculateGroundFootprintHalfExtents(
		    ArenaCamera->OrthoWidth, ArenaCamera->AspectRatio, GetActorRotation());
		// With the default yaw, screen up/down maps to world +/-X and screen left/right maps to world -/+Y.
		const FVector2D NegativeAxisInsets(BottomEdgeInset, LeftEdgeInset);
		const FVector2D PositiveAxisInsets(TopEdgeInset, RightEdgeInset);
		Target = AReEchoArenaSceneActor::ClampCameraFocusWithInsets(Desired,
		                                                            ArenaSource->GetArenaCenter(),
		                                                            ArenaSource->GetCameraClampHalfExtents(),
		                                                            NegativeAxisInsets,
		                                                            PositiveAxisInsets,
		                                                            Footprint);
	}
	const FVector Current3D = GetGroundFocus();
	const FVector2D Current(Current3D.X, Current3D.Y);
	const FVector2D NewFocus = !bLockPlayerToCameraCenter && bSmoothCameraFollow && DeltaSeconds > 0.0f
	                               ? FMath::Vector2DInterpTo(Current, Target, DeltaSeconds, CameraFollowSpeed)
	                               : Target;
	SetGroundFocus(NewFocus);
}

FVector AReEchoArenaCameraActor::GetGroundFocus() const
{
	const FVector Origin = ArenaCamera->GetComponentLocation();
	const FVector Forward = ArenaCamera->GetForwardVector();
	const float PlaneZ = ArenaSource ? ArenaSource->GetGameplayPlaneWorldZ() : 0.0f;
	if (FMath::Abs(Forward.Z) <= KINDA_SMALL_NUMBER)
	{
		return FVector(Origin.X, Origin.Y, PlaneZ);
	}
	return Origin + Forward * ((PlaneZ - Origin.Z) / Forward.Z);
}

void AReEchoArenaCameraActor::SetGroundFocus(const FVector2D& Focus)
{
	const FVector Current = GetGroundFocus();
	AddActorWorldOffset(FVector(Focus.X - Current.X, Focus.Y - Current.Y, 0.0f));
}
