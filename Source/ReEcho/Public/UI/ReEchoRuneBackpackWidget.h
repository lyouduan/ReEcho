#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/ReEchoRuneBackpackEntryWidget.h"
#include "ReEchoRuneBackpackWidget.generated.h"

class UVerticalBox;

/** Authored rune list; Run and the shop adapter retain all gameplay authority. */
UCLASS()

class REECHO_API UReEchoRuneBackpackWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UReEchoRuneBackpackWidget(const FObjectInitializer& ObjectInitializer);
	void ConfigureEntries(const TArray<FReEchoRuneBackpackEntryView>& Views);
	TArray<UReEchoRuneBackpackEntryWidget*> GetEntryWidgets() const;

	UFUNCTION(BlueprintCallable, Category = "Shop|Rune Backpack", meta = (BlueprintInternalUseOnly = "true"))
	void ApplyDesignerPreviewSettings();

	/** Used only to append rows when real data exceeds the authored examples. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shop|Rune Backpack")
	TSubclassOf<UReEchoRuneBackpackEntryWidget> EntryWidgetClass;

protected:
	virtual void NativePreConstruct() override;

private:
	void ApplyEntries();
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> EntryList;
	UPROPERTY(Transient)
	TArray<FReEchoRuneBackpackEntryView> PendingViews;
};
