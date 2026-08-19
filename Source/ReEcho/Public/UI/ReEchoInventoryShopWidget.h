#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/ReEchoTypes.h"
#include "Run/ReEchoShopCatalog.h"
#include "ReEchoInventoryShopWidget.generated.h"

class SWidget;
class UButton;
class UCanvasPanel;
class UHorizontalBox;
class UImage;
class UReEchoIndexedButton;
class UScaleBox;
class UScrollBox;
class UTextBlock;
class UTexture2D;
class UVerticalBox;
class UWidget;
class UReEchoRunSubsystem;

DECLARE_MULTICAST_DELEGATE(FReEchoInventoryShopClosed);
DECLARE_MULTICAST_DELEGATE_OneParam(FReEchoShopPurchaseRequested, FName);
DECLARE_MULTICAST_DELEGATE(FReEchoShopRefreshRequested);
DECLARE_MULTICAST_DELEGATE_OneParam(FReEchoWeaponLoadoutSaveRequested, const TArray<FName>&);
DECLARE_MULTICAST_DELEGATE(FReEchoEchoCommandRequested);
DECLARE_MULTICAST_DELEGATE_OneParam(FReEchoEchoGuidCommandRequested, FGuid);
DECLARE_MULTICAST_DELEGATE_OneParam(FReEchoEchoSelectionCommandRequested, const TArray<FGuid>&);

UENUM(BlueprintType)
enum class EReEchoInventoryShopMode : uint8
{
	Inventory,
	ManualShop,
	PostTraitIntermission
};

UENUM()
enum class EReEchoShopEchoPendingDecision : uint8
{
	Undecided,
	Stored,
	Skipped,
	Replacing
};

/** 复用同一覆盖层展示背包或商店，并将购买请求交给运行系统处理。 */
UCLASS()

