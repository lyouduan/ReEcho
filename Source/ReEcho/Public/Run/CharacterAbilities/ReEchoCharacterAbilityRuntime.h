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
REECHO_API FVector2D ResolveCurrentMissingHealthAttackBonus(const FReEchoCsvDataSnapshot& Snapshot,
                                                            FName CharacterId,
                                                            float CurrentHealth,
                                                            float MaximumHealth);
}
