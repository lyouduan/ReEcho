#include "UI/ReEchoLoadoutEntryWidget.h"

#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "UI/ReEchoIndexedButton.h"

void UReEchoLoadoutEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SelectButton->OnIndexedClicked.AddUniqueDynamic(this, &UReEchoLoadoutEntryWidget::HandleIndexedClicked);
	SelectButton->OnHovered.AddUniqueDynamic(this, &UReEchoLoadoutEntryWidget::HandleHovered);
}

void UReEchoLoadoutEntryWidget::Configure(const int32 InEntryIndex,
                                          const FText& Label,
                                          const FString& SelectedTexturePath,
                                          const FString& UnselectedTexturePath,
                                          const FString& FallbackTexturePath,
                                          const float EntryWidth,
                                          const float EntryHeight)
{
	EntryIndex = InEntryIndex;
	SelectButton->SetEntryIndex(EntryIndex);
	NameText->SetText(Label);
	if (EntryRootSizeBox)
	{
		EntryRootSizeBox->SetWidthOverride(EntryWidth);
		EntryRootSizeBox->SetHeightOverride(EntryHeight);
	}
	if (PortraitSize)
	{
		PortraitSize->SetWidthOverride(EntryWidth);
		PortraitSize->SetHeightOverride(480.0f);
	}
	SelectedTexture = LoadObject<UTexture2D>(nullptr, *SelectedTexturePath);
	UnselectedTexture = LoadObject<UTexture2D>(nullptr, *UnselectedTexturePath);
	if (!SelectedTexture)
	{
		SelectedTexture = LoadObject<UTexture2D>(nullptr, *FallbackTexturePath);
	}
	if (!UnselectedTexture)
	{
		UnselectedTexture = SelectedTexture;
	}
	SetPresentationState(false, false);
}

void UReEchoLoadoutEntryWidget::SetPresentationState(const bool bHasPreview, const bool bIsPreviewed)
{
	UTexture2D* Texture = !bHasPreview || bIsPreviewed ? SelectedTexture : UnselectedTexture;
	if (Texture)
	{
		PortraitImage->SetBrushFromTexture(Texture, true);
	}
	NameText->SetColorAndOpacity(
	    FSlateColor(!bHasPreview || bIsPreviewed ? FLinearColor(1.0f, 0.96f, 0.88f, 1.0f)
	                                             : FLinearColor(0.46f, 0.43f, 0.37f, 1.0f)));
	SelectButton->SetBackgroundColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.0f));
}

void UReEchoLoadoutEntryWidget::FocusSelection()
{
	SelectButton->SetKeyboardFocus();
}

void UReEchoLoadoutEntryWidget::HandleIndexedClicked(const int32 ClickedIndex)
{
	if (ClickedIndex == EntryIndex)
	{
		OnEntryPreviewed.Broadcast(EntryIndex);
		OnEntrySelected.Broadcast(EntryIndex);
	}
}

void UReEchoLoadoutEntryWidget::HandleHovered()
{
	if (EntryIndex != INDEX_NONE)
	{
		OnEntryPreviewed.Broadcast(EntryIndex);
	}
}

void UReEchoLoadoutEntryWidget::NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnAddedToFocusPath(InFocusEvent);
	if (EntryIndex != INDEX_NONE)
	{
		OnEntryPreviewed.Broadcast(EntryIndex);
	}
}
