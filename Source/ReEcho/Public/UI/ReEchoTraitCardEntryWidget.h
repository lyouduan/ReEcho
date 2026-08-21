#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReEchoTraitCardEntryWidget.generated.h"

class UReEchoIndexedButton;
class UImage;
class UTextBlock;
class UTexture2D;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FReEchoTraitCardEntrySelected, int32, EntryIndex);

/** Designer-owned presentation for one runtime Trait offer. */
UCLASS()
class REECHO_API UReEchoTraitCardEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FReEchoTraitCardEntrySelected OnEntrySelected;

	void Configure(int32 InEntryIndex,
	               const FText& Kicker,
	               const FText& DisplayName,
	               const FText& Description,
	               const TArray<FName>& Tags,
	               const FLinearColor& CardColor,
	               UTexture2D* CardArt = nullptr,
	               UTexture2D* CardIcon = nullptr);
	void SetSelectionEnabled(bool bEnabled);
	void SetSelectedVisual(bool bSelected, bool bHasSelection);
	void FocusSelection();

protected:
	virtual void NativeConstruct() override;

private:
	UFUNCTION()
	void HandleIndexedClicked(int32 ClickedIndex);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UReEchoIndexedButton> SelectButton;

	/** Placeholder frame tinted with the runtime card palette so offers retain their existing identity. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> ArtCardFrame;

	/** Card art (main visual). Optional; hidden when no source texture is provided (Plan 69). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> ArtImage;

	/** Small corner icon. Optional; hidden when no source texture is provided (Plan 69). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> IconImage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> KickerText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DescriptionText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PrimaryTagText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SecondaryTagText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SelectHintText;

	int32 EntryIndex = INDEX_NONE;
};
