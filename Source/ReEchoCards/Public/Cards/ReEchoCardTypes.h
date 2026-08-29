#pragma once

#include "Combat/ReEchoCombatTypes.h"
#include "CoreMinimal.h"
#include "ReEchoCardTypes.generated.h"

UENUM(BlueprintType)
enum class EReEchoCardValueOperation : uint8
{
	Add,
	Multiply,
	Override
};

UENUM(BlueprintType)
enum class EReEchoCardEconomyPenalty : uint8
{
	None,
	NoShopRefresh,
	NoExtraCardPurchase,
	NoEnemyShardDrops
};

/** Stable, serializable kinds of resolved card results. UI text is projected outside the Cards module. */
UENUM(BlueprintType)
enum class EReEchoCardOutcomeKind : uint8
{
	None,
	PendingEncounter,
	StatTrade,
	GrantedCards,
	StatGain,
	EconomyPenalty,
	FreeShopRefreshes,
	TimeShards,
	CumulativeStatGain,
	RunReset,
	FreeShopVisit,
	UnlimitedRefresh,
	Debt,
	WeaponMaster,
	RandomDetails
};

USTRUCT(BlueprintType)

struct REECHOCARDS_API FReEchoCardOutcomeState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName CardId = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EReEchoCardOutcomeKind Kind = EReEchoCardOutcomeKind::None;
	/** Stable stat/effect target such as PhysicalAttack, ElementalAttack or HpMaxAndPoint. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName PrimaryTarget = NAME_None;
	/** Latest or cumulative resolved value, according to Kind. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float PrimaryValue = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName SecondaryTarget = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float SecondaryValue = 0.0f;
	/** Number of resolved contributing events for cumulative outcomes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 ResolutionCount = 0;
	/** Target encounter while PendingEncounter; INDEX_NONE after settlement. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 EncounterIndex = INDEX_NONE;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EReEchoCardEconomyPenalty EconomyPenalty = EReEchoCardEconomyPenalty::None;
	/** Actual granted card IDs for deterministic display through the immutable catalog. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FName> RelatedCardIds;
	/** Arbitrary resolved stat details for cards with more than two independently rolled outcomes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FName> DetailTargets;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<float> DetailValues;
};

USTRUCT(BlueprintType)

struct REECHOCARDS_API FReEchoCardEffectDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Id = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Order = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Trigger = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName BehaviorId = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Target = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EReEchoCardValueOperation Operation = EReEchoCardValueOperation::Add;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Value = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName ParamName = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float ParamValue = 0.0f;
};

USTRUCT(BlueprintType)

struct REECHOCARDS_API FReEchoCardDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Id = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Tier = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FString DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Description;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FName> Tags;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName PromotionRoleId = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName OfferGroup = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName StackPolicy = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName ConflictPolicy = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bEnabled = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bOfferable = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FReEchoCardEffectDefinition> Effects;
};

/** Serializable state owned by the Cards domain. Time-bearing fields are remaining durations at save boundaries. */
USTRUCT(BlueprintType)

struct REECHOCARDS_API FReEchoShopCardPackRuntimeState
{
	GENERATED_BODY()

