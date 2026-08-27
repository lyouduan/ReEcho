#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReEchoLoadoutTooltipWidget.generated.h"

class UTextBlock;

/** Designer-owned tooltip presentation for character and weapon loadout entries. */
UCLASS()
class REECHO_API UReEchoLoadoutTooltipWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UReEchoLoadoutTooltipWidget(const FObjectInitializer& ObjectInitializer);

	void Configure(const FText& InTitle, const FText& InDescription);

	/** Applies the compact content-sized preview used by the Widget Designer. */
	UFUNCTION(BlueprintCallable, Category = "Loadout|Tooltip", meta = (BlueprintInternalUseOnly = "true"))
	void ApplyDesignerPreviewSettings();

	/** Authoring audit hook for the otherwise editor-only preview mode. */
	UFUNCTION(BlueprintPure, Category = "Loadout|Tooltip", meta = (BlueprintInternalUseOnly = "true"))
	bool HasDesiredDesignerPreview() const;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativePreConstruct() override;

private:
	void BuildWidgetTree();
	void ApplyContent();

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DescriptionText;

	FText PendingTitle;
	FText PendingDescription;
};
