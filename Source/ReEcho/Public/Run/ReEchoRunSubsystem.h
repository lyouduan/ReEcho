#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Core/ReEchoTypes.h"
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

	UFUNCTION(BlueprintCallable)
	void StartRun(FName CharacterId, FName WeaponId);

	bool TryEquipParts(const TArray<FName>& PartIds, FString& OutError);
	TSharedPtr<const FReEchoCsvDataSnapshot> GetRunDataSnapshot() const;

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

	/** 勇者每关结束后的三档锻炼选择。 */
	TArray<FReEchoTraitCardOffer> GenerateForgeOffers();
	bool ApplyForgeChoice(FName ForgeId);

	/** 消耗时间碎片购买一次性本轮商品；成功后写入背包并立即应用构筑效果。 */
	UFUNCTION(BlueprintCallable)
	bool PurchaseShopItem(FName ItemId);

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
	 * With no unlocked specific replay, or with an empty selection, this falls back to the rolling
	 * latest echo; otherwise it returns the selected stored echoes in selection order.
	 */
	TArray<FReEchoRecording> ResolveReplayRecordings(int32 RequestedCount) const;

	/**
	 * Legacy compatibility facade for the pre-Plan30 single-echo query.
	 * It only forwards to ResolveReplayRecordings, so there is no second history authority.
	 * Plan30 replaces the remaining ReEchoGameMode call sites and then this facade is removed.
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
	FReEchoEncounterRuntimeState PendingEncounterResume;

	TSharedPtr<const FReEchoCsvDataSnapshot> RunDataSnapshot;

	void SetPhase(EReEchoRunPhase NewPhase);

	/** Resets every echo storage field to fresh-run defaults. */
	void ResetEchoStorage();

	/** Drops selections that no longer resolve, de-duplicates, then truncates to the replay limit. */
	void NormalizeSelectedReplayIds();

	int32 FindStoredEchoIndex(const FGuid& RecordingId) const;
};
