#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ReEchoTimeShardPickupActor.generated.h"

class UPrimitiveComponent;
class UMaterialBillboardComponent;
class UMaterialInterface;
class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;
class UTexture2D;

/** Shared world pickup for configured enemy drops and weapon-rune shard rewards. */
UCLASS()

class REECHO_API AReEchoTimeShardPickupActor : public AActor
{
	GENERATED_BODY()

public:
	AReEchoTimeShardPickupActor();
	virtual void OnConstruction(const FTransform& Transform) override;
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

	UStaticMeshComponent* GetGroundShadowComponent() const
	{
		return GroundShadow;
	}

	float GetVisualWorldHeightCm() const
	{
		return VisualWorldHeightCm;
	}

private:
	void ApplyEditablePresentationSettings();
	void SnapToArenaGroundPlane();
	void TryCollect(AActor* Collector);

	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent,
	                        AActor* OtherActor,
	                        UPrimitiveComponent* OtherComponent,
	                        int32 OtherBodyIndex,
	                        bool bFromSweep,
	                        const FHitResult& SweepResult);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Time Shard|Collision", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Time Shard|Presentation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> PresentationRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Time Shard|Presentation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> VisualRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Time Shard|Presentation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> GroundRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Time Shard|Presentation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialBillboardComponent> Visual;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Time Shard|Presentation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> GroundShadow;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Time Shard|Presentation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTexture2D> PickupTexture;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Time Shard|Presentation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialInterface> PickupMaterial;

	/** Editor-facing icon height. Width follows the reviewed texture aspect ratio. */
	UPROPERTY(EditDefaultsOnly,
	          BlueprintReadOnly,
	          Category = "Time Shard|Presentation",
	          meta = (AllowPrivateAccess = "true", ClampMin = "1.0", UIMin = "1.0", Units = "cm"))
	float VisualWorldHeightCm = 76.0f;

	/** Keep this between the arena backdrop (-100) and character footpoint range (-50..50). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Time Shard|Presentation", meta = (AllowPrivateAccess = "true"))
	int32 GroundSortPriority = -60;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Time Shard|Presentation", meta = (AllowPrivateAccess = "true"))
	bool bShowGroundShadow = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Time Shard|Placement", meta = (AllowPrivateAccess = "true"))
	bool bSnapToArenaGroundPlane = true;

	UPROPERTY(EditDefaultsOnly,
	          BlueprintReadOnly,
	          Category = "Time Shard|Collision",
	          meta = (AllowPrivateAccess = "true", ClampMin = "0.0", UIMin = "0.0", Units = "cm"))
	float CollectionHeightToleranceCm = 100.0f;

	int32 Amount = 1;
	bool bCollected = false;
};
