#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Core/ReEchoTypes.h"
#include "Run/ReEchoShopCatalog.h"
#include "ReEchoRunSubsystem.generated.h"

class UReEchoRunSaveGame;
class UReEchoPlayerProgressSaveGame;

/** Read-only committed currency state. Cash and debt both affect shop eligibility. */
struct REECHO_API FReEchoTimeShardBalance
{
	int32 Cash = 0;
	int32 Debt = 0;

	int32 GetDisplayedBalance() const
	{
		return Cash - Debt;
	}

	bool operator==(const FReEchoTimeShardBalance& Other) const
	{
		return Cash == Other.Cash && Debt == Other.Debt;
	}
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FReEchoTimeShardBalanceChanged,
                                     const FReEchoTimeShardBalance&,
                                     const FReEchoTimeShardBalance&);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FReEchoRunPhaseChanged, EReEchoRunPhase, NewPhase);
DECLARE_MULTICAST_DELEGATE_TwoParams(FReEchoCardGrantCommitted, const FReEchoStatBlock&, EReEchoHealthAdjustment);

struct FReEchoCsvDataSnapshot;
struct FReEchoCsvCardRow;
struct FReEchoCardBuildState;

USTRUCT(BlueprintType)
struct REECHO_API FReEchoSaveSlotSummary
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 SlotIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly)
	bool bOccupied = false;

	UPROPERTY(BlueprintReadOnly)
	int32 EncounterNumber = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 CardCount = 0;

	UPROPERTY(BlueprintReadOnly)
	FDateTime SavedAtUtc;

	UPROPERTY(BlueprintReadOnly)
	FString PreviewScreenshotPath;
};

struct REECHO_API FReEchoStartRunResolveResult
{
	bool bSuccess = false;
	FReEchoBuildSnapshot Build;
	FString Error;
};

namespace ReEchoRunData
{
REECHO_API FReEchoStartRunResolveResult ResolveStartingBuildFromSnapshot(const FReEchoCsvDataSnapshot* Snapshot,
                                                                         FName CharacterId,
                                                                         FName WeaponId);
REECHO_API FReEchoStartRunResolveResult ResolveStartingBuild(FName CharacterId, FName WeaponId);
REECHO_API bool TryApplyCardEffectsToBuild(const FReEchoCsvCardRow& Card,
                                           const FReEchoBuildSnapshot& Build,
                                           FReEchoBuildSnapshot& OutBuild);
}

/** 跨关卡保存本轮构筑、遭遇进度和回响记录的运行时状态。 */
UCLASS()

