#include "Presentation/Scene/ReEchoArenaCameraActor.h"

#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Curves/CurveFloat.h"
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
	PrimaryActorTick.bTickEvenWhenPaused = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics;
	CameraRoot = CreateDefaultSubobject<USceneComponent>(TEXT("CameraRoot"));
	SetRootComponent(CameraRoot);
	ArenaCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ArenaCamera"));
	ArenaCamera->SetupAttachment(CameraRoot);
	ArenaCamera->SetProjectionMode(ECameraProjectionMode::Orthographic);
	ArenaCamera->SetOrthoWidth(2560.0f);
	ArenaCamera->SetAspectRatio(1376.0f / 768.0f);
	ArenaCamera->SetConstraintAspectRatio(true);
	ImpactShakeBaseCameraLocation = ArenaCamera->GetRelativeLocation();
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> CountdownPostProcessFinder(
	    TEXT("/Game/ReEcho/Materials/PostProcess/EncounterTransition/M_PP_EncounterCountdownGhost_V4."
	         "M_PP_EncounterCountdownGhost_V4"));
	EncounterCountdownPostProcessMaterial = CountdownPostProcessFinder.Object;
	SetActorLocation(FVector(-900.0f, 0.0f, 900.0f));
	SetActorRotation(FRotator(-45.0f, 0.0f, 0.0f));
}

void AReEchoArenaCameraActor::Configure(AReEchoPlayerPawn* InFollowTarget, AReEchoArenaSceneActor* InArenaSource)
{
	SetTickableWhenPaused(true);
	if (FollowTarget && FollowTarget != InFollowTarget)
	{
		RemoveTickPrerequisiteActor(FollowTarget);
	}
	FollowTarget = InFollowTarget;
	ArenaSource = InArenaSource;
	if (FollowTarget)
	{
		AddTickPrerequisiteActor(FollowTarget);
		if (!bStage01To02CameraSequenceActive)
		{
			UpdateFollow(0.0f);
		}
	}
}

void AReEchoArenaCameraActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bStage01To02CameraSequenceActive && !bSequenceAdvancesWhenPaused && UGameplayStatics::IsGamePaused(this))
	{
		return;
	}
	if (!IsValid(FollowTarget))
	{
		Configure(Cast<AReEchoPlayerPawn>(UGameplayStatics::GetPlayerPawn(this, 0)), ArenaSource);
	}
	if (bStage01To02CameraSequenceActive)
	{
		UpdateStage01To02CameraMove(DeltaSeconds);
	}
	else
	{
		UpdateFollow(DeltaSeconds);
	}
	UpdateImpactShake(DeltaSeconds);
	if (EncounterCountdownPostProcessIntensity > 0.0f && EncounterCountdownPostProcessMID)
	{
		EncounterCountdownPostProcessPhase +=
		    DeltaSeconds * FMath::Lerp(MinimumPulseSpeed, MaximumPulseSpeed, EncounterCountdownPostProcessIntensity);
		EncounterCountdownPostProcessMID->SetScalarParameterValue(TEXT("PulsePhase"),
		                                                          EncounterCountdownPostProcessPhase);
	}
}

void AReEchoArenaCameraActor::PlayImpactShake(const float AmplitudeCm, const float DurationSeconds)
{
	ImpactShakeAmplitudeCm = FMath::Max(0.0f, AmplitudeCm);
	ImpactShakeDurationSeconds = FMath::Max(0.0f, DurationSeconds);
	ImpactShakeElapsedSeconds = 0.0f;
	UpdateImpactShake(0.0f);
}

FVector AReEchoArenaCameraActor::ResolveImpactShakeOffset(const float ElapsedSeconds,
                                                          const float DurationSeconds,
                                                          const float AmplitudeCm)
{
	if (DurationSeconds <= KINDA_SMALL_NUMBER || AmplitudeCm <= 0.0f || ElapsedSeconds < 0.0f ||
	    ElapsedSeconds >= DurationSeconds)
	{
		return FVector::ZeroVector;
	}
	const float Alpha = FMath::Clamp(ElapsedSeconds / DurationSeconds, 0.0f, 1.0f);
	const float Envelope = 1.0f - Alpha;
	const float Phase = Alpha * 6.0f * UE_PI;
	return FVector(0.0f,
	               FMath::Sin(Phase) * AmplitudeCm * Envelope,
	               FMath::Sin(Phase * 1.35f + UE_PI * 0.5f) * AmplitudeCm * 0.55f * Envelope);
}

