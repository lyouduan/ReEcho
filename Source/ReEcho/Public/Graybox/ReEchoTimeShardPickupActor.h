#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ReEchoTimeShardPickupActor.generated.h"

class UPrimitiveComponent;
class UMaterialBillboardComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class USphereComponent;
class UTexture2D;

/** Shared world pickup for configured enemy drops and weapon-rune shard rewards. */
UCLASS()

class REECHO_API AReEchoTimeShardPickupActor : public AActor
{
	GENERATED_BODY()

public:
	AReEchoTimeShardPickupActor();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Lifetime <= 0 keeps the pickup until collection or explicit world cleanup. */
	void InitializePickup(int32 InAmount, float LifetimeSeconds = 20.0f);

	int32 GetAmount() const
	{
		return Amount;
	}

	UMaterialBillboardComponent* GetVisualComponent() const
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
	TObjectPtr<UMaterialBillboardComponent> Visual;

	UPROPERTY()
	TObjectPtr<UTexture2D> PickupTexture;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> PickupBaseMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> PickupMaterialInstance;

	int32 Amount = 1;
	bool bCollected = false;
};