class REECHO_API UReEchoRunSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UPROPERTY(BlueprintAssignable)
	FReEchoRunPhaseChanged OnPhaseChanged;

	/** Read-only post-commit notification. Subscribers may project the build but cannot mutate the transaction. */
	FReEchoCardGrantCommitted OnCardGrantCommitted;

	/** Non-combat economy commands broadcast once after the complete outer transaction, never mid-grant. */
	FReEchoTimeShardBalanceChanged OnTimeShardBalanceChanged;
	FReEchoTimeShardBalance GetTimeShardBalance() const;
	/** GM setters preserve the existing cash-only clamp; unlike direct assignment they notify shop observers. */
	void DebugSetTimeShards(int64 Amount);
	void DebugAddTimeShards(int32 Amount);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EReEchoRunPhase Phase = EReEchoRunPhase::CharacterSelect;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 EncounterIndex = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 TimeShards = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FReEchoBuildSnapshot CurrentBuild;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FName> InventoryItems;

	/** Weapon parts purchased during this run. Kept separate from one-shot shop items. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FName> OwnedPartIds;

	/** Weapons purchased/obtained during this run. Marks shop offers as owned and excludes from re-rolls. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TSet<FName> OwnedWeaponIds;

	/**
	 * Copies of each rune currently held (backpack + equipped). OwnedPartIds is a de-duplicated ownership
	 * set, so repeated purchases of the same rune are counted here to drive I->II->III synthesis.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TMap<FName, int32> RuneAcquisitionCounts;

	/** Runes already bought from the current shop page; their slots stay sold until the page regenerates. */
	TSet<FName> PurchasedWeaponPartOfferIds;

	UFUNCTION(BlueprintCallable)
	void StartRun(FName CharacterId, FName WeaponId);

	bool IsAutomaticAttackMode() const
	{
		return bAutomaticAttackMode;
	}

	void SetAutomaticAttackMode(bool bAutomatic)
	{
		bAutomaticAttackMode = bAutomatic;
	}

	bool TryEquipParts(const TArray<FName>& PartIds, FString& OutError);
	/**
	 * 显式背包装备：购买只入包/合成，不调用本命令自动挤槽。
	 * 槽位已满时挤出该槽位最早装备的旧件，旧件仍保留在
	 * OwnedPartIds（回落背包）。
	 */
	bool TryEquipPurchasedPart(FName PartId, FString& OutError);
	/**
	 * Equips a part into a specific occurrence of its slot type. Dual weapon slots render two independent
	 * slots of the same type, so switching one slot must replace only that occurrence and leave the other
	 * untouched (TryEquipPurchasedPart instead squeezes the oldest one, which visibly disturbs both slots).
	 */
	bool TryEquipPurchasedPartAt(FName PartId, int32 TargetOccurrenceIndex, FString& OutError);
	/** Rolls and freezes up to three currently eligible choices when a tier pack is paid. */
	void RollShopCardPackCandidates(const FReEchoCsvDataSnapshot& SnapshotRef,
	                                FReEchoCardBuildState& CardState,
	                                FReEchoShopCardPackRuntimeState& Pack,
	                                int32 Tier);
	/** Equips an already-owned weapon without shop cost or reroll; compatible runes remain equipped. */
	bool TryEquipOwnedWeapon(FName WeaponId, FString& OutError);
	/** Read-only owned-card presentation shared by the shop and terminal result screens. */
	TArray<FReEchoShopOffer> GetOwnedBuildCardView() const;
	/** True when the current build owns any card authored in the EasterEgg offer group. */
	bool HasOwnedEasterEggCard() const;
	FReEchoWeaponPartShopView GetWeaponPartShopView();
	/** Cash minus Curse Bank debt; presentation-only and never used for purchase authority. */
	int32 GetDisplayedTimeShardBalance() const;
	TSharedPtr<const FReEchoCsvDataSnapshot> GetRunDataSnapshot() const;
	/**
	 * True for tier-I runes, which stay purchasable while already owned because repeat purchases are what
	 * feed I->II->III synthesis. The shop must route these through the real purchase transaction instead
	 * of the free "re-equip an owned part" shortcut.
	 */
	bool IsShopOfferRepeatPurchasable(FName ItemId) const;
	/**
	 * How many cards a pack of the given tier lets the player take. Cadence abilities (for example the Sage
	 * taking two cards from every fourth non-tier-1 pack) raise this above 1. Evaluated before the choice
	 * screen opens so the UI can require that many picks.
	 */
	int32 ResolvePackSelectableCardCount(int32 CardPackTier) const;
	/** How many cards the pending post-encounter card pack lets the player take (1 normally, 2 on cadence). */
	int32 GetPendingTraitCardSelectableCount() const { return PendingTraitCardSelectableCount; }
	/** How many cards the shop pack waiting to be claimed lets the player take (1 normally, 2 on cadence). */
	int32 GetPaidShopCardPackSelectableCount() const;
	/** Applies several cards from the pending post-encounter pack at once (cadence ability: 3-choose-2). */
	bool ApplyTraitCards(const TArray<FName>& CardIds);
	/** Claims several cards from an already-paid shop pack at once (cadence ability: 3-choose-2). */
	FReEchoShopPurchaseOutcome ClaimPaidShopCardChoices(const TArray<FName>& ItemIds);
	int32 GetTotalEncounterCount() const;

	UFUNCTION(BlueprintCallable)
	void BeginEncounter();

	/** 保存遭遇录制，并根据存活与 Boss 状态推进本轮流程。 */
	UFUNCTION(BlueprintCallable)
	void CompleteEncounter(const FReEchoRecording& Recording,
	                       bool bPlayerSurvived,
	                       bool bBossKilled,
	                       float PlayerCurrentHealth = -1.0f);

	/** Skips the configured post-encounter free-card phase for the authored Encounter 1 CG route. */
	bool SkipPostEncounterCardChoiceForStageTransitionCg();

	/** 根据当前构筑和运行状态生成本次特质卡候选。 */
	UFUNCTION(BlueprintCallable)
	TArray<FReEchoTraitCardOffer> GenerateTraitCardOffers(int32 RequestedCount);

	/** 应用所选特质卡，并更新后续角色和回响共用的构筑快照。 */
	UFUNCTION(BlueprintCallable)
	bool ApplyTraitCard(FName CardId);

	/** Replaces one post-encounter/free card choice in-place using the shared card-slot refresh rule. */
	bool TryRefreshTraitCardSlot(int32 SlotIndex, FString& OutError);

	/** Clears any in-flight free/bonus trait-card choice offers. Used by the GameMode gate so an orphaned
	 *  trait-choice screen can never survive an encounter advance (which would leave the screen interactable
	 *  while Phase != CardChoice, soft-locking refresh/confirm). */
	void ResetPendingTraitCardChoice();

	/** 调试用：将指定卡牌直接加入当前构筑（忽略阶段/候选限制），用于复现与验证卡牌效果（如静默刻度 G_2_17）。仅由 GM
	 * 命令调用，Shipping 构建不暴露。 */
	UFUNCTION(BlueprintCallable)
	bool DebugGrantCard(FName CardId);

	FReEchoCardRuleSnapshot GetCardRules() const;
	/** Enables the hidden Phase3 final-stat multiplier once without changing canonical equipment-base stats. */
	bool ActivateBossPhase3FinalStatMultiplier();
	static void ApplyBossPhase3FinalStatMultiplier(FReEchoStatBlock& Stats);
	FReEchoCardEncounterTickResult AdvanceCardEncounter(float EncounterTimeSeconds);
	void ModifyCardOutgoingHit(FReEchoHitIntent& Intent,
	                           const FReEchoStatBlock& SourceStats,
	                           float EchoDistanceCm,
	                           float NearestEchoDistanceCm,
	                           bool bHasLivingEcho,
	                           bool bTargetHasElement);
	float ModifyCardIncomingHit(float RawDamage, FName AttackerDefinitionId = NAME_None);
	float NotifyCardReaction(FName ReactionId, bool bTriggeredByPlayer);
	void NotifyCardReactionAffectedTargets(FName ReactionId, const TArray<int32>& AffectedSpawnIndices);
	float GetCardReactionDamageMultiplier(FName ReactionId) const;
	void NotifyCardKill(bool bKilledByEcho, FName TargetDefinitionId = NAME_None);
	float NotifyCardDamageResolved(float RawDamage, float AppliedDamage, bool bDealtByEcho);
	void NotifyCardPlayerDamageReceived(float AppliedDamage);
	float NotifyCardNegativeStatusApplied(FName StatusId, bool bAppliedByEcho);
	void NotifyCardEchoDefeated();
	bool ConsumeCardEchoRemovalRequest();
	/** Derives a stable card-effect roll from this run's real-time seed without exposing mutable random state. */
	int32 BuildCardEffectRandomSeed(FName ContextId, int32 Sequence) const;
	int32 GetDiscountedShopPrice(int32 BasePrice) const;
	/** Refreshes only the three weapon/rune offers, using the configured per-encounter budget and price. */
	bool TryRefreshWeaponRuneShop(FString& OutError);
	/** Replaces one visible card in-place. The slot has its own refresh budget and remains the same tier. */
	bool TryRefreshShopCardSlot(int32 Tier, int32 SlotIndex, FString& OutError);
	/** Legacy C++ facade retained for old callers; PaidRefreshPrice is ignored in favor of CSV authority. */
	bool TryConsumeShopRefresh(int32 PaidRefreshPrice);
	bool CanPurchaseExtraShopCard() const;
	/** Unified purchase transaction: returns a reasoned result and always emits one Before/Result/After audit. */
	FReEchoShopPurchaseOutcome PurchaseShopItemDetailed(FName ItemId);
	/** Commits one fixed-tier card-pack payment and fires purchase-triggered cards exactly once. */
	FReEchoShopPurchaseOutcome PurchaseShopCardPackDetailed(int32 Tier);
	/** Claims one candidate from an already-paid pack without charging or firing purchase-triggered cards. */
	FReEchoShopPurchaseOutcome ClaimPaidShopCardChoice(FName ItemId);

	/** Backward-compatible bool facade. New UI/gameplay call sites should consume PurchaseShopItemDetailed. */
	UFUNCTION(BlueprintCallable)
	bool PurchaseShopItem(FName ItemId);

	/** Adds collected world-drop currency without exposing a writable currency field to the pickup actor. */
	bool GrantTimeShards(int32 Amount);
	/** True when a positive shop cost can be committed, including unlimited 诅咒银行 credit. */
	bool CanPayShopCost(int32 Cost) const;

	/** Resolves one configured enemy-death drop amount without changing the currency balance. */
	int32 ResolveEnemyDeathTimeShardDrop(FName EnemyId, int32 SpawnIndex);

	// --- Echo storage commands (Plan30) -------------------------------------------------
	// Every command is narrow and transactional: on a non-Success result nothing is mutated.

	/**
	 * Stages a successfully completed encounter as the pending store-or-skip decision and
	 * overwrites the rolling latest slot with the same recording.
	 * A still unresolved earlier pending decision expires deterministically here.
	 */
	EReEchoEchoStorageResult StagePendingRecording(const FReEchoRecording& Recording);

	bool HasPendingRecording() const
	{
		return bHasPendingRecording;
	}

	/** Drops permanent-storage eligibility for the pending echo; the rolling latest slot is kept. */
	EReEchoEchoStorageResult SkipPendingRecordingStorage();

	/** Replaces G_3_02's one persistent time anchor with the pending encounter. */
	EReEchoEchoStorageResult StorePendingRecordingAsTimeAnchor();

	/** Read-only projection for UI and tests; never exposes mutable recording payloads. */
	FReEchoEchoStorageSummary GetEchoStorageSummary() const;

	bool TryGetPendingRecording(FReEchoRecording& OutRecording) const;
	bool TryGetLatestCompletedRecording(FReEchoRecording& OutRecording) const;

	/**
	 * Resolves which echoes the next encounter should replay.
	 * G_3_02's single valid time anchor wins; otherwise this returns the rolling latest echoes.
	 */
	TArray<FReEchoRecording> ResolveReplayRecordings(int32 RequestedCount) const;

	/**
	 * Legacy compatibility facade for the pre-Plan30 single-echo query.
	 * It only forwards to ResolveReplayRecordings, so there is no second history authority.
	 * Plan31 replaces the remaining ReEchoGameMode call sites; later cleanup may remove this facade.
	 */
	UFUNCTION(BlueprintPure)
	TArray<FReEchoRecording> GetEchoRecordings(int32 RequestedCount) const;

	static constexpr int32 SaveSlotCount = 3;

	/** Returns true when any of the three persistent slots contains a compatible, resumable run. */
	bool HasSavedRun() const;
	TArray<FReEchoSaveSlotSummary> GetSaveSlotSummaries() const;
	bool SelectSaveSlot(int32 SlotIndex);
	bool SelectFirstEmptySaveSlot();
	int32 GetActiveSaveSlotIndex() const { return ActiveSaveSlotIndex; }
	bool SaveRun(const FReEchoEncounterRuntimeState* EncounterRuntimeState = nullptr) const;
	bool LoadSavedRun();
	bool LoadSavedRunFromSlot(int32 SlotIndex);
	void DeleteSavedRun() const;
	FString GetActiveSaveSlotPreviewPath() const;
	bool WriteActiveSaveSlotPreview(const TArray<uint8>& PngBytes) const;
	bool HasPendingEncounterResume() const;
	FReEchoEncounterRuntimeState ConsumePendingEncounterResume();
	bool HasViewedStage01To02Cg() const { return bHasViewedStage01To02Cg; }
	/** Persists the account-level watched flag. Returns false without granting it when persistence fails. */
	bool MarkStage01To02CgViewed();

	/** In-memory conversion used by persistence and deterministic automation. */
	UReEchoRunSaveGame* CreateSaveSnapshot(const FReEchoEncounterRuntimeState* EncounterRuntimeState = nullptr) const;
	bool RestoreSaveSnapshot(const UReEchoRunSaveGame& SaveGame);

