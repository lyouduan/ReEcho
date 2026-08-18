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
                                            const FText& Kicker,
                                            const FText& DisplayName,
                                            const FText& Description,
                                            const FLinearColor& CardColor)
{
	EntryIndex = InEntryIndex;
	SelectButton->SetEntryIndex(EntryIndex);
	SelectButton->SetBackgroundColor(FLinearColor::White);
	if (ArtCardFrame)
	{
		// The supplied card frame is a white/grey layout placeholder. Preserve the
		// established runtime card palette instead of presenting every offer as white.
		FLinearColor FrameTint = FLinearColor::LerpUsingHSV(FLinearColor::White, CardColor, 0.38f);
		FrameTint.A = 1.0f;
		ArtCardFrame->SetColorAndOpacity(FrameTint);
	}
	KickerText->SetText(Kicker);
	NameText->SetText(DisplayName);
	DescriptionText->SetText(Description);
	SelectHintText->SetText(NSLOCTEXT("ReEcho", "TraitCardSelectHint", "点击选择 · 确认后不可撤回"));
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
