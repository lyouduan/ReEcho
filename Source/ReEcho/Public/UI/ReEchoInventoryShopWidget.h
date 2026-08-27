#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/ReEchoTypes.h"
#include "Run/ReEchoShopCatalog.h"
#include "Combat/ReEchoCombatTypes.h"
#include "ReEchoInventoryShopWidget.generated.h"

class SWidget;
class UButton;
class UCanvasPanel;
class UHorizontalBox;
class UImage;
class UReEchoIndexedButton;
class UScaleBox;
class UScrollBox;
class USizeBox;
class UTextBlock;
class UTexture2D;
class UVerticalBox;
class UWidget;
class UReEchoRunSubsystem;

DECLARE_MULTICAST_DELEGATE(FReEchoInventoryShopClosed);
DECLARE_MULTICAST_DELEGATE_OneParam(FReEchoShopPurchaseRequested, FName);
DECLARE_MULTICAST_DELEGATE_OneParam(FReEchoShopCardPackRequested, int32);
DECLARE_MULTICAST_DELEGATE_OneParam(FReEchoWeaponEquipRequested, FName);
DECLARE_MULTICAST_DELEGATE(FReEchoShopRefreshRequested);
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
	FReEchoShopCardPackRequested OnCardPackRequested;
	FReEchoWeaponEquipRequested OnWeaponEquipRequested;
	FReEchoShopRefreshRequested OnRefreshRequested;

	FReEchoEchoCommandRequested OnEchoStoreRequested;
	FReEchoEchoCommandRequested OnEchoSkipRequested;
	FReEchoEchoGuidCommandRequested OnEchoReplaceRequested;
	FReEchoEchoSelectionCommandRequested OnEchoSelectionRequested;
	FReEchoEchoCommandRequested OnEchoSkipAndCloseRequested;

	/** 以只读背包模式刷新当前资源与已拥有物品。 */
	void ShowInventory(int32 TimeShards, const TArray<FName>& OwnedItems);
	void SetWeaponPartShopView(const FReEchoWeaponPartShopView& PartShopView);

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

	/** 注入当前玩家属性块，用于 DesignerShopClock 悬停属性面板。 */
	void SetPlayerStats(const FReEchoStatBlock& Stats);

	/** 更新时间碎片显示（购买后扣费，不重摇报价）。 */
	void SetTimeShards(int32 NewShards);
	/** 按 ItemId 标记某报价槽位为已购；构筑卡同步进入右侧卡牌槽，不触发重摇。 */
	void MarkItemPurchased(FName ItemId);

	EReEchoInventoryShopMode GetMode() const
	{
		return Mode;
	}

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	void BuildWidgetTree();
	void EnsureResponsiveLayout();
	UCanvasPanel* GetLayoutCanvas() const;
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
	void RebuildEquippedWeaponDisplay();
	void UpdateWeaponLoadoutText();
	int32 GetDisplayedTimeShardBalance() const;
	FName GetSlotTypeIdForIndex(int32 SlotIndex) const;
	const FReEchoShopOffer* FindOwnedPartByContentId(FName ContentId) const;
	void HandleAttachmentSlotClicked(int32 SlotIndex);
	UCanvasPanel* EnsureBackpackPopupLayer();
	FVector2D ResolveBackpackPopupPosition(const UWidget* AnchorWidget,
	                                       const FVector2D& PopupSize,
	                                       const FVector2D& FallbackPosition) const;
	UFUNCTION()
	void HandleAttachmentSlot0Clicked();
	UFUNCTION()
	void HandleAttachmentSlot1Clicked();
	UFUNCTION()
	void HandleAttachmentSlot2Clicked();
	void BuildBackpackPopup(int32 SlotIndex);
	void HideBackpackPopup();
	UFUNCTION()
	void HandleBackpackItemClicked(int32 ItemIndex);
	UFUNCTION()
	void HandleEquippedWeaponClicked();
	void BuildWeaponBackpackPopup();
	void HideWeaponBackpackPopup();
	UFUNCTION()
	void HandleWeaponBackpackItemClicked(int32 ItemIndex);
	UTexture2D* ResolveWeaponPartIcon(FName PartId) const;
	UWidget* BuildSlotTooltip(const FReEchoShopOffer& Offer);
	UWidget* BuildAttributePanel(const FReEchoStatBlock& Stats) const;
	bool HasEchoStorageCard() const;
	void
	AddTargetOfferCard(class UHorizontalBox* Row, const FReEchoShopOffer& Offer, int32 OfferIndex, bool bWeaponPart);
	void AddTargetCardPack(class UHorizontalBox* Row, const FReEchoShopCardPackOffer& Pack, int32 PackIndex);
	void Refresh();
	void RequestPurchase(int32 OfferIndex);
	bool IsPartEquipped(FName PartId) const;
	bool IsPartCompatibleWithCurrentWeapon(const FReEchoShopOffer& PartOffer) const;

	UFUNCTION()
	void HandleCloseClicked();

	UFUNCTION()
	void HandleOfferClicked(int32 OfferIndex);
	UFUNCTION()
	void HandleCardPackClicked(int32 PackIndex);

	UFUNCTION()
	void HandleRefreshClicked();

	UFUNCTION()
	void HandleWeaponPartOfferClicked(int32 PartOfferIndex);

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
	TObjectPtr<UTexture2D> ShopTitleTexture;
	UPROPERTY()
	TObjectPtr<UTexture2D> ShopCurrencyFrameTexture;
	UPROPERTY()
	TObjectPtr<UTexture2D> WhiteTexture;

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
	UPROPERTY(Transient)
	TArray<TObjectPtr<UReEchoIndexedButton>> CardPackButtons;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> CardPackTexts;

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

	UPROPERTY(Transient)
	TObjectPtr<UScaleBox> ResponsiveContentScale;
	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> ResponsiveContentCanvas;

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> ShopPresentationLayer;
	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> TargetPartOfferRow;
	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> TargetCardOfferRow;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> DesignerLoadoutCanvas;
	UPROPERTY(Transient)
	TObjectPtr<UImage> DesignerWeaponPanelWidget;
	UPROPERTY(Transient)
	TObjectPtr<UButton> DesignerEquippedWeaponButton;
	UPROPERTY(Transient)
	TObjectPtr<UImage> DesignerEquippedWeaponArt;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TargetCurrencyText;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TargetRefreshLimitText;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> DesignerAttachmentSlotButtons;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> DesignerAttachmentSlotArts;
	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> BackpackPopupLayer;
	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> BackpackPopupPanel;
	int32 ActiveBackpackSlotIndex = INDEX_NONE;
	TArray<FName> CachedBackpackItemIds;
	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> WeaponBackpackPopupPanel;
	TArray<FName> CachedWeaponBackpackIds;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> DesignerCardSlotButtons;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> DesignerCardSlotArts;

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

	/** 本轮商店会话中已购买的报价 ItemId 集合；购买后标记槽位为“已购”，打开/刷新时清除。 */
	TSet<FName> PurchasedItemIds;

	// echo state mirrors
	UPROPERTY(Transient)
	TObjectPtr<UReEchoRunSubsystem> CachedRunSubsystem;
	FReEchoStatBlock CachedPlayerStats;
	FReEchoEchoStorageSummary EchoSummary;
	TArray<FGuid> EchoSelection;
	TArray<FGuid> EchoSlotGuids;
	EReEchoShopEchoPendingDecision EchoPendingDecision = EReEchoShopEchoPendingDecision::Undecided;
	bool bEchoStoragePopupOpen = false;
};
