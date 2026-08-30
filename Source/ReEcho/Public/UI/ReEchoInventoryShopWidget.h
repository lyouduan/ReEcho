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
class UReEchoButtonVisualFeedback;
class UScaleBox;
class UScrollBox;
class USizeBox;
class UTextBlock;
class UTexture2D;
class UVerticalBox;
class UWidget;

DECLARE_MULTICAST_DELEGATE(FReEchoInventoryShopClosed);
DECLARE_MULTICAST_DELEGATE_OneParam(FReEchoShopPurchaseRequested, FName);
DECLARE_MULTICAST_DELEGATE_OneParam(FReEchoShopCardPackRequested, int32);
DECLARE_MULTICAST_DELEGATE_OneParam(FReEchoWeaponEquipRequested, FName);
/**
 * Raised when the player picks an already-owned rune from the rune backpack popup. This is a pure
 * re-equip intent and must never be routed through the purchase transaction.
 */
DECLARE_MULTICAST_DELEGATE_TwoParams(FReEchoOwnedPartEquipRequested, FName, int32);
DECLARE_MULTICAST_DELEGATE(FReEchoShopRefreshRequested);
DECLARE_MULTICAST_DELEGATE(FReEchoEchoCommandRequested);

UENUM(BlueprintType)
enum class EReEchoInventoryShopMode : uint8
{
	Inventory,
	ManualShop,
	PostTraitIntermission
};

/** 复用同一覆盖层展示背包或商店，并将购买请求交给运行系统处理。 */
UCLASS()

