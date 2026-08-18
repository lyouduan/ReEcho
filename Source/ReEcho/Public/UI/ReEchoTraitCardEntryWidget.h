#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReEchoTraitCardEntryWidget.generated.h"

class UReEchoIndexedButton;
class UImage;
class UTextBlock;

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
	               const FLinearColor& CardColor);
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

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> KickerText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DescriptionText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SelectHintText;

	int32 EntryIndex = INDEX_NONE;
};