void AReEchoArenaCameraActor::UpdateImpactShake(const float DeltaSeconds)
{
	if (!ArenaCamera)
	{
		return;
	}
	ImpactShakeElapsedSeconds += FMath::Max(0.0f, DeltaSeconds);
	ArenaCamera->SetRelativeLocation(
	    ImpactShakeBaseCameraLocation +
	    ResolveImpactShakeOffset(ImpactShakeElapsedSeconds, ImpactShakeDurationSeconds, ImpactShakeAmplitudeCm));
}

void AReEchoArenaCameraActor::BeginStage01To02CameraSequence(const bool bAdvanceWhenPaused)
{
	if (!ArenaCamera)
	{
		return;
	}
	SetActorTickEnabled(true);
	SetTickableWhenPaused(true);
	bStage01To02CameraSequenceActive = true;
	bSequenceAdvancesWhenPaused = bAdvanceWhenPaused;
	bStage01To02CameraMoveActive = false;
	Stage01To02CameraTarget = nullptr;
	Stage01To02SequenceStandardOrthoWidth = ArenaCamera->OrthoWidth;
	UE_LOG(LogTemp,
	       Display,
	       TEXT("[Stage01To02Camera] sequence begin focus=(%.1f,%.1f) ortho=%.1f tickEnabled=%s tickPaused=%s"),
	       GetGroundFocus().X,
	       GetGroundFocus().Y,
	       Stage01To02SequenceStandardOrthoWidth,
	       IsActorTickEnabled() ? TEXT("true") : TEXT("false"),
	       PrimaryActorTick.bTickEvenWhenPaused ? TEXT("true") : TEXT("false"));
}

float AReEchoArenaCameraActor::CalculateStage01To02CameraEaseAlpha(const float LinearAlpha)
{
	return FMath::SmoothStep(0.0f, 1.0f, FMath::Clamp(LinearAlpha, 0.0f, 1.0f));
}

bool AReEchoArenaCameraActor::FocusStage01To02Target(AActor* Target,
                                                     const float OrthoWidthRatio,
                                                     const float DurationSeconds)
{
	if (!bStage01To02CameraSequenceActive || !ArenaCamera || !IsValid(Target))
	{
		return false;
	}
	const FVector CurrentFocus = GetGroundFocus();
	Stage01To02MoveStartFocus = FVector2D(CurrentFocus.X, CurrentFocus.Y);
	Stage01To02CameraTarget = Target;
	Stage01To02MoveFallbackTargetFocus = FVector2D(Target->GetActorLocation().X, Target->GetActorLocation().Y);
	Stage01To02MoveStartOrthoWidth = ArenaCamera->OrthoWidth;
	Stage01To02MoveTargetOrthoWidth = Stage01To02SequenceStandardOrthoWidth * FMath::Clamp(OrthoWidthRatio, 0.1f, 1.0f);
	Stage01To02MoveElapsedSeconds = 0.0f;
	Stage01To02MoveDurationSeconds = FMath::Max(0.0f, DurationSeconds);
	bStage01To02CameraMoveActive = true;
	UpdateStage01To02CameraMove(0.0f);
	UE_LOG(LogTemp,
	       Display,
	       TEXT("[Stage01To02Camera] move target=%s duration=%.3f ortho=%.1f->%.1f"),
	       *GetNameSafe(Target),
	       Stage01To02MoveDurationSeconds,
	       Stage01To02MoveStartOrthoWidth,
	       Stage01To02MoveTargetOrthoWidth);
	return true;
}

bool AReEchoArenaCameraActor::FocusStage01To02TargetAtStandardWidth(AActor* Target, const float DurationSeconds)
{
	return FocusStage01To02Target(Target, 1.0f, DurationSeconds);
}

bool AReEchoArenaCameraActor::IsStage01To02CameraMoveComplete() const
{
	return bStage01To02CameraSequenceActive && !bStage01To02CameraMoveActive;
}