	/** Fixed tier identity. Runtime arrays use [Tier1, Tier2, Tier3]. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Tier = 0;
	/** Stable candidates for the current encounter. Each array index is one independently refreshable visible slot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FName> CandidateCardIds;
	/** Every card displayed by this tier pack, including candidates replaced by a slot refresh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FName> OfferHistoryCardIds;
	/** Per-slot refresh uses. Shape always matches CandidateCardIds; each slot owns its own configured limit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<int32> SlotRefreshUses;
	/** Stable undiscounted price paid when entering this pack. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 BasePrice = 0;
	/** Payment is committed before the choice screen opens and survives leaving the screen. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bPaymentCommitted = false;
	/** A successful card claim consumes this pack for the current page. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bPurchased = false;
	/** Remaining purchase count for this tier slot (from ShopTiers Tier:Count config). 0 = sold out / not configured. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 RemainingPurchases = 0;
	/** Claims made on this tier slot for the current page. Seeds each repeat purchase so it rolls its own cards. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 ClaimCount = 0;
	/** How many of the three cards this pack lets the player take. 1 normally; 2 on a tiered cadence ability. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 SelectableCardCount = 1;
	/** Picks still owed by this paid pack. Above 1 the pack stays open for another pick. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 PicksRemaining = 0;
};

USTRUCT(BlueprintType)

struct REECHOCARDS_API FReEchoCardRuntimeState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 RandomSequence = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 ActiveEncounterIndex = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float LastEncounterTimeSeconds = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 PreventedDamageCount = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bSubstitutionEchoRemovalFired = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bTenSecondStunFired = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bTwentySecondStunFired = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 LastEchoAuraPulseIndex = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 LastEchoHeadCursePulseIndex = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float ReactionHealCooldownRemaining = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 HuntTrackingEncounterIndex = INDEX_NONE;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 HuntKillCount = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 ReactionTrackingEncounterIndex = INDEX_NONE;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 ReactionCount = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 EchoKillProgress = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 PlayerKillProgress = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 EncounterKillCount = 0;
	/** Run-wide player-only kill history keyed by stable enemy definition id. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TMap<FName, int32> PlayerKillCountByEnemyId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FName> DistinctReactionIds;
	/** First reaction recorded by 回归基本功; None means the next resolved reaction becomes the record. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName RecordedReactionId = NAME_None;
	/** Outstanding principal plus encounter interest for 诅咒银行. Incoming shards repay this before cash balance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 TimeShardDebt = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bCurseBankDefaulted = false;
	/** Weapons that have completed at least one encounter while 武器大师 is owned. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FName> MasteredWeaponIds;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 FreeShopRefreshes = 0;
	/** This encounter's shop goods cost zero. INDEX_NONE means no free shop visit is active. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 FreeShopEncounterIndex = INDEX_NONE;
	/** Weapon/rune refresh-count limit is suspended until the next committed shop purchase. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bUnlimitedWeaponRuneRefresh = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bDragonSoulCompleted = false;
	/** Stable enemy definition ids whose damage is fully prevented for the rest of the run (兔兔杀手 etc.). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TSet<FName> ImmuneEnemyDefinitionIds;
	/** One-shot kill-threshold cards that have already resolved, keyed by card id. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TSet<FName> CompletedKillThresholdCardIds;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bNumericChallengeCompleted = false;
	/** Final applied damage resolved during the active encounter, split by stable source domain. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float EncounterPlayerDamage = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float EncounterEchoDamage = 0.0f;
	/** Self-race bonus selected from the previous completed encounter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float PlayerDamageMultiplier = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float EchoDamageMultiplier = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 ConductAffectedCount = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<int32> ConductAffectedSpawnIndices;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float ConductPlayerDamageMultiplier = 1.0f;
	/** Exact additive EchoEfficiency already materialized by the three-piece Echo set. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float EchoTrinityEfficiencyGranted = 0.0f;
	/** Easter-card encounter state. All fields are save-authoritative and deterministic. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 LastEasterStunPulseIndex = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float EasterDamageTaken = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bEasterDamageCardsGranted = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bHasPreviousEncounterShardIncome = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 PreviousEncounterGrossShardIncome = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 CurrentEncounterGrossShardIncome = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float EncounterShardIncomeMultiplier = 1.0f;
	/** SaveVersion <= 16 compatibility only. Weapon/rune and card refreshes no longer share this sequence. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 ShopRefreshSequence = 0;
	/** Stable build-card pack page for the current encounter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 ShopCardOfferEncounterIndex = INDEX_NONE;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 ShopCardOfferRefreshSequence = INDEX_NONE;
	/** Fixed [Tier1, Tier2, Tier3] pack states. An unconfigured/exhausted pack has no candidates. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FReEchoShopCardPackRuntimeState> ShopCardPackStates;
	/** SaveVersion <= 15 compatibility only. New runtime pages never populate this single-card cache. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FName> ShopCardOfferIds;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 BonusShardDropEncounterIndex = INDEX_NONE;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EReEchoCardEconomyPenalty EconomyPenalty = EReEchoCardEconomyPenalty::None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bHasAnchorRecording = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FGuid AnchorRecordingId;
	/** Stable resolved outcomes used by read-only owned-card presentation and saved with the run. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FReEchoCardOutcomeState> ResolvedOutcomes;
};

/** Card ownership and runtime state embedded by Run/Recording; this is the only writable card truth. */
USTRUCT(BlueprintType)

struct REECHOCARDS_API FReEchoCardBuildState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FString DomainRevision;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FName> OwnedCardIds;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FReEchoCardRuntimeState Runtime;
};

USTRUCT(BlueprintType)

