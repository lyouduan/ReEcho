#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Core/ReEchoTypes.h"
#include "ReEchoRunSubsystem.generated.h"

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

	UFUNCTION(BlueprintCallable)
	void BeginEncounter();

	UFUNCTION(BlueprintCallable)
	void SetEquippedWeapon(FName WeaponId);
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

	UFUNCTION(BlueprintCallable)
	void AddRecording(const FReEchoRecording& Recording);

	UFUNCTION(BlueprintCallable)
	void SetAnchor(FGuid RecordingId);

	UFUNCTION(BlueprintCallable)
	void ClearAnchor();

	UFUNCTION(BlueprintPure)
	TArray<FReEchoRecording> GetEchoRecordings(int32 RequestedCount) const;

	UFUNCTION(BlueprintPure)
	bool ShouldOpenShopAfterCurrentEncounter() const;

private:
	UPROPERTY()
	TArray<FReEchoRecording> RecordingHistory;

	UPROPERTY()
	FGuid AnchorId;

	UPROPERTY()
	TArray<FName> PendingTraitCardIds;

	void SetPhase(EReEchoRunPhase NewPhase);
};
