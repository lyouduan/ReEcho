#include "Weapons/ReEchoProjectileLogicComponent.h"

#include "Combat/ReEchoCombatTarget.h"
#include "Combat/ReEchoHitResolver.h"
#include "EngineUtils.h"

#if !UE_BUILD_SHIPPING
DEFINE_LOG_CATEGORY_STATIC(LogReEchoRangedCritProjectileTrace, Log, All);

namespace
{
bool IsRangedWeaponTrace(const FReEchoAttackIdentity& Attack)
{
	return Attack.WeaponId == TEXT("W_J_08") || Attack.WeaponId == TEXT("W_J_09");
}
} // namespace
#endif

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
	HitTargets.Reset();
	bActive = true;
	SetComponentTickEnabled(true);
	OnProjectileUpdated.Broadcast(GetSnapshot());
#if !UE_BUILD_SHIPPING
	if (IsRangedWeaponTrace(Spec.HitIntent.Attack))
	{
		UE_LOG(LogReEchoRangedCritProjectileTrace,
		       Warning,
		       TEXT("[RangedCritTrace] ProjectileInit actor=%s source=%s weapon=%s sequence=%lld rawDamage=%.3f "
		            "critical=%d pierceOnCritical=%d speed=%.3f radius=%.3f range=%.3f explosion=%.3f"),
		       *GetNameSafe(Owner),
		       *GetNameSafe(Spec.HitIntent.Attack.Source.Get()),
		       *Spec.HitIntent.Attack.WeaponId.ToString(),
		       static_cast<long long>(Spec.HitIntent.Attack.Sequence),
		       Spec.HitIntent.RawDamage,
		       Spec.HitIntent.bCritical ? 1 : 0,
		       Spec.bPierceOnCritical ? 1 : 0,
		       Spec.SpeedCmPerSecond,
		       Spec.CarrierRadiusCm,
		       Spec.MaximumRangeCm,
		       Spec.ExplosionRadiusCm);
	}
#endif
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
		if (!Target || !Target->IsCombatTargetAlive() || HitTargets.Contains(Candidate) ||
		    !ReEchoCombatRelations::CanDamage(
		        Spec.HitIntent.Attack, *Candidate, Spec.HitIntent.bAllowSameFactionDamage))
		{
			continue;
		}
		if (Target->IntersectsCombatPath(PreviousLocation, NewLocation, Spec.CarrierRadiusCm))
		{
#if !UE_BUILD_SHIPPING
			if (IsRangedWeaponTrace(Spec.HitIntent.Attack))
			{
				UE_LOG(
				    LogReEchoRangedCritProjectileTrace,
				    Warning,
				    TEXT(
				        "[RangedCritTrace] ProjectileContact actor=%s weapon=%s sequence=%lld target=%s rawDamage=%.3f "
				        "critical=%d travelled=%.3f pathStart=%s pathEnd=%s"),
				    *GetNameSafe(Owner),
				    *Spec.HitIntent.Attack.WeaponId.ToString(),
				    static_cast<long long>(Spec.HitIntent.Attack.Sequence),
				    *GetNameSafe(Candidate),
				    Spec.HitIntent.RawDamage,
				    Spec.HitIntent.bCritical ? 1 : 0,
				    TravelledCm,
				    *PreviousLocation.ToCompactString(),
				    *NewLocation.ToCompactString());
			}
#endif
			const FReEchoHitResolved DirectResult = ApplyAtLocation(Target->GetCombatTargetLocation(), Candidate);
			if (!Spec.bPierceOnCritical || !DirectResult.bCritical || DirectResult.AppliedDamage <= 0.0f)
			{
				Expire();
			}
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
	const FReEchoHitResolved Result = ReEchoHitResolver::ResolveHit(Intent);
#if !UE_BUILD_SHIPPING
	if (IsRangedWeaponTrace(Intent.Attack))
	{
		UE_LOG(LogReEchoRangedCritProjectileTrace,
		       Warning,
		       TEXT("[RangedCritTrace] ProjectileResolve actor=%s weapon=%s sequence=%lld source=%s target=%s "
		            "intentRaw=%.3f intentCritical=%d resolvedRaw=%.3f applied=%.3f resultCritical=%d blocked=%d "
		            "killed=%d"),
		       *GetNameSafe(GetOwner()),
		       *Intent.Attack.WeaponId.ToString(),
		       static_cast<long long>(Intent.Attack.Sequence),
		       *GetNameSafe(Intent.Attack.Source.Get()),
		       *GetNameSafe(Target),
		       Intent.RawDamage,
		       Intent.bCritical ? 1 : 0,
		       Result.RawDamage,
		       Result.AppliedDamage,
		       Result.bCritical ? 1 : 0,
		       Result.bBlocked ? 1 : 0,
		       Result.bKilled ? 1 : 0);
	}
#endif
	return Result;
}

FReEchoHitResolved UReEchoProjectileLogicComponent::ApplyAtLocation(const FVector& ImpactLocation, AActor* DirectTarget)
{
	FReEchoHitResolved DirectResult;
	auto Apply = [&](AActor* Candidate)
	{
		IReEchoCombatTarget* Target = Cast<IReEchoCombatTarget>(Candidate);
		if (!Target || !Target->IsCombatTargetAlive() || HitTargets.Contains(Candidate) ||
		    !ReEchoCombatRelations::CanDamage(
		        Spec.HitIntent.Attack, *Candidate, Spec.HitIntent.bAllowSameFactionDamage))
		{
			return FReEchoHitResolved{};
		}
		HitTargets.Add(Candidate);
		const FReEchoHitResolved Result = ResolveIntent(Candidate, Target->GetCombatTargetLocation());
		OnProjectileImpacted.Broadcast(GetSnapshot(), Result);
		return Result;
	};

	if (Spec.ExplosionRadiusCm <= 0.0f)
	{
		return Apply(DirectTarget);
	}
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		IReEchoCombatTarget* Target = Cast<IReEchoCombatTarget>(*It);
		if (Target && FVector::Dist2D(ImpactLocation, Target->GetCombatTargetLocation()) <= Spec.ExplosionRadiusCm)
		{
			const FReEchoHitResolved Result = Apply(*It);
			if (*It == DirectTarget)
			{
				DirectResult = Result;
			}
		}
	}
	return DirectResult;
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
