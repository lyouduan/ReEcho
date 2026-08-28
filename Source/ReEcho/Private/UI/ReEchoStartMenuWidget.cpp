#include "UI/ReEchoStartMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "ImageUtils.h"
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

constexpr int32 CloseSaveRollbackAction = 90;
constexpr int32 SaveSlotActionBase = 100;
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
	if (UReEchoIndexedButton* CloseButton =
	        Cast<UReEchoIndexedButton>(WidgetTree->FindWidget(TEXT("SaveRollbackCloseButton"))))
	{
		CloseButton->SetEntryIndex(CloseSaveRollbackAction);
		CloseButton->OnIndexedClicked.AddUniqueDynamic(this, &UReEchoStartMenuWidget::HandleMenuAction);
	}
	for (int32 SlotIndex = 0; SlotIndex < UReEchoRunSubsystem::SaveSlotCount; ++SlotIndex)
	{
		if (UReEchoIndexedButton* SlotButton = Cast<UReEchoIndexedButton>(
		        WidgetTree->FindWidget(*FString::Printf(TEXT("SaveSlotButton%d"), SlotIndex))))
		{
			SlotButton->SetEntryIndex(SaveSlotActionBase + SlotIndex);
			SlotButton->OnIndexedClicked.AddUniqueDynamic(this, &UReEchoStartMenuWidget::HandleMenuAction);
		}
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

void UReEchoStartMenuWidget::InitializeMenu(const TArray<FReEchoSaveSlotSummary>& InSaveSlots)
{
	SaveSlots = InSaveSlots;
	bHasSavedRun = SaveSlots.ContainsByPredicate(
	    [](const FReEchoSaveSlotSummary& Candidate) { return Candidate.bOccupied; });
	RefreshMenu();
	RefreshSaveRollbackPanel();
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

void UReEchoStartMenuWidget::OpenSaveRollbackPanel()
{
	bShowingSaveRollback = true;
	if (UWidget* MainMenuPanel = WidgetTree->FindWidget(TEXT("StartMenuPanel")))
	{
		MainMenuPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (UWidget* SavePanel = WidgetTree->FindWidget(TEXT("SaveRollbackPanel")))
	{
		SavePanel->SetVisibility(ESlateVisibility::Visible);
	}
	RefreshSaveRollbackPanel();
	if (UReEchoIndexedButton* FirstSlot =
	        Cast<UReEchoIndexedButton>(WidgetTree->FindWidget(TEXT("SaveSlotButton0"))))
	{
		FirstSlot->SetKeyboardFocus();
	}
}

void UReEchoStartMenuWidget::CloseSaveRollbackPanel()
{
	bShowingSaveRollback = false;
	if (UWidget* SavePanel = WidgetTree->FindWidget(TEXT("SaveRollbackPanel")))
	{
		SavePanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (UWidget* MainMenuPanel = WidgetTree->FindWidget(TEXT("StartMenuPanel")))
	{
		MainMenuPanel->SetVisibility(ESlateVisibility::Visible);
	}
	if (ContinueButton)
	{
		ContinueButton->SetKeyboardFocus();
	}
}

void UReEchoStartMenuWidget::RefreshSaveRollbackPanel()
{
	SavePreviewTextures.Reset();
	for (int32 SlotIndex = 0; SlotIndex < UReEchoRunSubsystem::SaveSlotCount; ++SlotIndex)
	{
		const FReEchoSaveSlotSummary* Summary = SaveSlots.FindByPredicate(
		    [SlotIndex](const FReEchoSaveSlotSummary& Candidate) { return Candidate.SlotIndex == SlotIndex; });
		const bool bOccupied = Summary && Summary->bOccupied;
		if (UWidget* OccupiedArt =
		        WidgetTree->FindWidget(*FString::Printf(TEXT("SaveSlotOccupiedArt%d"), SlotIndex)))
		{
			OccupiedArt->SetVisibility(bOccupied ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
		if (UWidget* EmptyArt = WidgetTree->FindWidget(*FString::Printf(TEXT("SaveSlotEmptyArt%d"), SlotIndex)))
		{
			EmptyArt->SetVisibility(bOccupied ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
		}
		if (!bOccupied)
		{
			continue;
		}
		if (UTextBlock* Name =
		        Cast<UTextBlock>(WidgetTree->FindWidget(*FString::Printf(TEXT("SaveSlotName%d"), SlotIndex))))
		{
			Name->SetText(FText::FromString(FString::Printf(TEXT("存档 %d"), SlotIndex + 1)));
		}
		if (UTextBlock* Metadata =
		        Cast<UTextBlock>(WidgetTree->FindWidget(*FString::Printf(TEXT("SaveSlotMetadata%d"), SlotIndex))))
		{
			Metadata->SetText(FText::FromString(FString::Printf(
			    TEXT("到达关卡 %d，卡牌数量 %d"), Summary->EncounterNumber, Summary->CardCount)));
		}
		if (UTextBlock* Timestamp =
		        Cast<UTextBlock>(WidgetTree->FindWidget(*FString::Printf(TEXT("SaveSlotTimestamp%d"), SlotIndex))))
		{
			const FTimespan LocalOffset = FDateTime::Now() - FDateTime::UtcNow();
			const FDateTime LocalTime = Summary->SavedAtUtc.GetTicks() > 0 ? Summary->SavedAtUtc + LocalOffset
			                                                                : FDateTime();
			Timestamp->SetText(LocalTime.GetTicks() > 0
			                       ? FText::FromString(LocalTime.ToString(TEXT("%Y.%m.%d  %H:%M")))
			                       : FText::GetEmpty());
		}
		if (UImage* Preview =
		        Cast<UImage>(WidgetTree->FindWidget(*FString::Printf(TEXT("SaveSlotPreview%d"), SlotIndex))))
		{
			if (!Summary->PreviewScreenshotPath.IsEmpty())
			{
				if (UTexture2D* Texture = FImageUtils::ImportFileAsTexture2D(Summary->PreviewScreenshotPath))
				{
					SavePreviewTextures.Add(Texture);
					Preview->SetBrushFromTexture(Texture, false);
				}
			}
		}
	}
}

void UReEchoStartMenuWidget::HandleMenuAction(const int32 ActionIndex)
{
	if (ActionIndex == CloseSaveRollbackAction)
	{
		CloseSaveRollbackPanel();
		return;
	}
	if (ActionIndex >= SaveSlotActionBase &&
	    ActionIndex < SaveSlotActionBase + UReEchoRunSubsystem::SaveSlotCount)
	{
		OnSaveSlotRequested.Broadcast(ActionIndex - SaveSlotActionBase);
		return;
	}
	switch (static_cast<EStartMenuAction>(ActionIndex))
	{
		case EStartMenuAction::Continue:
			OpenSaveRollbackPanel();
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
