#include "Graybox/ReEchoTimeShardPickupActor.h"

#include "ReEcho.h"
#include "Components/BillboardComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "Player/ReEchoPlayerPawn.h"
#include "Run/ReEchoRunSubsystem.h"
#include "UObject/ConstructorHelpers.h"

AReEchoTimeShardPickupActor::AReEchoTimeShardPickupActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->InitSphereRadius(32.0f);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Collision->SetGenerateOverlapEvents(true);
	Collision->OnComponentBeginOverlap.AddDynamic(this, &AReEchoTimeShardPickupActor::HandleBeginOverlap);

	Visual = CreateDefaultSubobject<UBillboardComponent>(TEXT("TimeShardVisual"));
	Visual->SetupAttachment(Collision);
	Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Visual->SetCastShadow(false);
	Visual->SetTranslucentSortPriority(30);
	Visual->SetRelativeLocation(FVector(0.0f, 0.0f, 28.0f));
	Visual->SetHiddenInGame(false);
	Visual->SetVisibility(true);
	Visual->bIsScreenSizeScaled = false;
	static ConstructorHelpers::FObjectFinder<UTexture2D> TimeShardTexture(
	    TEXT("/Game/ReEcho/Textures/Pickups/T_TimeShard.T_TimeShard"));
	if (TimeShardTexture.Succeeded())
	{
		Visual->SetSprite(TimeShardTexture.Object);
		constexpr float PickupWorldHeight = 38.0f;
		Visual->SetRelativeScale3D(
		    FVector(PickupWorldHeight / FMath::Max(1, TimeShardTexture.Object->GetSizeY())));
	}

	SetLifeSpan(20.0f);
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
	if (bCollected || !Cast<AReEchoPlayerPawn>(OtherActor))
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