void AReEchoArenaCameraActor::EndStage01To02CameraSequence()
{
	if (!bStage01To02CameraSequenceActive)
	{
		return;
	}
	bStage01To02CameraSequenceActive = false;
	bStage01To02CameraMoveActive = false;
	Stage01To02CameraTarget = nullptr;
	if (ArenaCamera && Stage01To02SequenceStandardOrthoWidth > 0.0f)
	{
		ArenaCamera->SetOrthoWidth(Stage01To02SequenceStandardOrthoWidth);
	}
	UpdateFollow(0.0f);
	UE_LOG(LogTemp, Display, TEXT("[Stage01To02Camera] sequence completed; player follow restored."));
}

void AReEchoArenaCameraActor::CancelStage01To02CameraSequence()
{
	if (!bStage01To02CameraSequenceActive)
	{
		return;
	}
	bStage01To02CameraSequenceActive = false;
	bStage01To02CameraMoveActive = false;
	Stage01To02CameraTarget = nullptr;
	if (ArenaCamera && Stage01To02SequenceStandardOrthoWidth > 0.0f)
	{
		ArenaCamera->SetOrthoWidth(Stage01To02SequenceStandardOrthoWidth);
	}
	UpdateFollow(0.0f);
	UE_LOG(LogTemp, Warning, TEXT("[Stage01To02Camera] sequence cancelled; player follow restored."));
}

void AReEchoArenaCameraActor::UpdateStage01To02CameraMove(const float DeltaSeconds)
{
	if (!bStage01To02CameraMoveActive || !ArenaCamera)
	{
		return;
	}
	Stage01To02MoveElapsedSeconds += FMath::Max(0.0f, DeltaSeconds);
	const float LinearAlpha =
	    Stage01To02MoveDurationSeconds <= KINDA_SMALL_NUMBER
	        ? 1.0f
	        : FMath::Clamp(Stage01To02MoveElapsedSeconds / Stage01To02MoveDurationSeconds, 0.0f, 1.0f);
	const float EaseAlpha = Stage01To02CameraEaseCurve
	                            ? FMath::Clamp(Stage01To02CameraEaseCurve->GetFloatValue(LinearAlpha), 0.0f, 1.0f)
	                            : CalculateStage01To02CameraEaseAlpha(LinearAlpha);
	SetGroundFocus(FMath::Lerp(Stage01To02MoveStartFocus, ResolveTransitionTargetFocus(), EaseAlpha));
	ArenaCamera->SetOrthoWidth(FMath::Lerp(Stage01To02MoveStartOrthoWidth, Stage01To02MoveTargetOrthoWidth, EaseAlpha));
	if (LinearAlpha >= 1.0f)
	{
		bStage01To02CameraMoveActive = false;
		UE_LOG(LogTemp,
		       Display,
		       TEXT("[Stage01To02Camera] move completed target=%s focus=(%.1f,%.1f) ortho=%.1f"),
		       *GetNameSafe(Stage01To02CameraTarget),
		       GetGroundFocus().X,
		       GetGroundFocus().Y,
		       ArenaCamera->OrthoWidth);
	}
}

FVector2D AReEchoArenaCameraActor::ResolveTransitionTargetFocus() const
{
	FVector2D Desired = Stage01To02MoveFallbackTargetFocus;
	if (IsValid(Stage01To02CameraTarget))
	{
		Desired =
		    FVector2D(Stage01To02CameraTarget->GetActorLocation().X, Stage01To02CameraTarget->GetActorLocation().Y);
	}
	if (!bClampToArenaBounds || !ArenaSource || !ArenaCamera)
	{
		return Desired;
	}
	const FVector2D Footprint = AReEchoArenaSceneActor::CalculateGroundFootprintHalfExtents(
	    Stage01To02MoveTargetOrthoWidth, ArenaCamera->AspectRatio, GetActorRotation());
	return AReEchoArenaSceneActor::ClampCameraFocusWithInsets(Desired,
	                                                          ArenaSource->GetArenaCenter(),
	                                                          ArenaSource->GetCameraClampHalfExtents(),
	                                                          FVector2D(BottomEdgeInset, LeftEdgeInset),
	                                                          FVector2D(TopEdgeInset, RightEdgeInset),
	                                                          Footprint);
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
