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
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float ReactionHealCooldownRemaining = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 HuntTrackingEncounterIndex = INDEX_NONE;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 HuntKillCount = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 ReactionTrackingEncounterIndex = INDEX_NONE;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 ReactionCount = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 EchoKillProgress = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 PlayerKillProgress = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 EncounterKillCount = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FName> DistinctReactionIds;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 FreeShopRefreshes = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 ShopRefreshSequence = 0;
	/** Stable build-card offer page for the current encounter/refresh sequence: fixed [Tier1, Tier2, Tier3] ids. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 ShopCardOfferEncounterIndex = INDEX_NONE;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 ShopCardOfferRefreshSequence = INDEX_NONE;
	/** NAME_None marks an unconfigured or exhausted tier slot. Legacy packed pages are rebuilt by Run. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FName> ShopCardOfferIds;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 BonusShardDropEncounterIndex = INDEX_NONE;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EReEchoCardEconomyPenalty EconomyPenalty = EReEchoCardEconomyPenalty::None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bHasAnchorRecording = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FGuid AnchorRecordingId;
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
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float DistanceDamageBonusPerMeter = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bCriticalOverridesElement = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bDoubleNonCoreSlotCapacity = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bDisableShopRefresh = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bDisableExtraCardPurchase = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bDisableEnemyShardDrops = false;
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
};

USTRUCT()

struct REECHOCARDS_API FReEchoCardEncounterTickResult
{
	GENERATED_BODY()

	FReEchoCardBuildState CardState;
	TArray<float> EnemyStunDurations;
	int32 EchoAuraPulseCount = 0;
};

USTRUCT()

struct REECHOCARDS_API FReEchoCardOutgoingHitInput
{
	GENERATED_BODY()

	float RawDamage = 0.0f;
	float CriticalRate = 0.0f;
	float CriticalEffect = 0.0f;
	float DistanceCm = 0.0f;
	bool bCritical = false;
	bool bTargetHasElement = false;
	EReEchoElement Element = EReEchoElement::None;
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
	int32 FreeShopRefreshesGranted = 0;
};