class REECHO_API UReEchoInventoryShopWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UReEchoInventoryShopWidget(const FObjectInitializer& ObjectInitializer);
	bool GetCardChoiceToShopCollapseTargetAbsolute(FVector2D& OutAbsoluteCenter) const;

	FReEchoInventoryShopClosed OnClosed;

	FReEchoShopPurchaseRequested OnPurchaseRequested;
	FReEchoShopCardPackRequested OnCardPackRequested;
	FReEchoWeaponEquipRequested OnWeaponEquipRequested;
	FReEchoOwnedPartEquipRequested OnOwnedPartEquipRequested;
	FReEchoShopRefreshRequested OnRefreshRequested;

	FReEchoEchoCommandRequested OnEchoStoreRequested;
	FReEchoEchoCommandRequested OnEchoSkipRequested;
	FReEchoEchoCommandRequested OnEchoSkipAndCloseRequested;

	/** 以只读背包模式刷新当前资源与已拥有物品。 */
	void ShowInventory(int32 TimeShards, const TArray<FName>& OwnedItems);
	void SetWeaponPartShopView(const FReEchoWeaponPartShopView& PartShopView,
	                          FName CharacterId = NAME_None);

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
	/** Resolves the authored echo-storage modal by stable Designer names and binds its interactions. */
	void BindEchoPresentation();
	void EnsureResponsiveLayout();
	UCanvasPanel* GetLayoutCanvas() const;
	void BuildShopLogicHost();
	void OrderShopLogicBlocks();
	void UpdateShopLogicViewportBounds();
	void BuildOfferEntries();
	void BuildLoadoutEntries();
	void BuildTargetShopPresentation();
	bool BindAuthoredShopPresentation();
	void BindSaveAndLeaveVisualFeedback();
	void BindDesignerLoadoutLayout();
	void RebuildTargetOfferRows();
	void RefreshAuthoredOfferCards();
	void RebuildOwnedCardSlots();
	/** Silently injects the selected character's passive as a synthetic "character card" into the owned-card
	 *  view so it shows in the right-side card panel; no grant popup, no RunSubsystem inventory mutation. */
	void InjectCharacterPassiveCardIntoOwnedView();
	void BindOwnedCardPagination();
	void UpdateOwnedCardPaginationControls();
	void RebuildAttachmentHoverSlots();
	void RebuildAttachmentSlotMapping();
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
	UFUNCTION()
	void HandleAttachmentSlot3Clicked();
	UFUNCTION()
	void HandleAttachmentSlot4Clicked();
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
	/** Creates (or reuses) the roman-numeral tier badge layered over a weapon-rune offer icon. */
	UTextBlock* EnsureRuneTierBadge(class UCanvasPanel* Card, class UImage* Icon, int32 Index);
	UWidget* BuildSlotTooltip(const FReEchoShopOffer& Offer);
	UWidget* BuildCardPackTooltip(const FReEchoShopCardPackOffer& Pack);
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
	void HandleOwnedCardPageLeftClicked();
	UFUNCTION()
	void HandleOwnedCardPageRightClicked();

	UFUNCTION()
	void HandleEchoPopupCloseClicked();

	// ---- single time-anchor management ----
	void BuildEchoPanel();

	UFUNCTION()
	void HandleStoreClicked();
	UFUNCTION()
	void HandleSkipClicked();
	UFUNCTION()
	void HandleConfirmSkipContinueClicked();
	UFUNCTION()
	void HandleConfirmReturnClicked();

	UPROPERTY()
	TObjectPtr<UTexture2D> ShopItemCardTexture;
	UPROPERTY()
	TObjectPtr<UTexture2D> ShopAttachmentSlotTexture;
	UPROPERTY()
	TObjectPtr<UTexture2D> ShopCoreAttachmentSlotTexture;
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
	/** Reviewed placeholder art used by the three card-pack offers. */
	UPROPERTY()
	TObjectPtr<UTexture2D> ShopEmptyCardSlotIconTexture;
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

	/** Keeps the authored save-and-leave art on the same hover scale contract as other menu buttons. */
	UPROPERTY(Transient)
	TObjectPtr<UReEchoButtonVisualFeedback> SaveAndLeaveVisualFeedback;

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

	// Runtime overlay text drawn on top of the cleaned refresh-button texture
	// (the baked-in "刷新" label was removed from the source PNG).
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ShopRefreshCountText;

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
	TArray<TObjectPtr<UCanvasPanel>> DesignerPartOfferCards;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> DesignerPartOfferIcons;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> DesignerPartOfferTierBadges;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> DesignerPartOfferDescriptions;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> DesignerPartOfferCosts;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UReEchoIndexedButton>> DesignerPartOfferBuyButtons;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> DesignerPartOfferBuyArts;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> DesignerPartOfferBuyLabels;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UCanvasPanel>> DesignerPackOfferCards;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> DesignerPackOfferBases;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> DesignerPackOfferIcons;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> DesignerPackOfferDescriptions;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> DesignerPackOfferCosts;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UReEchoIndexedButton>> DesignerPackOfferBuyButtons;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> DesignerPackOfferBuyArts;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> DesignerPackOfferBuyLabels;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> DesignerAttachmentSlotButtons;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> DesignerAttachmentSlotArts;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> DesignerStandardAttachmentSlotButtons;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> DesignerStandardAttachmentSlotArts;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> DesignerDualAttachmentSlotButtons;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> DesignerDualAttachmentSlotArts;
	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> DualAttachmentLayoutWidget;
	TArray<FName> DesignerAttachmentSlotTypeIds;
	TArray<int32> DesignerAttachmentSlotOccurrenceIndices;
	bool bUsingDualAttachmentLayout = false;
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
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DesignerCardPageCounter;
	UPROPERTY(Transient)
	TObjectPtr<UButton> DesignerCardPageLeftButton;
	UPROPERTY(Transient)
	TObjectPtr<UButton> DesignerCardPageRightButton;
	UPROPERTY(Transient)
	TObjectPtr<UReEchoButtonVisualFeedback> DesignerCardPageLeftVisualFeedback;
	UPROPERTY(Transient)
	TObjectPtr<UReEchoButtonVisualFeedback> DesignerCardPageRightVisualFeedback;

	TArray<FReEchoShopOffer> DisplayedOwnedCards;
	/** Stack count per entry in DisplayedOwnedCards; the same card obtained twice shows one icon with ×N. */
	TArray<int32> DisplayedOwnedCardCounts;
	int32 ActiveOwnedCardPage = 0;
	/** Creates (or reuses) the ×N stack-count badge layered over an owned-card slot icon. */
	UTextBlock* EnsureOwnedCardCountBadge(UWidget* Anchor, int32 Index);

	// ---- echo storage popup (authored WBP presentation with C++ fallback) ----
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UUserWidget> EchoStoragePopupWidget;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UScaleBox> EchoPanelScale;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> EchoPanel;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> EchoPopupCloseButton;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EchoCapacityText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EchoReplayModeText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EchoPendingInfoText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> EchoStoreButton;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EchoStoreLabel;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> EchoSkipButton;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> CloseConfirmWidget;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> EchoConfirmSkipContinue;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> EchoConfirmReturn;

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
	FName CurrentShopCharacterId = NAME_None;
	FReEchoWeaponPartShopView CurrentPartShopView;
	TArray<FReEchoShopOffer> VisibleRunItemOffers;
	TArray<FReEchoShopOffer> VisibleWeaponPartOffers;

	/** 本轮商店会话中已购买的报价 ItemId 集合；购买后标记槽位为“已购”，打开/刷新时清除。 */
	TSet<FName> PurchasedItemIds;

	// echo state mirrors
	FReEchoStatBlock CachedPlayerStats;
	FReEchoEchoStorageSummary EchoSummary;
	bool bEchoStoragePopupOpen = false;
};
