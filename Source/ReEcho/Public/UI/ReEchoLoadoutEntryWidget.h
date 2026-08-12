#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReEchoLoadoutEntryWidget.generated.h"

class UImage;
class UReEchoIndexedButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FReEchoLoadoutEntrySelected, int32, EntryIndex);

/** Designer-owned presentation shared by character and weapon loadout entries. */
UCLASS()
class REECHO_API UReEchoLoadoutEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FReEchoLoadoutEntrySelected OnEntrySelected;

	void Configure(int32 InEntryIndex, const FText& Label, const FString& TexturePath);
	void SetSelected(bool bSelected);
	void FocusSelection();

protected:
	virtual void NativeConstruct() override;

private:
	UFUNCTION()
	void HandleIndexedClicked(int32 ClickedIndex);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UReEchoIndexedButton> SelectButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> PortraitImage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> NameText;

	int32 EntryIndex = INDEX_NONE;
};
