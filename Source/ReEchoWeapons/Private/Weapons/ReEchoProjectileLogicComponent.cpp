#include "Weapons/ReEchoProjectileLogicComponent.h"

#include "Combat/ReEchoCombatTarget.h"
#include "Combat/ReEchoHitResolver.h"
#include "EngineUtils.h"

UReEchoProjectileLogicComponent::UReEchoProjectileLogicComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

bool UReEchoProjectileLogicComponent::InitializeProjectile(const FReEchoLogicalProjectileSpec& InSpec)
{
	AActor* Owner = GetOwner();
	if (!Owner || !InSpec.HitIntent.Attack.IsValid() || InSpec.SpeedCmPerSecond <= 0.0f ||
	    InSpec.CarrierRadiusCm <= 0.0f)
	{
		return false;
	}
	Spec = InSpec;
	ProjectileId.Value = FGuid::NewGuid();
	SpawnLocation = Owner->GetActorLocation();
	const FVector Direction = Spec.Direction.GetSafeNormal();
	Velocity = (Direction.IsNearlyZero() ? FVector::ForwardVector : Direction) * Spec.SpeedCmPerSecond;
	TravelledCm = 0.0f;
	bActive = true;
	SetComponentTickEnabled(true);
	OnProjectileUpdated.Broadcast(GetSnapshot());
	return true;
}

FReEchoProjectileSnapshot UReEchoProjectileLogicComponent::GetSnapshot() const
{
	FReEchoProjectileSnapshot Snapshot;
	Snapshot.ProjectileId = ProjectileId;
	Snapshot.Attack = Spec.HitIntent.Attack;
	Snapshot.Position = GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;
	Snapshot.Velocity = Velocity;
	Snapshot.TravelledCm = TravelledCm;
	Snapshot.MaximumRangeCm = Spec.MaximumRangeCm;
	Snapshot.bActive = bActive;
	return Snapshot;
}

void UReEchoProjectileLogicComponent::TickComponent(const float DeltaTime,
                                                    const ELevelTick TickType,
                                                    FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	Advance(DeltaTime);
}

#if WITH_DEV_AUTOMATION_TESTS
void UReEchoProjectileLogicComponent::AdvanceSimulation(const float DeltaTime)
{
	Advance(DeltaTime);
}
#endif

void UReEchoProjectileLogicComponent::Advance(const float DeltaTime)
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!bActive || !Owner || !World)
	{
		return;
	}

	const FVector PreviousLocation = Owner->GetActorLocation();
	const FVector NewLocation = PreviousLocation + Velocity * FMath::Max(0.0f, DeltaTime);
	Owner->SetActorLocation(NewLocation);
	TravelledCm += FVector::Dist(PreviousLocation, NewLocation);

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Candidate = *It;
		IReEchoCombatTarget* Target = Cast<IReEchoCombatTarget>(Candidate);
		if (!Target || Candidate == Spec.HitIntent.Attack.Source || !Target->IsCombatTargetAlive())
		{
			continue;
		}
		if (Target->IntersectsCombatPath(PreviousLocation, NewLocation, Spec.CarrierRadiusCm))
		{
			ApplyAtLocation(Target->GetCombatTargetLocation(), Candidate);
			Expire();
			return;
		}
	}

	OnProjectileUpdated.Broadcast(GetSnapshot());
	if (Spec.MaximumRangeCm > 0.0f && TravelledCm >= Spec.MaximumRangeCm)
	{
		ApplyAtLocation(NewLocation, nullptr);
		Expire();
	}
}

FReEchoHitResolved UReEchoProjectileLogicComponent::ResolveIntent(AActor* Target, const FVector& HitLocation) const
{
	FReEchoHitIntent Intent = Spec.HitIntent;
	Intent.Target = Target;
	Intent.HitLocation = HitLocation;
	return ReEchoHitResolver::ResolveHit(Intent);
}

void UReEchoProjectileLogicComponent::ApplyAtLocation(const FVector& ImpactLocation, AActor* DirectTarget)
{
	TSet<TWeakObjectPtr<AActor>> AppliedTargets;
	auto Apply = [&](AActor* Candidate)
	{
		IReEchoCombatTarget* Target = Cast<IReEchoCombatTarget>(Candidate);
		if (!Target || !Target->IsCombatTargetAlive() || AppliedTargets.Contains(Candidate))
		{
			return;
		}
		AppliedTargets.Add(Candidate);
		const FReEchoHitResolved Result = ResolveIntent(Candidate, Target->GetCombatTargetLocation());
		OnProjectileImpacted.Broadcast(GetSnapshot(), Result);
	};

	if (Spec.ExplosionRadiusCm <= 0.0f)
	{
		Apply(DirectTarget);
		return;
	}
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		IReEchoCombatTarget* Target = Cast<IReEchoCombatTarget>(*It);
		if (Target && FVector::Dist2D(ImpactLocation, Target->GetCombatTargetLocation()) <= Spec.ExplosionRadiusCm)
		{
			Apply(*It);
		}
	}
}

void UReEchoProjectileLogicComponent::Expire()
{
	if (!bActive)
	{
		return;
	}
	bActive = false;
	SetComponentTickEnabled(false);
	OnProjectileExpired.Broadcast(GetSnapshot());
	if (AActor* Owner = GetOwner())
	{
		Owner->Destroy();
	}
}
