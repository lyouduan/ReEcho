#include "Presentation/Scene/ReEchoArenaCameraActor.h"

#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Player/ReEchoPlayerPawn.h"
#include "Presentation/Scene/ReEchoArenaSceneActor.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
constexpr float CountdownStartSeconds = 3.0f;
constexpr float CountdownFullStrengthSeconds = 1.0f;
constexpr float MinimumGhostOffsetPixels = 3.0f;
constexpr float MaximumGhostOffsetPixels = 28.0f;
constexpr float MaximumBlurRadiusPixels = 10.0f;
constexpr float MinimumPulseSpeed = 2.0f;
constexpr float MaximumPulseSpeed = 8.0f;
}

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
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> CountdownPostProcessFinder(
	    TEXT("/Game/ReEcho/Materials/PostProcess/EncounterTransition/M_PP_EncounterCountdownGhost_V4."
	         "M_PP_EncounterCountdownGhost_V4"));
	EncounterCountdownPostProcessMaterial = CountdownPostProcessFinder.Object;
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
	if (EncounterCountdownPostProcessIntensity > 0.0f && EncounterCountdownPostProcessMID)
	{
		EncounterCountdownPostProcessPhase +=
		    DeltaSeconds * FMath::Lerp(MinimumPulseSpeed, MaximumPulseSpeed, EncounterCountdownPostProcessIntensity);
		EncounterCountdownPostProcessMID->SetScalarParameterValue(TEXT("PulsePhase"),
		                                                          EncounterCountdownPostProcessPhase);
	}
}

float AReEchoArenaCameraActor::CalculateEncounterCountdownPostProcessIntensity(const float RemainingTime)
{
	if (RemainingTime <= 0.0f || RemainingTime > CountdownStartSeconds)
	{
		return 0.0f;
	}
	const float Linear = FMath::Clamp(
	    (CountdownStartSeconds - RemainingTime) / (CountdownStartSeconds - CountdownFullStrengthSeconds), 0.0f, 1.0f);
	return FMath::SmoothStep(0.0f, 1.0f, Linear);
}

void AReEchoArenaCameraActor::SetEncounterCountdownPostProcessIntensity(const float Intensity)
{
	const float PreviousIntensity = EncounterCountdownPostProcessIntensity;
	EncounterCountdownPostProcessIntensity = FMath::Clamp(Intensity, 0.0f, 1.0f);
	EnsureEncounterCountdownPostProcess();
	UpdateEncounterCountdownPostProcessParameters();
	if (PreviousIntensity <= 0.0f && EncounterCountdownPostProcessIntensity > 0.0f)
	{
		UE_LOG(LogTemp,
		       Display,
		       TEXT("[EncounterCountdownPostProcess] activated intensity=%.3f camera=%s material=%s mid=%s bound=%s "
		            "blendables=%d"),
		       EncounterCountdownPostProcessIntensity,
		       *GetNameSafe(ArenaCamera),
		       *GetNameSafe(EncounterCountdownPostProcessMaterial),
		       *GetNameSafe(EncounterCountdownPostProcessMID),
		       bEncounterCountdownPostProcessBound ? TEXT("true") : TEXT("false"),
		       ArenaCamera ? ArenaCamera->PostProcessSettings.WeightedBlendables.Array.Num() : 0);
	}
}

void AReEchoArenaCameraActor::ResetEncounterCountdownPostProcess()
{
	EncounterCountdownPostProcessIntensity = 0.0f;
	EncounterCountdownPostProcessPhase = 0.0f;
	UpdateEncounterCountdownPostProcessParameters();
}

void AReEchoArenaCameraActor::EnsureEncounterCountdownPostProcess()
{
	if (!ArenaCamera || !EncounterCountdownPostProcessMaterial)
	{
		UE_LOG(LogTemp,
		       Error,
		       TEXT("[EncounterCountdownPostProcess] bind failed camera=%s material=%s"),
		       *GetNameSafe(ArenaCamera),
		       *GetNameSafe(EncounterCountdownPostProcessMaterial));
		return;
	}
	if (!EncounterCountdownPostProcessMID)
	{
		EncounterCountdownPostProcessMID = UMaterialInstanceDynamic::Create(
		    EncounterCountdownPostProcessMaterial, this, TEXT("EncounterCountdownPostProcessMID"));
	}
	if (EncounterCountdownPostProcessMID && !bEncounterCountdownPostProcessBound)
	{
		ArenaCamera->AddOrUpdateBlendable(EncounterCountdownPostProcessMID, 1.0f);
		bEncounterCountdownPostProcessBound = true;
		UE_LOG(LogTemp,
		       Display,
		       TEXT("[EncounterCountdownPostProcess] bound material=%s mid=%s blendables=%d blendWeight=%.3f"),
		       *GetNameSafe(EncounterCountdownPostProcessMaterial),
		       *GetNameSafe(EncounterCountdownPostProcessMID),
		       ArenaCamera->PostProcessSettings.WeightedBlendables.Array.Num(),
		       ArenaCamera->PostProcessBlendWeight);
	}
}

void AReEchoArenaCameraActor::UpdateEncounterCountdownPostProcessParameters()
{
	if (!EncounterCountdownPostProcessMID)
	{
		return;
	}
	const float Intensity = EncounterCountdownPostProcessIntensity;
	EncounterCountdownPostProcessMID->SetScalarParameterValue(TEXT("EffectStrength"), Intensity);
	EncounterCountdownPostProcessMID->SetScalarParameterValue(
	    TEXT("GhostOffsetPixels"), FMath::Lerp(MinimumGhostOffsetPixels, MaximumGhostOffsetPixels, Intensity));
	EncounterCountdownPostProcessMID->SetScalarParameterValue(TEXT("BlurRadiusPixels"),
	                                                          MaximumBlurRadiusPixels * Intensity);
	EncounterCountdownPostProcessMID->SetScalarParameterValue(
	    TEXT("GhostOpacity"), FMath::Clamp(CountdownGhostStrength, 0.0f, 1.0f) * Intensity);
	EncounterCountdownPostProcessMID->SetScalarParameterValue(TEXT("PulsePhase"), EncounterCountdownPostProcessPhase);
	EncounterCountdownPostProcessMID->SetScalarParameterValue(TEXT("ClearCenterRadius"),
	                                                          FMath::Clamp(CountdownClearCenterRadius, 0.0f, 1.0f));
	EncounterCountdownPostProcessMID->SetScalarParameterValue(
	    TEXT("EdgeBlurRadius"), FMath::Max(CountdownEdgeBlurRadius, CountdownClearCenterRadius + 0.001f));
	EncounterCountdownPostProcessMID->SetScalarParameterValue(TEXT("EdgeMaskPower"),
	                                                          FMath::Max(CountdownEdgeMaskPower, 0.1f));
	EncounterCountdownPostProcessMID->SetScalarParameterValue(TEXT("RGBSeparationEnabled"),
	                                                          bCountdownEnableRGBSeparation ? 1.0f : 0.0f);
	EncounterCountdownPostProcessMID->SetScalarParameterValue(TEXT("RGBSeparationStrength"),
	                                                          FMath::Clamp(CountdownRGBSeparationStrength, 0.0f, 1.0f));
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
