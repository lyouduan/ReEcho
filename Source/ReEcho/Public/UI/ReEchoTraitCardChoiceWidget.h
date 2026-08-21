#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/ReEchoTypes.h"
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

/** 展示三选一特质/锻造卡，并在揭示完成后接受一次选择。 */
UCLASS()

class REECHO_API UReEchoTraitCardChoiceWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UReEchoTraitCardChoiceWidget(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(BlueprintAssignable)
	FReEchoTraitCardSelected OnCardSelected;

	/** 装载本轮候选项并重置逐张揭示动画。 */
	void InitializeOffers(const TArray<FReEchoTraitCardOffer>& InOffers, int32 InTimeShards, bool bInForgeChoice);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	void BuildWidgetTree();
	void BuildCardEntries();
	void RefreshOffers();
	void RefreshSelectionVisuals();
	void ResetRevealAnimation();
	void SelectOffer(int32 OfferIndex);

	UFUNCTION()
	void HandleCardClicked(int32 OfferIndex);

	UFUNCTION()
	void HandleConfirmClicked();

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

	TArray<FReEchoTraitCardOffer> Offers;
	int32 CurrentTimeShards = 0;
	float RevealElapsed = 0.0f;
	int32 SelectedOfferIndex = INDEX_NONE;
	bool bForgeChoice = false;
	bool bRevealComplete = false;
};
