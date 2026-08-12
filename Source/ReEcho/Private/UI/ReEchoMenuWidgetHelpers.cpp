#include "UI/ReEchoMenuWidgetHelpers.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/ReEchoIndexedButton.h"

namespace ReEcho::UI
{
namespace
{
void ConfigureMenuButton(
    UWidgetTree& WidgetTree, UVerticalBox& Content, UButton& Button, const FText& Label, const FMenuButtonStyle& Style)
{
	Button.SetBackgroundColor(Style.Color);
	UVerticalBoxSlot* ButtonSlot = Content.AddChildToVerticalBox(&Button);
	ButtonSlot->SetHorizontalAlignment(HAlign_Center);
	ButtonSlot->SetPadding(Style.SlotPadding);

	UTextBlock* ButtonLabel = WidgetTree.ConstructWidget<UTextBlock>();
	ButtonLabel->SetText(Label);
	ButtonLabel->SetJustification(ETextJustify::Center);
	ButtonLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	ButtonLabel->SetMargin(Style.LabelMargin);
	FSlateFontInfo ButtonFont = ButtonLabel->GetFont();
	ButtonFont.Size = Style.FontSize;
	ButtonLabel->SetFont(ButtonFont);
	Button.SetContent(ButtonLabel);
}
}

UButton* AddMenuButton(UWidgetTree& WidgetTree,
                       UVerticalBox& Content,
                       const FName ButtonName,
                       const FText& Label,
                       const FMenuButtonStyle& Style)
{
	UButton* Button = WidgetTree.ConstructWidget<UButton>(UButton::StaticClass(), ButtonName);
	ConfigureMenuButton(WidgetTree, Content, *Button, Label, Style);
	return Button;
}

UReEchoIndexedButton* AddIndexedMenuButton(UWidgetTree& WidgetTree,
                                           UVerticalBox& Content,
                                           const FName ButtonName,
                                           const FText& Label,
                                           const int32 EntryIndex,
                                           const FMenuButtonStyle& Style)
{
	UReEchoIndexedButton* Button =
	    WidgetTree.ConstructWidget<UReEchoIndexedButton>(UReEchoIndexedButton::StaticClass(), ButtonName);
	Button->SetEntryIndex(EntryIndex);
	ConfigureMenuButton(WidgetTree, Content, *Button, Label, Style);
	return Button;
}
}
