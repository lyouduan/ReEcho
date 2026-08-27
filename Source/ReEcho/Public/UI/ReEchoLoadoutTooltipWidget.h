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
	void Configure(const FText& InTitle, const FText& InDescription);

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