class REECHO_API UReEchoInventoryShopWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UReEchoInventoryShopWidget(const FObjectInitializer& ObjectInitializer);

	FReEchoInventoryShopClosed OnClosed;

	FReEchoShopPurchaseRequested OnPurchaseRequested;
	FReEchoShopRefreshRequested OnRefreshRequested;
	FReEchoWeaponLoadoutSaveRequested OnWeaponLoadoutSaveRequested;

	FReEchoEchoCommandRequested OnEchoStoreRequested;
	FReEchoEchoCommandRequested OnEchoSkipRequested;
	FReEchoEchoGuidCommandRequested OnEchoReplaceRequested;
	FReEchoEchoSelectionCommandRequested OnEchoSelectionRequested;
	FReEchoEchoCommandRequested OnEchoSkipAndCloseRequested;

	/** 以只读背包模式刷新当前资源与已拥有物品。 */
	void ShowInventory(int32 TimeShards, const TArray<FName>& OwnedItems);
	void SetWeaponPartShopView(const FReEchoWeaponPartShopView& PartShopView, bool bResetDraft = false);

	/** 以商店模式刷新报价；实际扣款由外部订阅者决定。 */
	void ShowShop(int32 TimeShards,
	              const TArray<FName>& OwnedItems,
	              float ShopDiscount = 0.0f,
	              int32 FreeRefreshes = 0,
	              bool bRefreshAllowed = true,
	              bool bExtraCardPurchaseAllowed = true,
	              int32 RefreshSequence = 0);

	/** Opens the same shop presentation with intermission-only echo management enabled. */
	void ShowPostTraitIntermission(int32 TimeShards,
	                               const TArray<FName>& OwnedItems,
	                               const FReEchoEchoStorageSummary& EchoSummary,
	                               float ShopDiscount = 0.0f,
	                               int32 FreeRefreshes = 0,
	                               bool bRefreshAllowed = true,
	                               bool bExtraCardPurchaseAllowed = true,
	                               int32 RefreshSequence = 0);
	void SetEchoSummary(const FReEchoEchoStorageSummary& EchoSummary);
	void ShowEchoStatus(const FText& Status);
	void EnterEchoReplacementMode();
	void CompletePostTraitClose();
	void RequestClose();

	EReEchoInventoryShopMode GetMode() const
	{
		return Mode;
	}

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	void BuildWidgetTree();
	void BuildShopLogicHost();
	void OrderShopLogicBlocks();
	void UpdateShopLogicViewportBounds();
	void BuildOfferEntries();
	void BuildLoadoutEntries();
	void BuildTargetShopPresentation();
	void BindDesignerLoadoutLayout();
	void RebuildTargetOfferRows();
	void RebuildOwnedCardSlots();
	void RebuildAttachmentHoverSlots();
	UTexture2D* ResolveWeaponPartIcon(FName PartId) const;
	UWidget* BuildSlotTooltip(const FReEchoShopOffer& Offer);
	bool HasEchoStorageCard() const;
	void
	AddTargetOfferCard(class UHorizontalBox* Row, const FReEchoShopOffer& Offer, int32 OfferIndex, bool bWeaponPart);
	void Refresh();
	void RequestPurchase(int32 OfferIndex);
	void ToggleDraftPart(int32 OwnedPartIndex);

	UFUNCTION()
	void HandleCloseClicked();

	UFUNCTION()
	void HandleOfferClicked(int32 OfferIndex);

	UFUNCTION()
	void HandleRefreshClicked();

	UFUNCTION()
	void HandleWeaponPartOfferClicked(int32 PartOfferIndex);

	UFUNCTION()
	void HandleSaveLoadoutClicked();

	UFUNCTION()
	void HandleEchoStorageCardSlotClicked();

	UFUNCTION()
	void HandleEchoPopupCloseClicked();

	// ---- echo management (inline, mirroring origin/main left-panel layout) ----
	void RefreshEchoState(UReEchoRunSubsystem* RunSubsystem);
	void RequestStoreEcho(UReEchoRunSubsystem* RunSubsystem);
	void RequestSkipEcho(UReEchoRunSubsystem* RunSubsystem);
	void RequestReplaceEcho(UReEchoRunSubsystem* RunSubsystem, FGuid TargetId);
	void RequestCancelReplaceEcho();
	void ToggleReplaySelection(UReEchoRunSubsystem* RunSubsystem, FGuid Id);
	void BuildEchoPanel();

	void HandleEchoStoreRequested();
	void HandleEchoSkipRequested();
	void HandleEchoReplaceRequested(FGuid RecordingId);
	void HandleEchoSelectionRequested(const TArray<FGuid>& RecordingIds);
	void HandleEchoSkipAndCloseRequested();

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
	void HandleReplaceSlotClicked(int32 SlotIndex);
	void HandleSelectSlotClicked(int32 SlotIndex);
	UFUNCTION()
	void HandleConfirmSkipContinueClicked();
	UFUNCTION()
	void HandleConfirmReturnClicked();

	UPROPERTY()
	TObjectPtr<UTexture2D> InventoryBackgroundTexture;

	UPROPERTY()
	TObjectPtr<UTexture2D> ShopBackgroundTexture;

	UPROPERTY()
	TObjectPtr<UTexture2D> ShopItemCardTexture;
	UPROPERTY()
	TObjectPtr<UTexture2D> ShopAttachmentSlotTexture;
	UPROPERTY()
	TMap<FName, TObjectPtr<UTexture2D>> WeaponPartIconTextures;
	UPROPERTY()
	TObjectPtr<UTexture2D> ShopCardIconTexture;
	UPROPERTY()
	TObjectPtr<UTexture2D> ShopBuyTexture;
	UPROPERTY()
	TObjectPtr<UTexture2D> ShopRefreshTexture;
	UPROPERTY()
	TObjectPtr<UTexture2D> ShopCardSlotTexture;
	UPROPERTY()
	TObjectPtr<UTexture2D> ShopSaveLoadoutTexture;
	UPROPERTY()
	TObjectPtr<UTexture2D> ShopTitleTexture;
	UPROPERTY()
	TObjectPtr<UTexture2D> ShopCurrencyFrameTexture;
	UPROPERTY()
	TObjectPtr<UTexture2D> WhiteTexture;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> BackgroundImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> InventoryPanel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> ShopPanel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CurrencyText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> InventoryText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> OfferContainer;

	/** Scrollable logic-only host used until the authored shop UI supplies its own layout. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> ShopLogicScrollBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> ShopLogicPanel;

	/** Logic-only blocks. Their names form the hand-off contract for a later authored UI. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> RunItemOfferPanel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> ShopControlPanel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> WeaponPartOfferPanel;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UReEchoIndexedButton>> OfferButtons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> OfferTexts;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ShopRefreshButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ShopRefreshText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ShopRuleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> WeaponLoadoutPanel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> WeaponLoadoutText;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UReEchoIndexedButton>> WeaponPartOfferButtons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> WeaponPartOfferTexts;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> SaveLoadoutButton;

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> ShopPresentationLayer;
	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> TargetPartOfferRow;
	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> TargetCardOfferRow;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> DesignerLoadoutCanvas;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TargetCurrencyText;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> DesignerAttachmentSlotButtons;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> DesignerAttachmentSlotArts;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> DesignerCardSlotButtons;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> DesignerCardSlotArts;
	UPROPERTY(Transient)
	TObjectPtr<UButton> TargetSaveLoadoutButton;

	TArray<FReEchoShopOffer> DisplayedOwnedCards;

	// ---- inline echo panel (origin/main layout) ----
	UPROPERTY(Transient)
	TObjectPtr<UScaleBox> EchoPanelScale;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> EchoPanel;
	UPROPERTY(Transient)
	TObjectPtr<UButton> EchoPopupCloseButton;
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
	TArray<TObjectPtr<UTextBlock>> EchoSlotTexts;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> EchoReplaceButtons;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> EchoSelectButtons;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> EchoSelectLabels;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> EchoSelectionText;
	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> CloseConfirmWidget;

	EReEchoInventoryShopMode Mode = EReEchoInventoryShopMode::Inventory;
	bool bCloseBroadcasted = false;
	bool bShowingShop = false;
	int32 CurrentTimeShards = 0;
	float CurrentShopDiscount = 0.0f;
	int32 CurrentFreeShopRefreshes = 0;
	bool bCurrentShopRefreshAllowed = true;
	bool bCurrentExtraCardPurchaseAllowed = true;
	int32 CurrentShopRefreshSequence = 0;
	TArray<FName> CurrentOwnedItems;
	FReEchoWeaponPartShopView CurrentPartShopView;
	TArray<FReEchoShopOffer> VisibleRunItemOffers;
	TArray<FReEchoShopOffer> VisibleWeaponPartOffers;
	TArray<FName> DraftPartIds;
	FName DraftWeaponId;

	// echo state mirrors
	UPROPERTY(Transient)
	TObjectPtr<UReEchoRunSubsystem> CachedRunSubsystem;
	FReEchoEchoStorageSummary EchoSummary;
	TArray<FGuid> EchoSelection;
	TArray<FGuid> EchoSlotGuids;
	EReEchoShopEchoPendingDecision EchoPendingDecision = EReEchoShopEchoPendingDecision::Undecided;
	bool bEchoStoragePopupOpen = false;
};
