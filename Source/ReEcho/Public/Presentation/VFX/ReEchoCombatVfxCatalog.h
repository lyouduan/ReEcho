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
	PlayerMeleeSlash,
	EnemyHurt
};

/** Centralized semantic-to-asset mapping. Gameplay code never stores Niagara paths. */
struct REECHO_API FReEchoCombatVfxCatalog
{
	static const TCHAR* ResolvePath(EReEchoCombatVfxSemantic Semantic);
	static bool IsMeleeAttackPattern(FName AttackPatternId);
	/** Returns the measured authored center axis for a semantic asset. */
	static FVector ResolveAuthoredForwardAxis(EReEchoCombatVfxSemantic Semantic);
	/** Rotates the semantic asset's authored center axis onto the gameplay direction. */
	static FRotator ResolveRotation(EReEchoCombatVfxSemantic Semantic, const FVector& Direction);
};
