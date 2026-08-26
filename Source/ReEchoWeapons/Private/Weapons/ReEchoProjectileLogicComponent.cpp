#include "Weapons/ReEchoProjectileLogicComponent.h"

#include "Combat/ReEchoCombatTarget.h"
#include "Combat/ReEchoCombatantComponent.h"
#include "Combat/ReEchoHitResolver.h"
#include "Components/PrimitiveComponent.h"
#include "EngineUtils.h"

#if !UE_BUILD_SHIPPING
DEFINE_LOG_CATEGORY_STATIC(LogReEchoRangedCritProjectileTrace, Log, All);
DEFINE_LOG_CATEGORY_STATIC(LogReEchoSpawnDamageTrace, Log, All);

namespace
{
bool IsRangedWeaponTrace(const FReEchoAttackIdentity& Attack)
{
	return Attack.WeaponId == TEXT("W_J_08") || Attack.WeaponId == TEXT("W_J_09");
}

bool IsPlayerProjectileTrace(const FReEchoHitIntent& Intent)
{
	return Intent.DamageSource == EReEchoDamageSource::Player;
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
#if !UE_BUILD_SHIPPING
	DiagnosticLoggedNearTargets.Reset();
	DiagnosticClosestTarget.Reset();
	DiagnosticClosestPathDistanceCm = TNumericLimits<float>::Max();
#endif
	bActive = true;
	SetComponentTickEnabled(true);
	OnProjectileUpdated.Broadcast(GetSnapshot());
#if !UE_BUILD_SHIPPING
	if (IsPlayerProjectileTrace(Spec.HitIntent))
	{
		UE_LOG(LogReEchoSpawnDamageTrace,
		       Warning,
		       TEXT("[SpawnDamageTrace] ProjectileInit world=%.3f projectile=%s commit=%s#%lld weapon=%s "
		            "position=%s velocity=%s radius=%.3f rawDamage=%.3f range=%.3f"),
		       GetWorld() ? GetWorld()->GetTimeSeconds() : -1.0f,
		       *ProjectileId.Value.ToString(EGuidFormats::DigitsWithHyphensLower),
		       *GetNameSafe(Spec.HitIntent.Attack.Source.Get()),
		       static_cast<long long>(Spec.HitIntent.Attack.Sequence),
		       *Spec.HitIntent.Attack.WeaponId.ToString(),
		       *SpawnLocation.ToCompactString(),
		       *Velocity.ToCompactString(),
		       Spec.CarrierRadiusCm,
		       Spec.HitIntent.RawDamage,
		       Spec.MaximumRangeCm);
	}
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
		if (!Target)
		{
			continue;
		}
		const bool bTargetAlive = Target->IsCombatTargetAlive();
		const bool bAlreadyHit = HitTargets.Contains(Candidate);
		const bool bRelationAllowsDamage = ReEchoCombatRelations::CanDamage(
		    Spec.HitIntent.Attack, *Candidate, Spec.HitIntent.bAllowSameFactionDamage);
		const FVector TargetLocation = Target->GetCombatTargetLocation();
		const float PathDistanceCm = FMath::PointDistToSegment(TargetLocation, PreviousLocation, NewLocation);
		const bool bIntersects = Target->IntersectsCombatPath(PreviousLocation, NewLocation, Spec.CarrierRadiusCm);
#if !UE_BUILD_SHIPPING
		if (IsPlayerProjectileTrace(Spec.HitIntent) && PathDistanceCm < DiagnosticClosestPathDistanceCm)
		{
			DiagnosticClosestPathDistanceCm = PathDistanceCm;
			DiagnosticClosestTarget = Candidate;
		}
		if (IsPlayerProjectileTrace(Spec.HitIntent) &&
		    (bIntersects || PathDistanceCm <= Spec.CarrierRadiusCm + 50.0f) &&
		    !DiagnosticLoggedNearTargets.Contains(Candidate))
		{
			DiagnosticLoggedNearTargets.Add(Candidate);
			const UReEchoCombatantComponent* Combatant = Target->GetCombatTargetCombatant();
			const UPrimitiveComponent* RootCollision = Cast<UPrimitiveComponent>(Candidate->GetRootComponent());
			UE_LOG(LogReEchoSpawnDamageTrace,
			       Warning,
			       TEXT("[SpawnDamageTrace] Candidate world=%.3f projectile=%s commit=%s#%lld target=%s "
			            "pathDistance=%.3f intersects=%d alive=%d damageable=%d relation=%d alreadyHit=%d "
			            "actorCollision=%d rootCollision=%d health=%.3f pathStart=%s pathEnd=%s targetLocation=%s"),
			       World->GetTimeSeconds(),
			       *ProjectileId.Value.ToString(EGuidFormats::DigitsWithHyphensLower),
			       *GetNameSafe(Spec.HitIntent.Attack.Source.Get()),
			       static_cast<long long>(Spec.HitIntent.Attack.Sequence),
			       *GetNameSafe(Candidate),
			       PathDistanceCm,
			       bIntersects ? 1 : 0,
			       bTargetAlive ? 1 : 0,
			       Candidate->CanBeDamaged() ? 1 : 0,
			       bRelationAllowsDamage ? 1 : 0,
			       bAlreadyHit ? 1 : 0,
			       Candidate->GetActorEnableCollision() ? 1 : 0,
			       RootCollision && RootCollision->IsCollisionEnabled() ? 1 : 0,
			       Combatant ? Combatant->GetSnapshot().CurrentHealth : -1.0f,
			       *PreviousLocation.ToCompactString(),
			       *NewLocation.ToCompactString(),
			       *TargetLocation.ToCompactString());
		}
#endif
		if (!bTargetAlive || bAlreadyHit || !bRelationAllowsDamage)
		{
			continue;
		}
		if (bIntersects)
		{
#if !UE_BUILD_SHIPPING
			if (IsPlayerProjectileTrace(Spec.HitIntent))
			{
				UE_LOG(LogReEchoSpawnDamageTrace,
				       Warning,
				       TEXT("[SpawnDamageTrace] Contact world=%.3f projectile=%s commit=%s#%lld target=%s "
				            "travelled=%.3f pathDistance=%.3f"),
				       World->GetTimeSeconds(),
				       *ProjectileId.Value.ToString(EGuidFormats::DigitsWithHyphensLower),
				       *GetNameSafe(Spec.HitIntent.Attack.Source.Get()),
				       static_cast<long long>(Spec.HitIntent.Attack.Sequence),
				       *GetNameSafe(Candidate),
				       TravelledCm,
				       PathDistanceCm);
			}
#endif
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
	if (IsPlayerProjectileTrace(Intent))
	{
		UE_LOG(LogReEchoSpawnDamageTrace,
		       Warning,
		       TEXT("[SpawnDamageTrace] Resolve world=%.3f projectile=%s commit=%s#%lld target=%s raw=%.3f "
		            "applied=%.3f blocked=%d killed=%d"),
		       GetWorld() ? GetWorld()->GetTimeSeconds() : -1.0f,
		       *ProjectileId.Value.ToString(EGuidFormats::DigitsWithHyphensLower),
		       *GetNameSafe(Intent.Attack.Source.Get()),
		       static_cast<long long>(Intent.Attack.Sequence),
		       *GetNameSafe(Target),
		       Result.RawDamage,
		       Result.AppliedDamage,
		       Result.bBlocked ? 1 : 0,
		       Result.bKilled ? 1 : 0);
	}
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
#if !UE_BUILD_SHIPPING
	if (IsPlayerProjectileTrace(Spec.HitIntent))
	{
		UE_LOG(LogReEchoSpawnDamageTrace,
		       Warning,
		       TEXT("[SpawnDamageTrace] ProjectileEnd world=%.3f projectile=%s commit=%s#%lld travelled=%.3f "
		            "closestTarget=%s closestPathDistance=%.3f hitTargets=%d"),
		       GetWorld() ? GetWorld()->GetTimeSeconds() : -1.0f,
		       *ProjectileId.Value.ToString(EGuidFormats::DigitsWithHyphensLower),
		       *GetNameSafe(Spec.HitIntent.Attack.Source.Get()),
		       static_cast<long long>(Spec.HitIntent.Attack.Sequence),
		       TravelledCm,
		       *GetNameSafe(DiagnosticClosestTarget.Get()),
		       DiagnosticClosestPathDistanceCm,
		       HitTargets.Num());
	}
#endif
	OnProjectileExpired.Broadcast(GetSnapshot());
	if (AActor* Owner = GetOwner())
	{
		Owner->Destroy();
	}
}
