#include "UI/ReEchoTraitCardEntryWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "UI/ReEchoIndexedButton.h"

void UReEchoTraitCardEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SelectButton->OnIndexedClicked.AddUniqueDynamic(this, &UReEchoTraitCardEntryWidget::HandleIndexedClicked);
}

void UReEchoTraitCardEntryWidget::Configure(const int32 InEntryIndex,
                                            const FText& DisplayName,
                                            const FText& Description,
                                            UTexture2D* CardArt,
                                            UTexture2D* CardIcon)
{
	EntryIndex = InEntryIndex;
	if (ArtImage)
	{
		ArtImage->SetBrushFromTexture(CardArt);
		ArtImage->SetVisibility(CardArt ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (IconImage)
	{
		IconImage->SetBrushFromTexture(CardIcon);
		IconImage->SetVisibility(CardIcon ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
	SelectButton->SetEntryIndex(EntryIndex);
	SelectButton->SetBackgroundColor(FLinearColor::Transparent);
	NameText->SetText(DisplayName);
	DescriptionText->SetText(Description);
}

void UReEchoTraitCardEntryWidget::SetSelectedVisual(const bool bSelected, const bool bHasSelection)
{
	SetRenderOpacity(!bHasSelection || bSelected ? 1.0f : 0.38f);
	SetRenderScale(bSelected ? FVector2D(1.035f) : FVector2D(1.0f));
}

void UReEchoTraitCardEntryWidget::SetSelectionEnabled(const bool bEnabled)
{
	SelectButton->SetIsEnabled(bEnabled);
}

void UReEchoTraitCardEntryWidget::FocusSelection()
{
	SelectButton->SetKeyboardFocus();
}

void UReEchoTraitCardEntryWidget::HandleIndexedClicked(const int32 ClickedIndex)
{
	if (ClickedIndex == EntryIndex)
	{
		OnEntrySelected.Broadcast(EntryIndex);
	}
}
