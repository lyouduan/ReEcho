#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/ReEchoAttributeRowWidget.h"
#include "ReEchoAttributeTooltipWidget.generated.h"

class UVerticalBox;

/** Authored attribute panel. Reuses preview rows and grows only if the data catalog grows. */
UCLASS()

class REECHO_API UReEchoAttributeTooltipWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UReEchoAttributeTooltipWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Shop|Attributes")
	void ConfigureRows(const TArray<FReEchoAttributeRowView>& Rows);

	UFUNCTION(BlueprintCallable, Category = "Shop|Attributes", meta = (BlueprintInternalUseOnly = "true"))
	void ApplyDesignerPreviewSettings();

protected:
	virtual void NativePreConstruct() override;

	/** Only needed when the CSV has more rows than the authored preview list. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shop|Attributes")
	TSubclassOf<UReEchoAttributeRowWidget> AttributeRowWidgetClass;

private:
	void ApplyRows();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> AttributeRows;
	UPROPERTY(Transient)
	TArray<FReEchoAttributeRowView> PendingRows;
};
