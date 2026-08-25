#pragma once

#include "CoreMinimal.h"

enum class EReEchoCombatVfxSemantic : uint8
{
	RabbitCharging,
	RabbitProjectile,
	PlayerHurt,
	FoxCharging,
	FoxDirection,
	FoxDash,
	FoxImpact,
	PlayerMeleeSlash,
	PlayerScytheSlash,
	PlayerBowFlight,
	PlayerBowImpact,
	PlayerGunFlight,
	PlayerGunImpact,
	EnemyHurt,
	EchoWaterAura,
	EchoGrassAura,
	GoatSkill02Charging,
	GoatSkill02Bullet,
	GoatSkill02Impact,
	GoatSkill03Charging,
	GoatSkill03Alarming,
	GoatSkill03Impact,
	GoatSkill04Charging,
	GoatSkill04Lighting
};

enum class EReEchoVfxScalePolicy : uint8
{
	InheritAttachment,
	PreserveWorldSize
};

struct FReEchoVfxPlacement
{
	FVector LocalOffset = FVector::ZeroVector;
	FRotator LocalRotation = FRotator::ZeroRotator;
	FVector Scale = FVector::OneVector;
	EReEchoVfxScalePolicy ScalePolicy = EReEchoVfxScalePolicy::InheritAttachment;
};

/** Centralized semantic-to-asset mapping. Gameplay code never stores Niagara paths. */
struct REECHO_API FReEchoCombatVfxCatalog
{
	static FString ResolvePath(EReEchoCombatVfxSemantic Semantic);
	/** Single-ball texture used by the logic-driven rabbit projectile proxy. */
	static const TCHAR* ResolveRabbitProjectileTexturePath();
	/** Authored emissive material used to preserve the rabbit ball's red bloom without restoring Niagara motion. */
	static const TCHAR* ResolveRabbitProjectileMaterialPath();
	static int32 GetRabbitProjectileGlowMaterialCount();
	/** Standalone soft additive halo adapted from the delivered Glo_c002 texture. */
	static const TCHAR* ResolveRabbitProjectileGlowMaterialPath(int32 LayerIndex);
	/** Enumerates every first-encounter VFX root and proxy dependency for asynchronous warmup. */
	static void GatherPreloadAssetPaths(TArray<FString>& OutPaths);
	static bool IsMeleeAttackPattern(FName AttackPatternId);
	/** Resolves the dedicated one-shot Niagara semantic for a supported melee attack pattern. */
	static bool ResolveMeleeAttackSemantic(FName AttackPatternId, EReEchoCombatVfxSemantic& OutSemantic);
	/** Returns the measured authored center axis for a semantic asset. */
	static FVector ResolveAuthoredForwardAxis(EReEchoCombatVfxSemantic Semantic);
	/** Rotates the semantic asset's authored center axis onto the gameplay direction. */
	static FRotator ResolveRotation(EReEchoCombatVfxSemantic Semantic, const FVector& Direction);
	/** Resource-authored correction and owner-scale policy for attached semantic effects. */
	static FReEchoVfxPlacement ResolvePlacement(EReEchoCombatVfxSemantic Semantic);
};
