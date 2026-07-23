#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/ReEchoTypes.h"
#include "ReEchoTraitCardChoiceWidget.generated.h"

class SWidget;
class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FReEchoTraitCardSelected, FName, CardId);

UCLASS()
class REECHO_API UReEchoTraitCardChoiceWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FReEchoTraitCardSelected OnCardSelected;

	void InitializeOffers(const TArray<FReEchoTraitCardOffer>& InOffers);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	void BuildWidgetTree();
	void RefreshOffers();
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
	TArray<TObjectPtr<UTextBlock>> CardNames;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> CardDescriptions;

	TArray<FReEchoTraitCardOffer> Offers;
};