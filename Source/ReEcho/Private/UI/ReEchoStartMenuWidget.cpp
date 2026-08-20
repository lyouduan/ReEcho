#include "UI/ReEchoStartMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/ReEchoIndexedButton.h"
#include "UI/ReEchoMenuWidgetHelpers.h"

namespace
{
enum class EStartMenuAction : int32
{
	Continue,
	NewGame,
	Settings,
	About,
	Quit
};
}

TSharedRef<SWidget> UReEchoStartMenuWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		BuildWidgetTree();
	}
	return Super::RebuildWidget();
}

void UReEchoStartMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	BuildWidgetTree();
	EnsureOpaqueBackground();

	if (ContinueButton)
	{
		ContinueButton->SetEntryIndex(static_cast<int32>(EStartMenuAction::Continue));
		ContinueButton->OnIndexedClicked.AddUniqueDynamic(this, &UReEchoStartMenuWidget::HandleMenuAction);
	}
	if (NewGameButton)
	{
		NewGameButton->SetEntryIndex(static_cast<int32>(EStartMenuAction::NewGame));
		NewGameButton->OnIndexedClicked.AddUniqueDynamic(this, &UReEchoStartMenuWidget::HandleMenuAction);
	}
	if (GameSettingsButton)
	{
		GameSettingsButton->SetEntryIndex(static_cast<int32>(EStartMenuAction::Settings));
		GameSettingsButton->OnIndexedClicked.AddUniqueDynamic(this, &UReEchoStartMenuWidget::HandleMenuAction);
	}
	if (AboutButton)
	{
		AboutButton->SetEntryIndex(static_cast<int32>(EStartMenuAction::About));
		AboutButton->OnIndexedClicked.AddUniqueDynamic(this, &UReEchoStartMenuWidget::HandleMenuAction);
	}
	if (QuitButton)
	{
		QuitButton->SetEntryIndex(static_cast<int32>(EStartMenuAction::Quit));
		QuitButton->OnIndexedClicked.AddUniqueDynamic(this, &UReEchoStartMenuWidget::HandleMenuAction);
	}
	RefreshMenu();
	if (bHasSavedRun && ContinueButton)
	{
		ContinueButton->SetKeyboardFocus();
	}
	else if (NewGameButton)
	{
		NewGameButton->SetKeyboardFocus();
	}
	else if (GameSettingsButton)
	{
		GameSettingsButton->SetKeyboardFocus();
	}
	else if (QuitButton)
	{
		QuitButton->SetKeyboardFocus();
	}
}

void UReEchoStartMenuWidget::InitializeMenu(const bool bInHasSavedRun)
{
	bHasSavedRun = bInHasSavedRun;
	RefreshMenu();
}

void UReEchoStartMenuWidget::EnsureOpaqueBackground()
{
	if (!WidgetTree)
	{
		return;
	}

	UPanelWidget* Root = Cast<UPanelWidget>(WidgetTree->RootWidget);
	if (!Root)
	{
		return;
	}

	UBorder* Background = NewObject<UBorder>(this, TEXT("OpaqueStartMenuBackground"));
	Background->SetBrushColor(FLinearColor(0.01f, 0.015f, 0.035f, 1.0f));

	Root->AddChild(Background);
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Background->Slot))
	{
		// Anchor to fill the entire viewport and draw behind every other element.
		CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		CanvasSlot->SetOffsets(FMargin(0.0f));
		CanvasSlot->SetZOrder(-100);
	}
}

void UReEchoStartMenuWidget::BuildWidgetTree()
{
	if (NewGameButton || !WidgetTree)
	{
		return;
	}

	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("StartBackground"));
	Background->SetBrushColor(FLinearColor(0.01f, 0.015f, 0.035f, 1.0f));
	Background->SetPadding(FMargin(140.0f));
	Background->SetHorizontalAlignment(HAlign_Center);
	Background->SetVerticalAlignment(VAlign_Center);
	WidgetTree->RootWidget = Background;

	UVerticalBox* Content =
	    WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("StartContent"));
	Background->SetContent(Content);

	UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StartTitle"));
	TitleText->SetText(NSLOCTEXT("ReEcho", "StartTitle", "时间回响"));
	TitleText->SetJustification(ETextJustify::Center);
	TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.45f, 0.88f, 1.0f)));
	FSlateFontInfo TitleFont = TitleText->GetFont();
	TitleFont.Size = 54;
	TitleText->SetFont(TitleFont);
	UVerticalBoxSlot* TitleSlot = Content->AddChildToVerticalBox(TitleText);
	TitleSlot->SetHorizontalAlignment(HAlign_Center);
	TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 20.0f));

	const ReEcho::UI::FMenuButtonStyle PrimaryButtonStyle{
	    FLinearColor(0.12f, 0.32f, 0.62f, 1.0f), FMargin(0.0f, 7.0f), FMargin(58.0f, 14.0f), 26};
	ReEcho::UI::FMenuButtonStyle ContinueButtonStyle = PrimaryButtonStyle;
	ContinueButtonStyle.Color = FLinearColor(0.08f, 0.42f, 0.32f, 1.0f);
	ContinueButton = ReEcho::UI::AddIndexedMenuButton(*WidgetTree,
	                                                  *Content,
	                                                  TEXT("ContinueButton"),
	                                                  FText::FromString(TEXT("继续游戏")),
	                                                  static_cast<int32>(EStartMenuAction::Continue),
	                                                  ContinueButtonStyle);
	NewGameButton = ReEcho::UI::AddIndexedMenuButton(*WidgetTree,
	                                                 *Content,
	                                                 TEXT("NewGameButton"),
	                                                 FText::FromString(TEXT("新开始游戏")),
	                                                 static_cast<int32>(EStartMenuAction::NewGame),
	                                                 PrimaryButtonStyle);
	GameSettingsButton = ReEcho::UI::AddIndexedMenuButton(*WidgetTree,
	                                                      *Content,
	                                                      TEXT("GameSettingsButton"),
	                                                      FText::FromString(TEXT("游戏设置")),
	                                                      static_cast<int32>(EStartMenuAction::Settings),
	                                                      PrimaryButtonStyle);
	AboutButton = ReEcho::UI::AddIndexedMenuButton(*WidgetTree,
	                                               *Content,
	                                               TEXT("AboutButton"),
	                                               FText::FromString(TEXT("关于我们")),
	                                               static_cast<int32>(EStartMenuAction::About),
	                                               PrimaryButtonStyle);
	QuitButton = ReEcho::UI::AddIndexedMenuButton(*WidgetTree,
	                                              *Content,
	                                              TEXT("QuitButton"),
	                                              FText::FromString(TEXT("退出")),
	                                              static_cast<int32>(EStartMenuAction::Quit),
	                                              PrimaryButtonStyle);
	RefreshMenu();
}

void UReEchoStartMenuWidget::RefreshMenu()
{
	if (ContinueButton)
	{
		ContinueButton->SetVisibility(ESlateVisibility::Visible);
	}
}

void UReEchoStartMenuWidget::HandleMenuAction(const int32 ActionIndex)
{
	switch (static_cast<EStartMenuAction>(ActionIndex))
	{
		case EStartMenuAction::Continue:
			OnContinueGameRequested.Broadcast();
			break;
		case EStartMenuAction::NewGame:
			OnNewGameRequested.Broadcast();
			break;
		case EStartMenuAction::Settings:
			OnGameSettingRequested.Broadcast();
			break;
		case EStartMenuAction::About:
			OnAboutRequested.Broadcast();
			break;
		case EStartMenuAction::Quit:
			OnQuitRequested.Broadcast();
			break;
		default:
			break;
	}
}
