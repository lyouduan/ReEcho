#include "Graybox/ReEchoTimeShardPickupActor.h"

#include "ReEcho.h"
#include "Components/MaterialBillboardComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Player/ReEchoPlayerPawn.h"
#include "Presentation/Scene/ReEchoArenaSceneActor.h"
#include "Run/ReEchoRunSubsystem.h"
#include "UObject/ConstructorHelpers.h"

namespace ReEchoTimeShardPickup
{
constexpr float CollisionRadiusCm = 48.0f;
} // namespace ReEchoTimeShardPickup

AReEchoTimeShardPickupActor::AReEchoTimeShardPickupActor()
{
	PrimaryActorTick.bCanEverTick = true;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->InitSphereRadius(ReEchoTimeShardPickup::CollisionRadiusCm);
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
	ApplyEditablePresentationSettings();
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
	Element.Material = PickupMaterial;
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

void AReEchoTimeShardPickupActor::SnapToArenaGroundPlane()
{
	if (!bSnapToArenaGroundPlane || !GetWorld())
	{
		return;
	}
	for (TActorIterator<AReEchoArenaSceneActor> It(GetWorld()); It; ++It)
	{
		FVector GroundLocation = GetActorLocation();
		GroundLocation.Z = It->GetGameplayPlaneWorldZ();
		SetActorLocation(GroundLocation, false, nullptr, ETeleportType::TeleportPhysics);
		return;
	}
}

void AReEchoTimeShardPickupActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bCollected)
	{
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
		TryCollect(Player);
	}
}

void AReEchoTimeShardPickupActor::InitializePickup(const int32 InAmount, const float LifetimeSeconds)
{
	Amount = FMath::Max(1, InAmount);
	bCollected = false;
	SetLifeSpan(FMath::Max(0.0f, LifetimeSeconds));
}

void AReEchoTimeShardPickupActor::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent,
                                                     AActor* OtherActor,
                                                     UPrimitiveComponent* OtherComponent,
                                                     const int32 OtherBodyIndex,
                                                     const bool bFromSweep,
                                                     const FHitResult& SweepResult)
{
	TryCollect(OtherActor);
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
		Destroy();
	}
}
