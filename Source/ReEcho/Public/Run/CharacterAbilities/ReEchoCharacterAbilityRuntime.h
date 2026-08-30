#pragma once

#include "CoreMinimal.h"

struct FReEchoBuildSnapshot;
struct FReEchoCsvDataSnapshot;
struct FReEchoStatBlock;

/** Data-driven character ability policy. It owns no world objects and never mutates Combat directly. */
namespace ReEchoCharacterAbilityRuntime
{
REECHO_API void ApplyStaticBuildEffects(const FReEchoCsvDataSnapshot& Snapshot,
                                        FName CharacterId,
                                        FReEchoStatBlock& Stats);
REECHO_API void ApplyEncounterCompletedEffects(const FReEchoCsvDataSnapshot& Snapshot,
                                               FName CharacterId,
                                               FReEchoStatBlock& Stats);
REECHO_API int32 ResolveExtraTraitChoices(const FReEchoCsvDataSnapshot& Snapshot,
                                          FName CharacterId,
                                          int32 NormalSelectionCount);
/**
 * True when a card pack of the given tier should advance the character's EveryNth cadence counter.
 * Both shop packs and the post-encounter free pack use the same 1/2/3 tiering, so either source can be
 * passed here. A non-positive tier (no pack dropped) never satisfies a tier-gated ability such as the
 * Sage counting only non-tier-1 card packs.
 */
REECHO_API bool DoesCardPackTierAdvanceTraitBonus(const FReEchoCsvDataSnapshot& Snapshot,
                                                  FName CharacterId,
                                                  int32 CardPackTier);
/**
 * Same cadence as ResolveExtraTraitChoices but tier-aware: only packs at or above an ability's
 * MinCardPackTier trigger it. Returns how many extra cards the pack should let the player pick,
 * which turns the usual 3-choose-1 into 3-choose-(1 + result).
 */
REECHO_API int32 ResolveExtraTraitChoicesForTier(const FReEchoCsvDataSnapshot& Snapshot,
                                                 FName CharacterId,
                                                 int32 NormalSelectionCount,
                                                 int32 CardPackTier);
REECHO_API FVector2D ResolveCurrentMissingHealthAttackBonus(const FReEchoCsvDataSnapshot& Snapshot,
                                                            FName CharacterId,
                                                            float CurrentHealth,
                                                            float MaximumHealth);
}
