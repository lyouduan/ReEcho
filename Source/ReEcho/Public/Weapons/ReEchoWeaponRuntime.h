#pragma once

#include "CoreMinimal.h"
#include "Core/ReEchoTypes.h"
#include "Data/ReEchoCsvDataRegistry.h"
#include "Weapons/ReEchoWeaponTypes.h"

struct REECHO_API FReEchoEffectiveWeaponDefinition
{
	FReEchoCsvWeaponRow Weapon;
	TArray<FReEchoCsvAttackStepRow> AttackSteps;
	FName DamageChannelId = TEXT("Physical");
	bool bUsesCyclingElement = false;
	bool bUsesDeterministicRandomElement = false;
	float OnKillHealPercent = 0.0f;
};

namespace ReEchoWeaponRuntime
{
REECHO_API float ApplyValueOperation(float CurrentValue, EReEchoCsvValueOp ValueOp, float Value);
REECHO_API FString GetBuildConfigurationError(const FReEchoCsvDataSnapshot& Snapshot,
                                              const FReEchoBuildSnapshot& Build);
REECHO_API bool TryEquipParts(const FReEchoCsvDataSnapshot& Snapshot,
                              const FReEchoBuildSnapshot& Build,
                              const TArray<FName>& PartIds,
                              FReEchoBuildSnapshot& OutBuild,
                              FString& OutError);
REECHO_API int32 GetEffectiveSlotCapacity(const FReEchoCsvDataSnapshot& Snapshot,
                                          const FReEchoBuildSnapshot& Build,
                                          FName WeaponTypeId,
                                          FName SlotTypeId);
REECHO_API bool
TryGetEquipmentBaseBuild(const FReEchoBuildSnapshot& Build, FReEchoBuildSnapshot& OutBaseBuild, FString& OutError);
REECHO_API bool TrySelectWeapon(const FReEchoCsvDataSnapshot& Snapshot,
                                const FReEchoBuildSnapshot& Build,
                                FName WeaponId,
                                FReEchoBuildSnapshot& OutBuild,
                                FString& OutError);
REECHO_API bool BuildEffectiveWeaponDefinition(const FReEchoCsvDataSnapshot& Snapshot,
                                               const FReEchoBuildSnapshot& Build,
                                               FReEchoEffectiveWeaponDefinition& OutDefinition,
                                               FString& OutError);
/** Main-module data adapter: compile validated CSV rows into resource-free ReEchoWeapons runtime data. */
REECHO_API FReEchoWeaponDefinition CompileLogicDefinition(const FReEchoEffectiveWeaponDefinition& Definition);
REECHO_API bool
IsInsideMeleeArc(const FVector& Origin, const FVector& Forward, const FVector& Target, float RangeCm, float ArcDegrees);
REECHO_API TArray<FVector>
BuildProjectileDirections(const FVector& Forward, int32 ProjectileCount, float SpreadDegrees);
REECHO_API EReEchoElement ElementFromDamageChannel(FName DamageChannelId, int32 AttackSequence);
} // namespace ReEchoWeaponRuntime