struct REECHOCARDS_API FReEchoCardRuleSnapshot
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float ShopDiscount = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 MaximumEchoes = 1;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bEchoesDisabled = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bEchoesCanAttack = true;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float EchoHealthMultiplier = 1.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bEchoTaunts = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bWaterEchoAura = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bGrassEchoAura = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float EchoSlowAura = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float EnemyElementImmunitySeconds = -1.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bElementDamageCanCrit = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 CriticalRollCount = 1;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float ElementAttachedCriticalEffectBonus = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float DistanceDamageBonusPerStep = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float DistanceDamageStepCm = 100.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float ProximityDamageBonus = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bUnlimitedShopCredit = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bAlternatingPlayerEchoDamage = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bConnectionLineDamage = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bWeaponMaster = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bEchoHead = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bEchoBody = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bEchoLegs = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bEchoTrinityComplete = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bInfiniteStackingBurn = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bVaporizeWaterSplash = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bConductDamageGrowth = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bOverhealCapacity = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bCriticalOverridesElement = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bDoubleNonCoreSlotCapacity = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bDisableShopRefresh = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bDisableExtraCardPurchase = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bDisableEnemyShardDrops = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bEasterEchoContact = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float EasterEchoContactDamage = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float EasterEchoContactHealing = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bEasterRandomStun = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float EasterRandomStunRadiusCm = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float EasterRandomStunDuration = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float EasterRandomStunInterval = 0.5f;
};

USTRUCT()

struct REECHOCARDS_API FReEchoCardGrantInput
{
	GENERATED_BODY()

	FReEchoStatBlock Stats;
	FReEchoCardBuildState CardState;
	int32 TimeShards = 0;
	int32 EncounterIndex = 0;
	int32 RandomSeed = 0;
	bool bRecordOwnership = true;
};

USTRUCT()

struct REECHOCARDS_API FReEchoCardGrantResult
{
	GENERATED_BODY()

	bool bSucceeded = false;
	FString Error;
	FReEchoStatBlock Stats;
	FReEchoCardBuildState CardState;
	int32 TimeShards = 0;
	TArray<FName> GrantedCardIds;
	/** Run-owned rune inventory/equipment must be cleared atomically after this card transaction commits. */
	bool bClearWeaponRunes = false;
	EReEchoHealthAdjustment HealthAdjustment = EReEchoHealthAdjustment::None;
};

USTRUCT()

struct REECHOCARDS_API FReEchoCardEncounterTickResult
{
	GENERATED_BODY()

	FReEchoCardBuildState CardState;
	TArray<float> EnemyStunDurations;
	int32 EchoAuraPulseCount = 0;
	int32 EchoHeadCursePulseCount = 0;
	int32 EasterRandomStunPulseCount = 0;
	float EasterRandomStunRadiusCm = 0.0f;
	float EasterRandomStunDuration = 0.0f;
};

USTRUCT()

struct REECHOCARDS_API FReEchoCardOutgoingHitInput
{
	GENERATED_BODY()

	float RawDamage = 0.0f;
	float CriticalRate = 0.0f;
	float CriticalEffect = 0.0f;
	float DistanceCm = 0.0f;
	float NearestEchoDistanceCm = 0.0f;
	bool bHasLivingEcho = false;
	bool bPreviousPlayerEchoSourceKnown = false;
	EReEchoDamageSource PreviousPlayerEchoSource = EReEchoDamageSource::Player;
	bool bCritical = false;
	bool bTargetHasElement = false;
	EReEchoDamageSource DamageSource = EReEchoDamageSource::Player;
	EReEchoElement Element = EReEchoElement::None;
	FName TargetDefinitionId = NAME_None;
	int32 TimeShards = 0;
	int32 RandomSeed = 0;
};

USTRUCT()

struct REECHOCARDS_API FReEchoCardOutgoingHitResult
{
	GENERATED_BODY()

	FReEchoCardBuildState CardState;
	float RawDamage = 0.0f;
	bool bCritical = false;
	EReEchoElement Element = EReEchoElement::None;
	TArray<FName> PreDamageStatusIds;
	int32 TimeShards = 0;
};

USTRUCT()

struct REECHOCARDS_API FReEchoCardIncomingHitResult
{
	GENERATED_BODY()

	FReEchoCardBuildState CardState;
	float RawDamage = 0.0f;
	int32 TimeShards = 0;
	bool bPrevented = false;
	bool bRemoveAllEchoes = false;
};

USTRUCT()

struct REECHOCARDS_API FReEchoCardEventResult
{
	GENERATED_BODY()

	FReEchoCardBuildState CardState;
	FReEchoStatBlock Stats;
	float Healing = 0.0f;
	int32 TimeShardsGranted = 0;
	int32 FreeShopRefreshesGranted = 0;
	/** Optional absolute post-effect balance; INDEX_NONE means use TimeShardsGranted as a delta. */
	int32 ProjectedTimeShards = INDEX_NONE;
	EReEchoHealthAdjustment HealthAdjustment = EReEchoHealthAdjustment::None;
};
