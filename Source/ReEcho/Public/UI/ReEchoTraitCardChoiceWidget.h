#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/ReEchoTypes.h"
#include "ReEchoTraitCardChoiceWidget.generated.h"

class SWidget;
class UButton;
class UImage;
class USizeBox;
class UTextBlock;
class UTexture2D;
class UWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FReEchoTraitCardSelected, FName, CardId);

UCLASS()

class REECHO_API UReEchoTraitCardChoiceWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UReEchoTraitCardChoiceWidget(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(BlueprintAssignable)
	FReEchoTraitCardSelected OnCardSelected;

	void InitializeOffers(const TArray<FReEchoTraitCardOffer>& InOffers);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void BuildWidgetTree();
	void RefreshOffers();
	void ResetRevealAnimation();
	void SelectOffer(int32 OfferIndex);

	UFUNCTION()
	void HandleFirstCardClicked();

	UFUNCTION()
	void HandleSecondCardClicked();

	UFUNCTION()
	void HandleThirdCardClicked();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> CardButtons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USizeBox>> CardPanels;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> CardNames;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> CardDescriptions;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> NeedleWidget;

	UPROPERTY()
	TObjectPtr<UTexture2D> DrawBackgroundTexture;

	TArray<FReEchoTraitCardOffer> Offers;
	float RevealElapsed = 0.0f;
	bool bRevealComplete = false;
};