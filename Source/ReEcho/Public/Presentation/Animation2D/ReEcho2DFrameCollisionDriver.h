#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Presentation/Animation2D/ReEcho2DFrameCollisionTrack.h"
#include "ReEcho2DFrameCollisionDriver.generated.h"

class UReEcho2DAnimationComponent;

/** Immutable-for-the-frame query snapshot produced from authored collision geometry. */
USTRUCT(BlueprintType)
struct REECHO_API FReEcho2DFrameCollisionSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TArray<FReEcho2DCollisionPolygon> BodyHurtboxes;

	UPROPERTY(BlueprintReadOnly)
	TArray<FReEcho2DCollisionPolygon> WeaponAttackHitboxes;

	UPROPERTY(BlueprintReadOnly)
	int32 FrameIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly)
	int64 AttackInstanceId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly)
	bool bAttackActive = false;

	void Reset();
};

/**
 * Presentation collision clock. It mirrors authored polygons and exposes Query-only snapshots;
 * movement blocking and gameplay damage remain owned by the existing Capsule/weapon systems.
 */
UCLASS(ClassGroup = (ReEcho), meta = (BlueprintSpawnableComponent))
class REECHO_API UReEcho2DFrameCollisionDriver : public UActorComponent
{
	GENERATED_BODY()

public:
	UReEcho2DFrameCollisionDriver();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	void BindRenderer(UReEcho2DAnimationComponent* InRenderer);
	void BeginAttackInstance(int64 InAttackInstanceId);
	void EndAttackInstance(int64 InAttackInstanceId);
	void RefreshSnapshot();

	const FReEcho2DFrameCollisionSnapshot& GetSnapshot() const { return Snapshot; }
	bool HasValidTrack() const { return bTrackValid; }
	const FString& GetFallbackReason() const { return FallbackReason; }
	bool IsLocalPointInsideBody(FVector2D LocalPoint) const;
	bool IsLocalPointInsideActiveAttack(FVector2D LocalPoint, int64 AttackInstanceId) const;

private:
	static FReEcho2DCollisionPolygon TransformPolygon(const FReEcho2DCollisionPolygon& Source,
	                                                  const UReEcho2DFrameCollisionTrack& Track,
	                                                  float FacingSign);
	static bool ContainsPoint(const FReEcho2DCollisionPolygon& Polygon, FVector2D Point);
	void Invalidate(FString Reason);

	UPROPERTY()
	TObjectPtr<UReEcho2DAnimationComponent> Renderer;

	FReEcho2DFrameCollisionSnapshot Snapshot;
	TWeakObjectPtr<const UReEcho2DFrameCollisionTrack> ValidatedTrack;
	FString FallbackReason;
	int64 CommittedAttackInstanceId = INDEX_NONE;
	bool bTrackValid = false;
};
