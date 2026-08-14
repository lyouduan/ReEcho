#pragma once

#include "Components/ActorComponent.h"
#include "Weapons/ReEchoWeaponTypes.h"
#include "ReEchoProjectileLogicComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FReEchoProjectileSnapshotEvent, const FReEchoProjectileSnapshot&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FReEchoProjectileImpactEvent,
                                     const FReEchoProjectileSnapshot&,
                                     const FReEchoHitResolved&);

/** Presentation-free projectile/wave movement, target detection, lifetime, and hit-intent production. */
UCLASS(ClassGroup = (ReEcho), meta = (BlueprintSpawnableComponent))

class REECHOWEAPONS_API UReEchoProjectileLogicComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UReEchoProjectileLogicComponent();

	bool InitializeProjectile(const FReEchoLogicalProjectileSpec& InSpec);
	FReEchoProjectileSnapshot GetSnapshot() const;
#if WITH_DEV_AUTOMATION_TESTS
	void AdvanceSimulation(float DeltaTime);
#endif

	FReEchoProjectileSnapshotEvent OnProjectileUpdated;
	FReEchoProjectileImpactEvent OnProjectileImpacted;
	FReEchoProjectileSnapshotEvent OnProjectileExpired;

protected:
	virtual void
	TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void Advance(float DeltaTime);
	FReEchoHitResolved ResolveIntent(AActor* Target, const FVector& HitLocation) const;
	void ApplyAtLocation(const FVector& ImpactLocation, AActor* DirectTarget);
	void Expire();

	FReEchoLogicalProjectileSpec Spec;
	FReEchoProjectileId ProjectileId;
	FVector SpawnLocation = FVector::ZeroVector;
	FVector Velocity = FVector::ZeroVector;
	float TravelledCm = 0.0f;
	bool bActive = false;
};
