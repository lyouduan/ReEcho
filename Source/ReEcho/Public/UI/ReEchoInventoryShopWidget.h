#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/ReEchoTypes.h"
#include "ReEchoInventoryShopWidget.generated.h"

class SWidget;
class UButton;
class UImage;
class UTextBlock;
class UTexture2D;
class UVerticalBox;
class UReEchoRunSubsystem;

/** Widget-side lifecycle of the pending recording decision shown in the shop. */
enum class EReEchoShopEchoPendingDecision : uint8
{
	Undecided,
	Stored,
	Skipped,
	Replacing
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FReEchoInventoryShopClosed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FReEchoShopPurchaseRequested, FName, ItemId);

/** 复用同一覆盖层展示背包或商店，并将购买请求交给运行系统处理。 */
UCLASS()

class REECHO_API UReEchoInventoryShopWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UReEchoInventoryShopWidget(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(BlueprintAssignable)
	FReEchoInventoryShopClosed OnClosed;

	UPROPERTY(BlueprintAssignable)
	FReEchoShopPurchaseRequested OnPurchaseRequested;

	/** 以只读背包模式刷新当前资源与已拥有物品。 */
	void ShowInventory(int32 TimeShards, const TArray<FName>& OwnedItems);

	/** 以商店模式刷新报价；实际扣款由外部订阅者决定。 */
	void ShowShop(int32 TimeShards, const TArray<FName>& OwnedItems);

	// ---- Plan31: echo storage / replay selection management ----
	// The widget only ever holds the read-only summary projection and stable GUIDs.
	// It never owns a full recording, writes save state, or indexes mutable backend arrays.
	// The GameMode injects the authoritative RunSubsystem (the bridge); every command goes
	// through Plan29's transactional store/skip/replace/select commands.

	/** Re-read the authoritative echo summary from RunSubsystem and rebuild the echo panel. */
	void RefreshEchoState(UReEchoRunSubsystem* RunSubsystem);

	/** Store the pending recording into a free slot, or enter replacement mode when storage is full. */
	void RequestStoreEcho(UReEchoRunSubsystem* RunSubsystem);

	/** Skip the pending recording (drops only the storage decision; the rolling latest echo stays). */
	void RequestSkipEcho(UReEchoRunSubsystem* RunSubsystem);

	/** Store the pending recording by replacing the stored echo identified by TargetId. */
	void RequestReplaceEcho(UReEchoRunSubsystem* RunSubsystem, FGuid TargetId);

	/** Leave replacement mode and return to the pending Store/Skip selection. */
	void RequestCancelReplaceEcho();

	/** Toggle a stored echo in/out of the next-encounter replay selection, honouring SpecificReplayLimit. */
	void ToggleReplaySelection(UReEchoRunSubsystem* RunSubsystem, FGuid Id);

	/** Close intent: gate on an undecided pending recording before broadcasting OnClosed. */
	void RequestClose();

	bool IsPendingEchoDecided() const { return !EchoSummary.bHasPendingRecording; }
	const FReEchoEchoStorageSummary& GetEchoSummary() const { return EchoSummary; }
	EReEchoShopEchoPendingDecision GetEchoPendingDecision() const { return EchoPendingDecision; }
	UReEchoRunSubsystem* GetCachedRunSubsystem() const { return CachedRunSubsystem; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	void BuildWidgetTree();
	void Refresh();
	void RequestPurchase(int32 OfferIndex);

	UFUNCTION()
	void HandleCloseClicked();

	UFUNCTION()
	void HandleFirstOfferClicked();

	UFUNCTION()
	void HandleSecondOfferClicked();

	UFUNCTION()
	void HandleThirdOfferClicked();

	UFUNCTION()
	void HandleFourthOfferClicked();

	// ---- Plan31 echo management handlers ----
	UFUNCTION()
	void HandleStoreClicked();

	UFUNCTION()
	void HandleSkipClicked();

	UFUNCTION()
	void HandleCancelReplaceClicked();

	UFUNCTION()
	void HandleReplaceSlot0Clicked();

	UFUNCTION()
	void HandleReplaceSlot1Clicked();

	UFUNCTION()
	void HandleReplaceSlot2Clicked();

	UFUNCTION()
	void HandleSelectSlot0Clicked();

	UFUNCTION()
	void HandleSelectSlot1Clicked();

	UFUNCTION()
	void HandleSelectSlot2Clicked();

	UFUNCTION()
	void HandleConfirmSkipContinueClicked();

	UFUNCTION()
	void HandleConfirmReturnClicked();

	void HandleReplaceSlotClicked(int32 SlotIndex);
	void HandleSelectSlotClicked(int32 SlotIndex);
	void BuildEchoPanel();

	UPROPERTY()
	TObjectPtr<UTexture2D> InventoryBackgroundTexture;

	UPROPERTY()
	TObjectPtr<UTexture2D> ShopBackgroundTexture;

	UPROPERTY(Transient)
	TObjectPtr<UImage> BackgroundImage;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> InventoryPanel;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> ShopPanel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CurrencyText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> InventoryText;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> OfferButtons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> OfferTexts;

	// ---- Plan31 echo management state (summary + stable GUIDs only) ----
	FReEchoEchoStorageSummary EchoSummary;
	TArray<FGuid> EchoSelection;
	TArray<FGuid> EchoSlotGuids; // slot index -> stored echo GUID, reset every refresh
	EReEchoShopEchoPendingDecision EchoPendingDecision = EReEchoShopEchoPendingDecision::Undecided;
	UPROPERTY()
	UReEchoRunSubsystem* CachedRunSubsystem = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> EchoPanel;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> CloseConfirmWidget;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> EchoCapacityText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> EchoReplayModeText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> EchoPendingInfoText;

	UPROPERTY(Transient)
	TObjectPtr<UButton> EchoStoreButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> EchoSkipButton;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> EchoReplaceInstructionText;

	UPROPERTY(Transient)
	TObjectPtr<UButton> EchoCancelReplaceButton;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> EchoReplaceButtons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> EchoSelectButtons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> EchoSlotTexts;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> EchoSelectLabels;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> EchoSelectionText;

	bool bShowingShop = false;
	int32 CurrentTimeShards = 0;
	TArray<FName> CurrentOwnedItems;
};
