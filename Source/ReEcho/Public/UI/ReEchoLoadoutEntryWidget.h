#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReEchoLoadoutEntryWidget.generated.h"

class UImage;
class UReEchoIndexedButton;
class UReEchoLoadoutTooltipWidget;
class USizeBox;
class UTextBlock;
class UTexture2D;
class UWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FReEchoLoadoutEntrySelected, int32, EntryIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FReEchoLoadoutEntryPreviewed, int32, EntryIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FReEchoLoadoutEntryHovered, int32, EntryIndex);

/** Designer-owned presentation shared by character and weapon loadout entries. */
UCLASS()
class REECHO_API UReEchoLoadoutEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UReEchoLoadoutEntryWidget(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(BlueprintAssignable)
	FReEchoLoadoutEntrySelected OnEntrySelected;

	UPROPERTY(BlueprintAssignable)
	FReEchoLoadoutEntryPreviewed OnEntryPreviewed;

	UPROPERTY(BlueprintAssignable)
	FReEchoLoadoutEntryHovered OnEntryHovered;

	void Configure(int32 InEntryIndex,
	               const FText& Label,
	               const FText& Description,
	               const FString& SelectedTexturePath,
	               const FString& UnselectedTexturePath,
	               const FString& FallbackTexturePath,
	               float EntryWidth,
	               float EntryHeight);
	void SetPresentationState(bool bHasPreview, bool bIsPreviewed);
	void FocusSelection();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent) override;

private:
	UFUNCTION()
	void HandleIndexedClicked(int32 ClickedIndex);

	UFUNCTION()
	void HandleHovered();
	UWidget* BuildTooltip(const FText& Label, const FText& Description);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UReEchoIndexedButton> SelectButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> PortraitImage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> EntryRootSizeBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> PortraitSize;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> SelectedTexture;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> UnselectedTexture;

	UPROPERTY(EditDefaultsOnly, Category = "Loadout | Tooltip")
	TSubclassOf<UReEchoLoadoutTooltipWidget> TooltipWidgetClass;

	int32 EntryIndex = INDEX_NONE;
};
