#include "UI/ReEchoAttackModeWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

TSharedRef<SWidget> UReEchoAttackModeWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		BuildWidgetTree();
	}
	return Super::RebuildWidget();
}

void UReEchoAttackModeWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildWidgetTree();
	if (AutomaticButton)
	{
		AutomaticButton->OnClicked.AddUniqueDynamic(this, &UReEchoAttackModeWidget::HandleAutomaticClicked);
	}
	if (ManualButton)
	{
		ManualButton->OnClicked.AddUniqueDynamic(this, &UReEchoAttackModeWidget::HandleManualClicked);
	}
	Refresh();
}

void UReEchoAttackModeWidget::SetAutomaticMode(const bool bAutomatic)
{
	bAutomaticMode = bAutomatic;
	Refresh();
}

void UReEchoAttackModeWidget::BuildWidgetTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}
	UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("AttackModeRoot"));
	WidgetTree->RootWidget = Root;
	CurrentModeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CurrentModeText"));
	CurrentModeText->SetJustification(ETextJustify::Center);
	Root->AddChildToVerticalBox(CurrentModeText)->SetPadding(FMargin(4.0f));

	UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ModeButtons"));
	Root->AddChildToVerticalBox(Buttons)->SetPadding(FMargin(4.0f));
	AutomaticButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("AutomaticButton"));
	UTextBlock* AutomaticText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AutomaticText"));
	AutomaticText->SetText(NSLOCTEXT("ReEcho", "AutomaticAttack", "Automatic Attack"));
	AutomaticButton->SetContent(AutomaticText);
	Buttons->AddChildToHorizontalBox(AutomaticButton);
	ManualButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ManualButton"));
	UTextBlock* ManualText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ManualText"));
	ManualText->SetText(NSLOCTEXT("ReEcho", "ManualAttack", "Manual Attack"));
	ManualButton->SetContent(ManualText);
	Buttons->AddChildToHorizontalBox(ManualButton);
}

void UReEchoAttackModeWidget::Refresh()
{
	if (CurrentModeText)
	{
		CurrentModeText->SetText(FText::Format(NSLOCTEXT("ReEcho", "CurrentAttackMode", "Current mode: {0}"),
		                                      bAutomaticMode ? NSLOCTEXT("ReEcho", "AutomaticMode", "Automatic")
		                                                     : NSLOCTEXT("ReEcho", "ManualMode", "Manual")));
	}
	if (AutomaticButton)
	{
		AutomaticButton->SetIsEnabled(!bAutomaticMode);
	}
	if (ManualButton)
	{
		ManualButton->SetIsEnabled(bAutomaticMode);
	}
}

void UReEchoAttackModeWidget::HandleAutomaticClicked()
{
	OnAutomaticRequested.Broadcast();
}

void UReEchoAttackModeWidget::HandleManualClicked()
{
	OnManualRequested.Broadcast();
}
