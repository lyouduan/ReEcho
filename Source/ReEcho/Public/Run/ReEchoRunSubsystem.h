#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Core/ReEchoTypes.h"
#include "Run/ReEchoShopCatalog.h"
#include "ReEchoRunSubsystem.generated.h"

class UReEchoRunSaveGame;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FReEchoRunPhaseChanged, EReEchoRunPhase, NewPhase);

struct FReEchoCsvDataSnapshot;
struct FReEchoCsvCardRow;

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
	UPROPERTY(BlueprintAssignable)
	FReEchoRunPhaseChanged OnPhaseChanged;

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
	 * 购买即装备：把刚购买的配件直接装入其槽位。
	 * 槽位已满时挤出该槽位最早装备的旧件，旧件仍保留在 OwnedPartIds（回落背包）。
	 */
	bool TryEquipPurchasedPart(FName PartId, FString& OutError);
	/** Equips an already-owned weapon without shop cost or reroll; compatible runes remain equipped. */
	bool TryEquipOwnedWeapon(FName WeaponId, FString& OutError);
	FReEchoWeaponPartShopView GetWeaponPartShopView();
	TSharedPtr<const FReEchoCsvDataSnapshot> GetRunDataSnapshot() const;
	int32 GetTotalEncounterCount() const;

	UFUNCTION(BlueprintCallable)
	void BeginEncounter();

	/** 保存遭遇录制，并根据存活与 Boss 状态推进本轮流程。 */
	UFUNCTION(BlueprintCallable)
	void CompleteEncounter(const FReEchoRecording& Recording, bool bPlayerSurvived, bool bBossKilled);

	/** 根据当前构筑和运行状态生成本次特质卡候选。 */
	UFUNCTION(BlueprintCallable)
	TArray<FReEchoTraitCardOffer> GenerateTraitCardOffers(int32 RequestedCount);

	/** 应用所选特质卡，并更新后续角色和回响共用的构筑快照。 */
	UFUNCTION(BlueprintCallable)
	bool ApplyTraitCard(FName CardId);

	/** Replaces one post-encounter/free card choice in-place using the shared card-slot refresh rule. */
	bool TryRefreshTraitCardSlot(int32 SlotIndex, FString& OutError);

	/** 调试用：将指定卡牌直接加入当前构筑（忽略阶段/候选限制），用于复现与验证卡牌效果（如静默刻度 G_2_17）。仅由 GM
	 * 命令调用，Shipping 构建不暴露。 */
	UFUNCTION(BlueprintCallable)
	bool DebugGrantCard(FName CardId);

	FReEchoCardRuleSnapshot GetCardRules() const;
	FReEchoCardEncounterTickResult AdvanceCardEncounter(float EncounterTimeSeconds);
	void ModifyCardOutgoingHit(FReEchoHitIntent& Intent,
	                           const FReEchoStatBlock& SourceStats,
	                           float EchoDistanceCm,
	                           bool bTargetHasElement);
	float ModifyCardIncomingHit(float RawDamage);
	float NotifyCardReaction(FName ReactionId, bool bTriggeredByPlayer);
	void NotifyCardKill(bool bKilledByEcho);
	void NotifyCardEchoDefeated();
	bool ConsumeCardEchoRemovalRequest();
	int32 GetDiscountedShopPrice(int32 BasePrice) const;
	/** Refreshes only the three weapon/rune offers, using the configured per-encounter budget and price. */
	bool TryRefreshWeaponRuneShop(FString& OutError);
	/** Replaces one visible card in-place. The slot has its own refresh budget and remains the same tier. */
	bool TryRefreshShopCardSlot(int32 Tier, int32 SlotIndex, FString& OutError);
	/** Legacy C++ facade retained for old callers; PaidRefreshPrice is ignored in favor of CSV authority. */
	bool TryConsumeShopRefresh(int32 PaidRefreshPrice);
	bool CanPurchaseExtraShopCard() const;
	bool SetCardAnchorRecording(FGuid RecordingId);
	void ClearCardAnchorRecording();

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

	/** Moves the pending echo into a free storage slot; fails when no free slot exists. */
	EReEchoEchoStorageResult StorePendingRecording();

	/** Moves the pending echo over an explicitly named stored echo, keeping slot order. */
	EReEchoEchoStorageResult StorePendingRecordingReplacing(FGuid ReplacedRecordingId);

	/** Read-only projection for UI and tests; never exposes mutable recording payloads. */
	FReEchoEchoStorageSummary GetEchoStorageSummary() const;

	/** Replaces the whole selection atomically; rejects unknown, duplicate or over-limit ids. */
	EReEchoEchoStorageResult SetSelectedReplayIds(const TArray<FGuid>& RequestedIds);

	const TArray<FGuid>& GetSelectedReplayIds() const
	{
		return SelectedReplayIds;
	}

	int32 GetStorageCapacity() const
	{
		return StorageCapacity;
	}

	int32 GetSpecificReplayLimit() const
	{
		return SpecificReplayLimit;
	}

	/** Refuses out-of-range values and any shrink that would drop already stored echoes. */
	EReEchoEchoStorageResult SetStorageCapacity(int32 NewCapacity);

	/** Refuses out-of-range values; lowering the limit truncates the selection deterministically. */
	EReEchoEchoStorageResult SetSpecificReplayLimit(int32 NewLimit);

	bool TryGetStoredEcho(FGuid RecordingId, FReEchoRecording& OutRecording) const;
	bool TryGetPendingRecording(FReEchoRecording& OutRecording) const;
	bool TryGetLatestCompletedRecording(FReEchoRecording& OutRecording) const;

	/**
	 * Resolves which echoes the next encounter should replay.
	 * Before specific replay is unlocked, this returns the rolling latest echo. Once unlocked, it
	 * returns only explicitly selected stored echoes; an empty selection intentionally returns none.
	 */
	TArray<FReEchoRecording> ResolveReplayRecordings(int32 RequestedCount) const;

	/**
	 * Legacy compatibility facade for the pre-Plan30 single-echo query.
	 * It only forwards to ResolveReplayRecordings, so there is no second history authority.
	 * Plan31 replaces the remaining ReEchoGameMode call sites; later cleanup may remove this facade.
	 */
	UFUNCTION(BlueprintPure)
	TArray<FReEchoRecording> GetEchoRecordings(int32 RequestedCount) const;

	/** Returns true only when the persistent slot contains a compatible, resumable run. */
	bool HasSavedRun() const;
	bool SaveRun(const FReEchoEncounterRuntimeState* EncounterRuntimeState = nullptr) const;
	bool LoadSavedRun();
	void DeleteSavedRun() const;
	bool HasPendingEncounterResume() const;
	FReEchoEncounterRuntimeState ConsumePendingEncounterResume();

	/** In-memory conversion used by persistence and deterministic automation. */
	UReEchoRunSaveGame* CreateSaveSnapshot(const FReEchoEncounterRuntimeState* EncounterRuntimeState = nullptr) const;
	bool RestoreSaveSnapshot(const UReEchoRunSaveGame& SaveGame);

