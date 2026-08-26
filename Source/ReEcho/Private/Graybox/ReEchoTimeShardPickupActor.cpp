#include "Graybox/ReEchoTimeShardPickupActor.h"

#include "ReEcho.h"
#include "ReEchoGameMode.h"
#include "ReEchoAudioEvents.h"
#include "ReEchoAudioService.h"
#include "ReEchoAudioTypes.h"
#include "Components/MaterialBillboardComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Player/ReEchoPlayerPawn.h"
#include "Run/ReEchoRunSubsystem.h"
#include "UObject/ConstructorHelpers.h"

namespace ReEchoTimeShardPickup
{
constexpr float DefaultAttractionRadiusCm = 300.0f;
} // namespace ReEchoTimeShardPickup

AReEchoTimeShardPickupActor::AReEchoTimeShardPickupActor()
{
	PrimaryActorTick.bCanEverTick = true;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->InitSphereRadius(ReEchoTimeShardPickup::DefaultAttractionRadiusCm);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Collision->SetGenerateOverlapEvents(true);
	Collision->OnComponentBeginOverlap.AddDynamic(this, &AReEchoTimeShardPickupActor::HandleBeginOverlap);

	PresentationRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PresentationRoot"));
	PresentationRoot->SetupAttachment(Collision);
	PresentationRoot->bEditableWhenInherited = true;
	VisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualRoot"));
	VisualRoot->SetupAttachment(PresentationRoot);
	VisualRoot->bEditableWhenInherited = true;
	GroundRoot = CreateDefaultSubobject<USceneComponent>(TEXT("GroundRoot"));
	GroundRoot->SetupAttachment(PresentationRoot);
	GroundRoot->bEditableWhenInherited = true;

	Visual = CreateDefaultSubobject<UMaterialBillboardComponent>(TEXT("TimeShardVisual"));
	Visual->SetupAttachment(VisualRoot);
	Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Visual->SetCastShadow(false);
	Visual->SetRelativeLocation(FVector::ZeroVector);
	Visual->SetHiddenInGame(false);
	Visual->SetVisibility(true);
	Visual->bEditableWhenInherited = true;
	static ConstructorHelpers::FObjectFinder<UTexture2D> TimeShardTexture(
	    TEXT("/Game/ReEcho/Textures/Pickups/T_TimeShard.T_TimeShard"));
	PickupTexture = TimeShardTexture.Object;
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> TimeShardMaterial(
	    TEXT("/Game/ReEcho/Materials/Pickups/MI_TimeShardPickup.MI_TimeShardPickup"));
	PickupMaterial = TimeShardMaterial.Object;

	GroundShadow = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GroundShadow"));
	GroundShadow->SetupAttachment(GroundRoot);
	GroundShadow->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GroundShadow->SetCastShadow(false);
	GroundShadow->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")));
	GroundShadow->SetRelativeLocation(FVector::ZeroVector);
	GroundShadow->SetRelativeScale3D(FVector(0.48f, 0.24f, 1.0f));
	GroundShadow->bEditableWhenInherited = true;
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GroundShadowMaterial(
	    TEXT("/Game/ReEcho/Materials/M_GroundShadow_Procedural.M_GroundShadow_Procedural"));
	GroundShadow->SetMaterial(0, GroundShadowMaterial.Object);

	ApplyEditablePresentationSettings();

	SetLifeSpan(20.0f);
}

void AReEchoTimeShardPickupActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyEditablePresentationSettings();
}

void AReEchoTimeShardPickupActor::BeginPlay()
{
	Super::BeginPlay();
	SnapToArenaGroundPlane();
	RuntimePickupMaterial = PickupMaterial ? UMaterialInstanceDynamic::Create(PickupMaterial, this) : nullptr;
	if (RuntimePickupMaterial)
	{
		RuntimePickupMaterial->SetScalarParameterValue(TEXT("Opacity"), 1.0f);
	}
	ApplyEditablePresentationSettings();
	CacheAuthoredPresentationTransform();
	UpdateLandingPresentation(0.0f);
}

float AReEchoTimeShardPickupActor::ResolveAttractionSpeed(const float PlayerPlanarSpeedCmPerSecond,
	                                                       const float MinimumSpeedCmPerSecond,
	                                                       const float SpeedAdvantageCmPerSecond)
{
	return FMath::Max(FMath::Max(0.0f, MinimumSpeedCmPerSecond),
	                  FMath::Max(0.0f, PlayerPlanarSpeedCmPerSecond) +
	                      FMath::Max(0.0f, SpeedAdvantageCmPerSecond));
}

