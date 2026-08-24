#include "Graybox/ReEchoTimeShardPickupActor.h"

#include "ReEcho.h"
#include "Components/MaterialBillboardComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Player/ReEchoPlayerPawn.h"
#include "Run/ReEchoRunSubsystem.h"
#include "UObject/ConstructorHelpers.h"

namespace ReEchoTimeShardPickup
{
constexpr float CollisionRadiusCm = 48.0f;
constexpr float CollectionHeightToleranceCm = 100.0f;
// Arena backdrop is -100 and character footpoint sorting starts at -50. Keep pickups between both bands.
constexpr int32 GroundPickupSortPriority = -60;
constexpr float VisualWorldHeightCm = 76.0f;
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

	Visual = CreateDefaultSubobject<UMaterialBillboardComponent>(TEXT("TimeShardVisual"));
	Visual->SetupAttachment(Collision);
	Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Visual->SetCastShadow(false);
	Visual->SetTranslucentSortPriority(ReEchoTimeShardPickup::GroundPickupSortPriority);
	Visual->SetRelativeLocation(FVector::ZeroVector);
	Visual->SetHiddenInGame(false);
	Visual->SetVisibility(true);
	static ConstructorHelpers::FObjectFinder<UTexture2D> TimeShardTexture(
	    TEXT("/Game/ReEcho/Textures/Pickups/T_TimeShard.T_TimeShard"));
	PickupTexture = TimeShardTexture.Object;
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> TranslucentSpriteMaterial(
	    TEXT("/Paper2D/TranslucentUnlitSpriteMaterial.TranslucentUnlitSpriteMaterial"));
	PickupBaseMaterial = TranslucentSpriteMaterial.Object;

	SetLifeSpan(20.0f);
}

void AReEchoTimeShardPickupActor::BeginPlay()
{
	Super::BeginPlay();
	if (!Visual || !PickupTexture || !PickupBaseMaterial)
	{
		UE_LOG(LogReEcho,
		       Error,
		       TEXT("[TimeShardPickup] missing visual dependency actor=%s texture=%s material=%s"),
		       *GetName(),
		       *GetNameSafe(PickupTexture),
		       *GetNameSafe(PickupBaseMaterial));
		return;
	}

	PickupMaterialInstance = UMaterialInstanceDynamic::Create(PickupBaseMaterial, this);
	PickupMaterialInstance->SetTextureParameterValue(TEXT("SpriteTexture"), PickupTexture);
	const float VisualWorldWidthCm = ReEchoTimeShardPickup::VisualWorldHeightCm *
	                                 static_cast<float>(PickupTexture->GetSizeX()) /
	                                 FMath::Max(1, PickupTexture->GetSizeY());
	Visual->AddElement(PickupMaterialInstance,
	                   nullptr,
	                   false,
	                   VisualWorldWidthCm,
	                   ReEchoTimeShardPickup::VisualWorldHeightCm,
	                   nullptr);
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
	if (Offset.SizeSquared2D() <= FMath::Square(ReEchoTimeShardPickup::CollisionRadiusCm) &&
	    FMath::Abs(Offset.Z) <= ReEchoTimeShardPickup::CollectionHeightToleranceCm)
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