private:
	int32 ResolveConfiguredFreeTraitTier() const;
	void AdvanceToConfiguredTraitChoice();

	UPROPERTY()
	bool bAutomaticAttackMode = true;

	/** Finished encounter awaiting an explicit store-or-skip decision. */
	UPROPERTY()
	bool bHasPendingRecording = false;

	UPROPERTY()
	FReEchoRecording PendingRecording;

	/** Rolling previous-encounter echo; independent from StoredEchoes and occupies no slot. */
	UPROPERTY()
	bool bHasLatestCompletedRecording = false;

	UPROPERTY()
	FReEchoRecording LatestCompletedRecording;

	UPROPERTY()
	bool bHasPreviousCompletedRecording = false;

	UPROPERTY()
	FReEchoRecording PreviousCompletedRecording;

	/** Explicitly stored echoes only; persistent identity is FReEchoRecording::Id. */
	UPROPERTY()
	TArray<FReEchoRecording> StoredEchoes;

	UPROPERTY()
	TArray<FGuid> SelectedReplayIds;

	UPROPERTY()
	int32 StorageCapacity = ReEchoEchoStorage::DefaultStorageCapacity;

	UPROPERTY()
	int32 SpecificReplayLimit = ReEchoEchoStorage::SpecificReplayUnavailable;

	UPROPERTY()
	TArray<FName> PendingTraitCardIds;

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

	/** Randomized once per run and persisted so reopening a card choice cannot reroll it. */
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
	bool bPendingCardEchoRemoval = false;

	void SetPhase(EReEchoRunPhase NewPhase);

	/** Resets every echo storage field to fresh-run defaults. */
	void ResetEchoStorage();

	/** Drops selections that no longer resolve, de-duplicates, then truncates to the replay limit. */
	void NormalizeSelectedReplayIds();

	int32 FindStoredEchoIndex(const FGuid& RecordingId) const;
};
