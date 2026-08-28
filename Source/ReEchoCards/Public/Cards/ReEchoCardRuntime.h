#pragma once

#include "Cards/ReEchoCardCatalog.h"

namespace ReEchoCardRuntime
{
REECHOCARDS_API float ApplyValueOperation(float CurrentValue, EReEchoCardValueOperation Operation, float Value);
REECHOCARDS_API int32 CountOwned(const FReEchoCardBuildState& State, FName CardId);
REECHOCARDS_API bool HasCard(const FReEchoCardBuildState& State, FName CardId);
REECHOCARDS_API bool CanOffer(const FReEchoCardCatalog& Catalog,
                              const FReEchoCardBuildState& State,
                              const FReEchoCardDefinition& Card,
                              int32 EncounterIndex = INDEX_NONE);
REECHOCARDS_API TArray<FReEchoCardDefinition> BuildOfferPool(const FReEchoCardCatalog& Catalog,
                                                             const FReEchoCardBuildState& State,
                                                             FName OfferGroup,
                                                             int32 Tier = INDEX_NONE,
                                                             int32 EncounterIndex = INDEX_NONE);
/** Selects one stable slot: roll Easter first, otherwise use the requested normal tier, never promoting exhaustion. */
REECHOCARDS_API FName SelectOfferForSlot(const FReEchoCardCatalog& Catalog,
                                        const FReEchoCardBuildState& State,
                                        int32 NormalTier,
                                        int32 EncounterIndex,
	                                        const TArray<FName>& OfferHistory,
	                                        int32 RandomSeed,
	                                        float EasterChance,
	                                        bool bExcludeOwnedNormalCards = false);
REECHOCARDS_API FReEchoCardRuleSnapshot CompileRules(const FReEchoCardCatalog& Catalog,
                                                     const FReEchoCardBuildState& State);
REECHOCARDS_API FReEchoCardGrantResult TryGrantCard(const FReEchoCardCatalog& Catalog,
                                                    FName CardId,
                                                    const FReEchoCardGrantInput& Input);
REECHOCARDS_API FReEchoCardBuildState BeginEncounter(const FReEchoCardBuildState& State, int32 EncounterIndex);
REECHOCARDS_API FReEchoCardEncounterTickResult AdvanceEncounter(const FReEchoCardCatalog& Catalog,
                                                                const FReEchoCardBuildState& State,
                                                                float EncounterTimeSeconds);
REECHOCARDS_API FReEchoCardOutgoingHitResult ModifyOutgoingHit(const FReEchoCardCatalog& Catalog,
                                                               const FReEchoCardBuildState& State,
                                                               const FReEchoCardOutgoingHitInput& Input);
REECHOCARDS_API FReEchoCardIncomingHitResult ModifyIncomingHit(const FReEchoCardCatalog& Catalog,
                                                               const FReEchoCardBuildState& State,
                                                               float RawDamage,
                                                               int32 TimeShards);
REECHOCARDS_API FReEchoCardEventResult OnReaction(const FReEchoCardCatalog& Catalog,
                                                  const FReEchoCardBuildState& State,
                                                  const FReEchoStatBlock& Stats,
                                                  FName ReactionId,
                                                  bool bTriggeredByPlayer);
REECHOCARDS_API float
GetReactionDamageMultiplier(const FReEchoCardCatalog& Catalog, const FReEchoCardBuildState& State, FName ReactionId);
REECHOCARDS_API FReEchoCardEventResult OnKillResolved(const FReEchoCardCatalog& Catalog,
                                                      const FReEchoCardBuildState& State,
                                                      const FReEchoStatBlock& Stats,
                                                      bool bKilledByEcho,
                                                      FName TargetDefinitionId = NAME_None);
REECHOCARDS_API FReEchoCardEventResult OnDamageResolved(const FReEchoCardCatalog& Catalog,
                                                        const FReEchoCardBuildState& State,
                                                        const FReEchoStatBlock& Stats,
                                                        float RawDamage,
                                                        float AppliedDamage,
                                                        bool bDealtByEcho);
REECHOCARDS_API FReEchoCardGrantResult OnPlayerDamageReceived(const FReEchoCardCatalog& Catalog,
                                                              const FReEchoCardBuildState& State,
                                                              const FReEchoStatBlock& Stats,
                                                              int32 TimeShards,
                                                              float AppliedDamage,
                                                              int32 EncounterIndex,
                                                              int32 RandomSeed);
REECHOCARDS_API FReEchoCardEventResult OnNegativeStatusApplied(const FReEchoCardCatalog& Catalog,
                                                               const FReEchoCardBuildState& State,
                                                               const FReEchoStatBlock& Stats,
                                                               FName StatusId,
                                                               bool bAppliedByEcho);
REECHOCARDS_API FReEchoCardEventResult OnPurchase(const FReEchoCardCatalog& Catalog,
                                                  const FReEchoCardBuildState& State,
                                                  const FReEchoStatBlock& Stats);
REECHOCARDS_API FReEchoCardEventResult OnCoreInventoryChanged(const FReEchoCardCatalog& Catalog,
                                                              const FReEchoCardBuildState& State,
                                                              const FReEchoStatBlock& Stats,
                                                              int32 DistinctOwnedCoreCount);
REECHOCARDS_API FReEchoCardEventResult OnEchoKilled(const FReEchoCardCatalog& Catalog,
                                                    const FReEchoCardBuildState& State,
                                                    const FReEchoStatBlock& Stats);
REECHOCARDS_API FReEchoCardEventResult EndEncounter(const FReEchoCardCatalog& Catalog,
                                                    const FReEchoCardBuildState& State,
                                                    const FReEchoStatBlock& Stats,
                                                    int32 EncounterIndex,
                                                    int32 PlayerKillCount,
                                                    int32 TimeShards = 0,
                                                    int32 RandomSeed = 0,
                                                    int32 GrossTimeShardIncome = 0);
}
