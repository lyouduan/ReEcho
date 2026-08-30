#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReEchoShopTooltipWidget.generated.h"

class UTextBlock;

/** Authored item description and optional resolved-effect panel. Never owns gameplay state. */
UCLASS()

class REECHO_API UReEchoShopTooltipWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UReEchoShopTooltipWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Shop|Tooltip")
	void Configure(const FText& Title, const FText& Description, const FText& Outcome);

	UFUNCTION(BlueprintCallable, Category = "Shop|Tooltip", meta = (BlueprintInternalUseOnly = "true"))
	void ApplyDesignerPreviewSettings();

protected:
	virtual void NativePreConstruct() override;

private:
	void ApplyContent();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DescriptionText;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> OutcomePanel;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> OutcomeText;

	FText PendingTitle;
	FText PendingDescription;
	FText PendingOutcome;
};
