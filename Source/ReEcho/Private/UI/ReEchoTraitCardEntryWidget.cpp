#include "UI/ReEchoTraitCardEntryWidget.h"

#include "Components/TextBlock.h"
#include "UI/ReEchoIndexedButton.h"

void UReEchoTraitCardEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SelectButton->OnIndexedClicked.AddUniqueDynamic(this, &UReEchoTraitCardEntryWidget::HandleIndexedClicked);
}

void UReEchoTraitCardEntryWidget::Configure(const int32 InEntryIndex,
                                            const FText& Kicker,
                                            const FText& DisplayName,
                                            const FText& Description,
                                            const FLinearColor& CardColor)
{
	EntryIndex = InEntryIndex;
	SelectButton->SetEntryIndex(EntryIndex);
	SelectButton->SetBackgroundColor(CardColor);
	KickerText->SetText(Kicker);
	NameText->SetText(DisplayName);
	DescriptionText->SetText(Description);
	SelectHintText->SetText(NSLOCTEXT("ReEcho", "TraitCardSelectHint", "点击选择 · 确认后不可撤回"));
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