private:
	class FScopedTimeShardBalanceChange;
	int32 TimeShardBalanceChangeDepth = 0;
	FReEchoTimeShardBalance TimeShardBalanceBeforeChange;

	void LoadPlayerProgress();
	FString GetSaveSlotName(int32 SlotIndex) const;
	FString GetSaveSlotPreviewPath(int32 SlotIndex) const;
	const UReEchoRunSaveGame* LoadValidatedSaveForSlot(int32 SlotIndex, bool& bOutLegacy) const;
	void CommitShopCost(int32 Cost);
	void ApplyProjectedCardCurrency(int32 PreviousBalance,
	                                int32 ProjectedBalance,
	                                bool bApplyEncounterIncomeRules = true);
	void RefreshCurseBankOutcome();
	void RefreshWeaponMasterOutcome();
	int32 ResolveConfiguredFreeTraitTier() const;
	void AdvanceToConfiguredTraitChoice();

	UPROPERTY()
	bool bAutomaticAttackMode = true;

	UPROPERTY()
	bool bHasViewedStage01To02Cg = false;

	/** All subsequent automatic saves target this slot. INDEX_NONE means no new run slot was selected yet. */
	UPROPERTY()
	int32 ActiveSaveSlotIndex = INDEX_NONE;

	/** Finished encounter awaiting an explicit store-or-skip decision. */
	UPROPERTY()
	bool bHasPendingRecording = false;

	UPROPERTY()
	FReEchoRecording PendingRecording;

	/** Rolling previous-encounter echo; independent from the time anchor. */
	UPROPERTY()
	bool bHasLatestCompletedRecording = false;

	UPROPERTY()
	FReEchoRecording LatestCompletedRecording;

	UPROPERTY()
	bool bHasPreviousCompletedRecording = false;

	UPROPERTY()
	FReEchoRecording PreviousCompletedRecording;

	/** G_3_02's only persistent recording. The card runtime stores the matching stable id. */
	UPROPERTY()
	bool bHasTimeAnchorRecording = false;

	UPROPERTY()
	FReEchoRecording TimeAnchorRecording;

	UPROPERTY()
	TArray<FName> PendingTraitCardIds;

	/** All cards displayed by the current free-choice group, including replaced cards. */
	UPROPERTY()
	TArray<FName> PendingTraitCardOfferHistoryIds;

	UPROPERTY()
	TArray<int32> PendingTraitCardRefreshUses;

	UPROPERTY()
	int32 PendingTraitCardOfferEncounterIndex = INDEX_NONE;

	/** Stable weapon/rune page. Ownership changes mark offers sold but do not regenerate the remaining slots. */
	UPROPERTY()
	int32 WeaponPartShopOfferEncounterIndex = INDEX_NONE;

	UPROPERTY()
	int32 WeaponPartShopOfferRefreshSequence = INDEX_NONE;

	UPROPERTY()
	TArray<FName> WeaponPartShopOfferIds;

	/** Current desired weapon/rune offer page; separate from the cached page key above. */
	UPROPERTY()
	int32 WeaponRuneRefreshSequence = 0;

	UPROPERTY()
	int32 WeaponRuneRefreshEncounterIndex = INDEX_NONE;

	UPROPERTY()
	int32 WeaponRuneRefreshesUsed = 0;

	/** Unified random root generated once per run and persisted for every randomized content outlet. */
	UPROPERTY()
	int32 RunSeed = 0;

	/** Run-scoped stream for card offers; persisted so reopening a card choice cannot reroll it. */
	UPROPERTY()
	int32 TraitOfferSeed = 0;

	/** Independent per-run seed for deterministic enemy shard ranges. */
	UPROPERTY()
	int32 EnemyShardDropSeed = 0;

	/** Packed EncounterIndex/SpawnIndex identities already processed by the reward transaction. */
	UPROPERTY()
	TSet<int64> RewardedEnemyShardDropKeys;

	UPROPERTY()
	FReEchoEncounterRuntimeState PendingEncounterResume;

	TSharedPtr<const FReEchoCsvDataSnapshot> RunDataSnapshot;
	/** Cards the pending post-encounter pack lets the player take; >1 when a cadence ability fires on it. */
	int32 PendingTraitCardSelectableCount = 1;
	/** Picks still owed by the pending post-encounter pack. The pack stays open until this reaches zero. */
	int32 PendingTraitCardPicksRemaining = 1;
	bool bPendingCardEchoRemoval = false;

	void SetPhase(EReEchoRunPhase NewPhase);

	/** Resets every echo storage field to fresh-run defaults. */
	void ResetEchoStorage();

	void ReevaluateCoreCollectionCard();

	void ClearTimeAnchorRecording();
};