void AReEchoTimeShardPickupActor::ApplyEditablePresentationSettings()
{
	if (!Visual || !PickupTexture || !PickupMaterial)
	{
		return;
	}

	const float SafeHeight = FMath::Max(VisualWorldHeightCm, 1.0f);
	const float VisualWorldWidthCm =
	    SafeHeight * static_cast<float>(PickupTexture->GetSizeX()) / FMath::Max(1, PickupTexture->GetSizeY());
	FMaterialSpriteElement Element;
	Element.Material = RuntimePickupMaterial ? RuntimePickupMaterial.Get() : PickupMaterial.Get();
	Element.bSizeIsInScreenSpace = false;
	Element.BaseSizeX = VisualWorldWidthCm;
	Element.BaseSizeY = SafeHeight;
	Visual->SetElements({Element});
	Visual->SetTranslucentSortPriority(GroundSortPriority);
	if (GroundShadow)
	{
		GroundShadow->SetVisibility(bShowGroundShadow, true);
		GroundShadow->SetHiddenInGame(!bShowGroundShadow);
		GroundShadow->SetTranslucentSortPriority(GroundSortPriority - 1);
	}
}

void AReEchoTimeShardPickupActor::CacheAuthoredPresentationTransform()
{
	if (!VisualRoot)
	{
		return;
	}
	AuthoredVisualRootLocation = VisualRoot->GetRelativeLocation();
	bPresentationTransformCached = true;
}

void AReEchoTimeShardPickupActor::UpdateLandingPresentation(const float DeltaSeconds)
{
	if (!bPresentationTransformCached || !VisualRoot)
	{
		return;
	}

	const float SafeDuration = FMath::Max(LandingBounceDurationSeconds, UE_SMALL_NUMBER);
	LandingAnimationElapsedSeconds = FMath::Min(LandingAnimationElapsedSeconds + DeltaSeconds, SafeDuration);
	const float Progress = LandingBounceDurationSeconds > 0.0f
	                           ? FMath::Clamp(LandingAnimationElapsedSeconds / SafeDuration, 0.0f, 1.0f)
	                           : 1.0f;
	const float Oscillations = static_cast<float>(FMath::Max(0, LandingBounceCount)) + 0.5f;
	const float HeightAlpha = (1.0f - Progress) * FMath::Abs(FMath::Cos(Progress * PI * Oscillations));
	VisualRoot->SetRelativeLocation(AuthoredVisualRootLocation +
	                                FVector(0.0f, 0.0f, LandingBounceHeightCm * HeightAlpha));
}

void AReEchoTimeShardPickupActor::BeginCollectionPresentation()
{
	if (!VisualRoot)
	{
		Destroy();
		return;
	}

	SetLifeSpan(0.0f);
	CollectionAnimationElapsedSeconds = 0.0f;
	CollectionStartVisualRootLocation = VisualRoot->GetRelativeLocation();
	if (GroundShadow)
	{
		GroundShadow->SetVisibility(false, true);
		GroundShadow->SetHiddenInGame(true);
	}
	if (RuntimePickupMaterial)
	{
		RuntimePickupMaterial->SetScalarParameterValue(TEXT("Opacity"), 1.0f);
	}
}

void AReEchoTimeShardPickupActor::BeginAttraction(AActor* Candidate)
{
	AReEchoPlayerPawn* Player = Cast<AReEchoPlayerPawn>(Candidate);
	if (bCollected || bAttracting || !Player)
	{
		return;
	}
	bAttracting = true;
	AttractionTarget = Player;
	UE_LOG(LogReEcho,
	       Verbose,
	       TEXT("[TimeShardPickup] attraction started actor=%s player=%s range=%.1f"),
	       *GetName(),
	       *Player->GetName(),
	       Collision ? Collision->GetScaledSphereRadius() : 0.0f);
}

bool AReEchoTimeShardPickupActor::TryResolveActiveArenaGroundPlane(float& OutGameplayPlaneZ) const
{
	const AReEchoGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AReEchoGameMode>() : nullptr;
	return GameMode && GameMode->TryGetActiveArenaGameplayPlaneZ(OutGameplayPlaneZ);
}

void AReEchoTimeShardPickupActor::UpdateAttraction(const float DeltaSeconds)
{
	AReEchoPlayerPawn* Player = AttractionTarget.Get();
	if (!Player)
	{
		Player = Cast<AReEchoPlayerPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
		AttractionTarget = Player;
	}
	if (!Player)
	{
		return;
	}

	FVector CurrentLocation = GetActorLocation();
	FVector TargetLocation = Player->GetActorLocation();
	float GameplayPlaneZ = CurrentLocation.Z;
	TryResolveActiveArenaGroundPlane(GameplayPlaneZ);
	CurrentLocation.Z = GameplayPlaneZ;
	TargetLocation.Z = GameplayPlaneZ;

	const float AttractionSpeed = ResolveAttractionSpeed(Player->GetVelocity().Size2D(),
	                                                      AttractionMinimumSpeedCmPerSecond,
	                                                      AttractionSpeedAdvantageCmPerSecond);
	const FVector NewLocation =
	    FMath::VInterpConstantTo(CurrentLocation, TargetLocation, DeltaSeconds, AttractionSpeed);
	SetActorLocation(NewLocation, false, nullptr, ETeleportType::None);
	if (FVector::DistSquared2D(NewLocation, TargetLocation) <= FMath::Square(AttractionCaptureRadiusCm))
	{
		TryCollect(Player);
	}
}

