#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ReEchoTimeShardPickupActor.generated.h"

class UPrimitiveComponent;
class UBillboardComponent;
class USphereComponent;

/** Shared world pickup for configured enemy drops and weapon-rune shard rewards. */
UCLASS()

class REECHO_API AReEchoTimeShardPickupActor : public AActor
{
	GENERATED_BODY()

public:
	AReEchoTimeShardPickupActor();
	virtual void Tick(float DeltaSeconds) override;

	/** Lifetime <= 0 keeps the pickup until collection or explicit world cleanup. */
	void InitializePickup(int32 InAmount, float LifetimeSeconds = 20.0f);

	int32 GetAmount() const
	{
		return Amount;
	}

	UBillboardComponent* GetVisualComponent() const
	{
		return Visual;
	}

private:
	void TryCollect(AActor* Collector);

	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent,
	                        AActor* OtherActor,
	                        UPrimitiveComponent* OtherComponent,
	                        int32 OtherBodyIndex,
	                        bool bFromSweep,
	                        const FHitResult& SweepResult);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBillboardComponent> Visual;

	int32 Amount = 1;
	bool bCollected = false;
};
