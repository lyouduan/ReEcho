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

	/** Applies the content-sized preview used when this Entry is opened on its own in the UMG Designer. */
	UFUNCTION(BlueprintCallable, Category = "Loadout|Entry", meta = (BlueprintInternalUseOnly = "true"))
	void ApplyDesignerPreviewSettings();

	/** Authoring audit hook for the otherwise editor-only preview mode. */
	UFUNCTION(BlueprintPure, Category = "Loadout|Entry", meta = (BlueprintInternalUseOnly = "true"))
	bool HasDesiredDesignerPreview() const;

#if WITH_EDITOR
	void ApplyDesignerPreviewState(bool bIsSelected, bool bShowSelectionArrow = false);
#endif

protected:
	virtual void NativeConstruct() override;
	virtual void NativePreConstruct() override;
	virtual void NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent) override;

private:
	UFUNCTION()
	void HandleIndexedClicked(int32 ClickedIndex);

	UFUNCTION()
	void HandleHovered();
	UWidget* BuildTooltip(const FText& Label, const FText& Description);
	void ApplyPortraitTexture(UTexture2D* Texture);
	void RefreshSelectionArrowBrushSize();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UReEchoIndexedButton> SelectButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> PortraitImage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> NameText;

	/** Lives beside SelectButton in the same Overlay so global hover feedback scales both together. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> EntrySelectionArrow;

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

#if WITH_EDITORONLY_DATA
	/** Representative values used only while this entry is rendered inside the UMG Designer. */
	UPROPERTY(EditAnywhere, Category = "Loadout | Designer Preview")
	FText DesignerPreviewLabel;

	UPROPERTY(EditAnywhere, Category = "Loadout | Designer Preview")
	TObjectPtr<UTexture2D> DesignerPreviewSelectedTexture;

	UPROPERTY(EditAnywhere, Category = "Loadout | Designer Preview")
	TObjectPtr<UTexture2D> DesignerPreviewUnselectedTexture;

	UPROPERTY(EditAnywhere, Category = "Loadout | Designer Preview")
	bool bDesignerPreviewSelected = true;

	UPROPERTY(EditAnywhere, Category = "Loadout | Designer Preview", meta = (ClampMin = "1.0"))
	float DesignerPreviewWidth = 390.0f;

	UPROPERTY(EditAnywhere, Category = "Loadout | Designer Preview", meta = (ClampMin = "1.0"))
	float DesignerPreviewHeight = 560.0f;
#endif

	int32 EntryIndex = INDEX_NONE;
};
