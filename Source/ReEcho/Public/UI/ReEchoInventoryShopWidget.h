#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReEchoInventoryShopWidget.generated.h"

class SWidget;
class UButton;
class UImage;
class UTextBlock;
class UTexture2D;
class UVerticalBox;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FReEchoInventoryShopClosed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FReEchoShopPurchaseRequested, FName, ItemId);

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

	void ShowInventory(int32 TimeShards, const TArray<FName>& OwnedItems);
	void ShowShop(int32 TimeShards, const TArray<FName>& OwnedItems);

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

	bool bShowingShop = false;
	int32 CurrentTimeShards = 0;
	TArray<FName> CurrentOwnedItems;
};
