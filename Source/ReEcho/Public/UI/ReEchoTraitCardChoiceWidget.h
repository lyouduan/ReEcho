#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/ReEchoTypes.h"
#include "Run/ReEchoShopCatalog.h"
#include "ReEchoTraitCardChoiceWidget.generated.h"

class SWidget;
class UButton;
class UCanvasPanel;
class UImage;
class UReEchoIndexedButton;
class UReEchoTraitCardEntryWidget;
class USizeBox;
class UTextBlock;
class UTexture2D;
class UWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FReEchoTraitCardSelected, FName, CardId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FReEchoShopCardChoiceSelected, FName, ItemId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FReEchoCardSlotRefreshRequested, int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FReEchoShopCardChoiceCancelled);

/** 展示三选一特质/锻造卡，并在揭示完成后接受一次选择。 */
UCLASS()

class REECHO_API UReEchoTraitCardChoiceWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UReEchoTraitCardChoiceWidget(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(BlueprintAssignable)
	FReEchoTraitCardSelected OnCardSelected;
	UPROPERTY(BlueprintAssignable)
	FReEchoShopCardChoiceSelected OnShopCardSelected;
	UPROPERTY(BlueprintAssignable)
	FReEchoCardSlotRefreshRequested OnCardSlotRefreshRequested;
	UPROPERTY(BlueprintAssignable)
	FReEchoShopCardChoiceCancelled OnShopChoiceCancelled;

	/** 装载本轮候选项。默认重置全部揭示；刷新成功时只重播指定稳定槽位。 */
	void InitializeOffers(const TArray<FReEchoTraitCardOffer>& InOffers,
	                      int32 InTimeShards,
	                      int32 RefreshedSlotIndex = INDEX_NONE);
	/** Reuses the reveal/selection layout to claim from an already-paid, same-tier shop-card pack. */
	void InitializeShopOffers(const TArray<FReEchoShopCardChoiceOffer>& InOffers,
	                          int32 InTimeShards,
	                          int32 Tier,
	                          int32 RefreshedSlotIndex = INDEX_NONE);
	/** Re-enables the exact same choices after a rejected purchase or refresh transaction. */
	void RestoreChoiceFailure(int32 InTimeShards);
#if WITH_DEV_AUTOMATION_TESTS
	void AdvanceRevealAnimationForTesting(float DeltaSeconds);
#endif

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	void BuildWidgetTree();
	void EnsureShopCancelButton();
	void BuildCardEntries();
	void RefreshOffers();
	void RefreshSelectionVisuals();

	/** UI art contract for trait cards (Plan 69). Mirrors the weapon-texture resolution convention:
	    resolve a per-card texture path from the CardId, falling back to a generic icon when absent. */
	static FString ResolveCardArtTexturePath(int32 Tier);
	static FString ResolveCardIconTexturePath(FName CardId);
	void AdvanceRevealAnimation(float DeltaSeconds);
	void ResetRevealAnimation();
	void ResetRevealAnimationForSlot(int32 SlotIndex);
	void SelectOffer(int32 OfferIndex);
	bool CanSelectOffer(int32 OfferIndex) const;

	UFUNCTION()
	void HandleCardClicked(int32 OfferIndex);
	UFUNCTION()
	void HandleCardRefreshClicked(int32 OfferIndex);

	UFUNCTION()
	void HandleConfirmClicked();
	UFUNCTION()
	void HandleShopCancelClicked();

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> TraitCardContainer;

	/** Designer-owned card frames. Their SizeBox dimensions are the runtime card dimensions. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> TraitCardSlot0;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> TraitCardSlot1;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> TraitCardSlot2;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UReEchoIndexedButton>> CardButtons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UReEchoIndexedButton>> CardRefreshButtons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> CardRefreshTexts;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UReEchoTraitCardEntryWidget>> CardEntries;

	UPROPERTY()
	TSubclassOf<UReEchoTraitCardEntryWidget> CardEntryWidgetClass;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USizeBox>> CardPanels;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> CardNames;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> CardDescriptions;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SubtitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CurrencyText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> NeedleWidget;

	/** Final-effect flow: selecting a card previews it; this button commits the choice. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ConfirmButton;
	UPROPERTY(Transient)
	TObjectPtr<UButton> ShopCancelButton;

	TArray<FReEchoTraitCardOffer> Offers;
	TArray<FReEchoShopCardChoiceOffer> ShopOffers;
	int32 CurrentTimeShards = 0;
	int32 ShopTier = 0;
	float RevealElapsed = 0.0f;
	int32 SelectedOfferIndex = INDEX_NONE;
	int32 RevealingCardIndex = INDEX_NONE;
	bool bRevealComplete = false;
	bool bShopMode = false;
};
