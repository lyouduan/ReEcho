#pragma once

#include "CoreMinimal.h"
#include "Combat/ReEchoCombatTypes.h"

class AActor;
class UWorld;

namespace ReEchoWeaponGeometry
{
REECHOWEAPONS_API bool
IsInsideMeleeArc(const FVector& Origin, const FVector& Forward, const FVector& Target, float RangeCm, float ArcDegrees);
REECHOWEAPONS_API bool IsInsideMeleeSphere(const FVector& Origin, const FVector& Target, float RangeCm);
REECHOWEAPONS_API TArray<FVector>
BuildProjectileDirections(const FVector& Forward, int32 ProjectileCount, float SpreadDegrees);
REECHOWEAPONS_API TArray<AActor*> FindMeleeTargets(UWorld& World,
                                                   const FReEchoAttackIdentity& Attack,
                                                   const FVector& Origin,
                                                   const FVector& Forward,
                                                   float RangeCm,
                                                   float ArcDegrees);
REECHOWEAPONS_API TArray<AActor*>
FindMeleeTargetsInSphere(UWorld& World, const FReEchoAttackIdentity& Attack, const FVector& Origin, float RangeCm);
}
