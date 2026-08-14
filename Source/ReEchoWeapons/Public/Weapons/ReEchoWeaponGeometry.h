#pragma once

#include "CoreMinimal.h"

class AActor;
class UWorld;

namespace ReEchoWeaponGeometry
{
REECHOWEAPONS_API bool
IsInsideMeleeArc(const FVector& Origin, const FVector& Forward, const FVector& Target, float RangeCm, float ArcDegrees);
REECHOWEAPONS_API TArray<FVector>
BuildProjectileDirections(const FVector& Forward, int32 ProjectileCount, float SpreadDegrees);
REECHOWEAPONS_API TArray<AActor*> FindMeleeTargets(
    UWorld& World, AActor* Source, const FVector& Origin, const FVector& Forward, float RangeCm, float ArcDegrees);
}
