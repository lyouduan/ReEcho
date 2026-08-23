#include "Graybox/ReEchoTimeShardPickupActor.h"

#include "Components/SphereComponent.h"
#include "Engine/GameInstance.h"
#include "Player/ReEchoPlayerPawn.h"
#include "Run/ReEchoRunSubsystem.h"

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

	SetLifeSpan(20.0f);
}

void AReEchoTimeShardPickupActor::InitializePickup(const int32 InAmount)
{
	Amount = FMath::Max(1, InAmount);
}

void AReEchoTimeShardPickupActor::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent,
                                                     AActor* OtherActor,
                                                     UPrimitiveComponent* OtherComponent,
                                                     const int32 OtherBodyIndex,
                                                     const bool bFromSweep,
                                                     const FHitResult& SweepResult)
{
	if (!Cast<AReEchoPlayerPawn>(OtherActor))
	{
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	UReEchoRunSubsystem* RunSubsystem = GameInstance ? GameInstance->GetSubsystem<UReEchoRunSubsystem>() : nullptr;
	if (RunSubsystem && RunSubsystem->GrantTimeShards(Amount))
	{
		Destroy();
	}
}
