#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReEchoAttributeRowWidget.generated.h"

class UImage;
class UTextBlock;
class UTexture2D;

/** Already formatted read-only data; no attribute rules or formatting live in the row. */
USTRUCT(BlueprintType)

struct FReEchoAttributeRowView
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute")
	FText Name;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute")
	FText Value;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute")
	TObjectPtr<UTexture2D> Icon = nullptr;
};

/** Reusable Designer-owned attribute row, also previewed inside the complete tooltip. */
UCLASS()

class REECHO_API UReEchoAttributeRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UReEchoAttributeRowWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Shop|Attributes")
	void Configure(const FReEchoAttributeRowView& Row);

	UFUNCTION(BlueprintCallable, Category = "Shop|Attributes", meta = (BlueprintInternalUseOnly = "true"))
	void ApplyDesignerPreviewSettings();

	/** Sample data only. Never used for a live player's attributes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Designer Preview")
	FReEchoAttributeRowView DesignerPreview;

protected:
	virtual void NativePreConstruct() override;

private:
	void ApplyContent(const FReEchoAttributeRowView& Row);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> AttributeIcon;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> AttributeNameText;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> AttributeValueText;
	UPROPERTY(Transient)
	FReEchoAttributeRowView PendingRow;
};
