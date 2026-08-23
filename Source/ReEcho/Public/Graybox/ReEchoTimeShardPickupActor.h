#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ReEchoTimeShardPickupActor.generated.h"

class UPrimitiveComponent;
class USphereComponent;

/** Lightweight world pickup spawned by weapon-rune shard rewards. */
UCLASS()

class REECHO_API AReEchoTimeShardPickupActor : public AActor
{
	GENERATED_BODY()

public:
	AReEchoTimeShardPickupActor();

	void InitializePickup(int32 InAmount);

private:
	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent,
	                        AActor* OtherActor,
	                        UPrimitiveComponent* OtherComponent,
	                        int32 OtherBodyIndex,
	                        bool bFromSweep,
	                        const FHitResult& SweepResult);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> Collision;

	int32 Amount = 1;
};
