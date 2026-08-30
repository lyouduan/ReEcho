#pragma once

#include "CoreMinimal.h"

enum class EReEchoElement : uint8;

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
	PlayerMeleeSlashFlame,
	PlayerMeleeSlashLightning,
	PlayerMeleeSlashGrass,
	PlayerMeleeSlashWater,
	PlayerScytheSlashFlame,
	PlayerScytheSlashLightning,
	PlayerScytheSlashGrass,
	PlayerScytheSlashWater,
	PlayerLongSwordImpact,
	PlayerScytheImpact,
	PlayerBowFlight,
	PlayerBowFlightFlame,
	PlayerBowFlightLightning,
	PlayerBowFlightGrass,
	PlayerBowFlightWater,
	PlayerBowImpact,
	PlayerGunFlight,
	PlayerGunFlightFlame,
	PlayerGunFlightLightning,
	PlayerGunFlightGrass,
	PlayerGunFlightWater,
	PlayerGunMuzzle,
	PlayerProjectileImpact,
	PlayerProjectileExplosionImpact,
	EnemyHurt,
	EchoWaterAura,
	EchoGrassAura,
	EchoBorn,
	EchoConnectionLine,
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
	bool bScaleWithAttackRange = false;
	FVector AttackRangeScaleMask = FVector::ZeroVector;
	float MinAttackRangeMultiplier = 0.5f;
	float MaxAttackRangeMultiplier = 2.0f;
	bool bUseWorldDirectionRotation = false;
	float PlaybackDurationSeconds = 0.6f;
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
	/** Resolves a Longsword attack's final combat element to its dedicated slash semantic. */
	static bool ResolveLongSwordSlashSemantic(EReEchoElement Element, EReEchoCombatVfxSemantic& OutSemantic);
	static bool IsLongSwordSlashSemantic(EReEchoCombatVfxSemantic Semantic);
	/** Resolves a Scythe attack's final combat element to its dedicated slash semantic. */
	static bool ResolveScytheSlashSemantic(EReEchoElement Element, EReEchoCombatVfxSemantic& OutSemantic);
	static bool IsScytheSlashSemantic(EReEchoCombatVfxSemantic Semantic);
	/** Resolves a successful weapon commit to its weapon-local release semantic. */
	static bool ResolveAttackCommittedSemantic(FName AttackPatternId, EReEchoCombatVfxSemantic& OutSemantic);
	/** Resolves a successful source-side weapon hit to its configured DamageApplied semantic. */
	static bool ResolveWeaponDamageSemantic(FName WeaponId, EReEchoCombatVfxSemantic& OutSemantic);
	/** Resolves a Gun projectile's already-authoritative combat element to its dedicated flight semantic. */
	static bool ResolveGunFlightSemantic(EReEchoElement Element, EReEchoCombatVfxSemantic& OutSemantic);
	/** Resolves a Bow projectile's already-authoritative combat element to its dedicated flight semantic. */
	static bool ResolveBowFlightSemantic(EReEchoElement Element, EReEchoCombatVfxSemantic& OutSemantic);
	/** Selects the shared ranged impact, or the explosion impact when the compiled projectile has an area radius. */
	static bool ResolveProjectileImpactSemantic(FName WeaponVisualKey,
	                                            float ExplosionRadiusCm,
	                                            EReEchoCombatVfxSemantic& OutSemantic);
	/** Visual-only delay used to release a melee slash after its weapon completes the authored motion. */
	static float ResolveMeleeSlashDelay(EReEchoCombatVfxSemantic Semantic);
	/** Returns the measured authored center axis for a semantic asset. */
	static FVector ResolveAuthoredForwardAxis(EReEchoCombatVfxSemantic Semantic);
	/** Rotates the semantic asset's authored center axis onto the gameplay direction. */
	static FRotator ResolveRotation(EReEchoCombatVfxSemantic Semantic, const FVector& Direction);
	/** Resource-authored correction and owner-scale policy for attached semantic effects. */
	static FReEchoVfxPlacement ResolvePlacement(EReEchoCombatVfxSemantic Semantic);
};