void AReEchoTimeShardPickupActor::UpdateCollectionPresentation(const float DeltaSeconds)
{
	if (!VisualRoot)
	{
		Destroy();
		return;
	}

	const float SafeDuration = FMath::Max(CollectionRiseDurationSeconds, UE_SMALL_NUMBER);
	CollectionAnimationElapsedSeconds = FMath::Min(CollectionAnimationElapsedSeconds + DeltaSeconds, SafeDuration);
	const float Progress = FMath::Clamp(CollectionAnimationElapsedSeconds / SafeDuration, 0.0f, 1.0f);
	const float RiseAlpha = 1.0f - FMath::Pow(1.0f - Progress, 3.0f);
	VisualRoot->SetRelativeLocation(CollectionStartVisualRootLocation +
	                                FVector(0.0f, 0.0f, CollectionRiseHeightCm * RiseAlpha));
	if (RuntimePickupMaterial)
	{
		RuntimePickupMaterial->SetScalarParameterValue(TEXT("Opacity"), 1.0f - FMath::SmoothStep(0.0f, 1.0f, Progress));
	}
	if (Progress >= 1.0f)
	{
		Destroy();
	}
}

void AReEchoTimeShardPickupActor::SnapToArenaGroundPlane()
{
	if (!bSnapToArenaGroundPlane)
	{
		return;
	}
	float GameplayPlaneZ = 0.0f;
	if (TryResolveActiveArenaGroundPlane(GameplayPlaneZ))
	{
		FVector GroundLocation = GetActorLocation();
		GroundLocation.Z = GameplayPlaneZ;
		SetActorLocation(GroundLocation, false, nullptr, ETeleportType::TeleportPhysics);
	}
}

void AReEchoTimeShardPickupActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bCollected)
	{
		UpdateCollectionPresentation(DeltaSeconds);
		return;
	}
	UpdateLandingPresentation(DeltaSeconds);

	if (bAttracting)
	{
		UpdateAttraction(DeltaSeconds);
		return;
	}

	AReEchoPlayerPawn* Player = Cast<AReEchoPlayerPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!Player)
	{
		return;
	}

	const FVector Offset = Player->GetActorLocation() - GetActorLocation();
	const float CollectionRadiusCm = Collision ? Collision->GetScaledSphereRadius() : 0.0f;
	if (Offset.SizeSquared2D() <= FMath::Square(CollectionRadiusCm) &&
	    FMath::Abs(Offset.Z) <= CollectionHeightToleranceCm)
	{
		BeginAttraction(Player);
	}
}

void AReEchoTimeShardPickupActor::InitializePickup(const int32 InAmount, const float LifetimeSeconds)
{
	Amount = FMath::Max(1, InAmount);
	bAttracting = false;
	bCollected = false;
	AttractionTarget.Reset();
	SetLifeSpan(FMath::Max(0.0f, LifetimeSeconds));
}

void AReEchoTimeShardPickupActor::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent,
                                                     AActor* OtherActor,
                                                     UPrimitiveComponent* OtherComponent,
                                                     const int32 OtherBodyIndex,
                                                     const bool bFromSweep,
                                                     const FHitResult& SweepResult)
{
	BeginAttraction(OtherActor);
}

void AReEchoTimeShardPickupActor::TryCollect(AActor* Collector)
{
	if (bCollected || !Cast<AReEchoPlayerPawn>(Collector))
	{
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	UReEchoRunSubsystem* RunSubsystem = GameInstance ? GameInstance->GetSubsystem<UReEchoRunSubsystem>() : nullptr;
	if (RunSubsystem && RunSubsystem->GrantTimeShards(Amount))
	{
		UE_LOG(LogReEcho,
		       Display,
		       TEXT("[TimeShardPickup] collected actor=%s amount=%d balance=%d"),
		       *GetName(),
		       Amount,
		       RunSubsystem->TimeShards);
		bCollected = true;
		Collision->SetGenerateOverlapEvents(false);
		Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (UReEchoAudioService* AudioService = GameInstance->GetSubsystem<UReEchoAudioService>())
		{
			FReEchoAudioEventRequest Request;
			Request.EventId = FReEchoAudioEvents::ItemPickup;
			Request.WorldLocation = GetActorLocation();
			Request.SourceCategory = EReEchoAudioSourceCategory::Environment;
			AudioService->PostEvent(this, Request);
		}
		BeginCollectionPresentation();
	}
}
