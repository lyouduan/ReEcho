#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReEchoRuneBackpackEntryWidget.generated.h"

class UImage;
class UTextBlock;
class UTexture2D;
class UReEchoIndexedButton;

/** Read-only presentation, never an inventory or synthesis rule. */
USTRUCT(BlueprintType)

struct FReEchoRuneBackpackEntryView
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rune")
	FText Name;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rune")
	int32 Count = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rune")
	TObjectPtr<UTexture2D> Icon = nullptr;
};

/** The Blueprint owns all layout and style, including its preview inside the panel. */
UCLASS()

class REECHO_API UReEchoRuneBackpackEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UReEchoRuneBackpackEntryWidget(const FObjectInitializer& ObjectInitializer);
	void Configure(const FReEchoRuneBackpackEntryView& View, int32 Index);
	UReEchoIndexedButton* GetSelectButton() const;

	UFUNCTION(BlueprintCallable, Category = "Shop|Rune Backpack", meta = (BlueprintInternalUseOnly = "true"))
	void ApplyDesignerPreviewSettings();

	/** Only used in Designer. Never grants or equips a rune. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Designer Preview")
	FReEchoRuneBackpackEntryView DesignerPreview;

protected:
	virtual void NativePreConstruct() override;

private:
	void ApplyContent(const FReEchoRuneBackpackEntryView& View);
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UReEchoIndexedButton> SelectButton;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> RuneIcon;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> NameText;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CountText;
	UPROPERTY(Transient)
	FReEchoRuneBackpackEntryView PendingView;
	int32 PendingIndex = INDEX_NONE;
};
