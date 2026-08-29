#include "UI/ReEchoRestartWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Core/ReEchoBalanceSettings.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "UI/Framework/ReEchoButtonVisualFeedback.h"
#include "UI/ReEchoMenuWidgetHelpers.h"

namespace
{
const FName DefaultSettlementCharacterId(TEXT("J_HEART"));
constexpr int32 DefeatCardSlotCount = 5;

const TCHAR* DefaultShopCardIconTexturePath =
    TEXT("/Game/ReEcho/Textures/UI/InteractionPlaceholder/InventoryShop/T_UI_Shop_CardIcon.T_UI_Shop_CardIcon");

bool IsSupportedSettlementCharacter(const FName CharacterId)
{
	return CharacterId == TEXT("J_HEART") || CharacterId == TEXT("J_SPADE") ||
	       CharacterId == TEXT("J_CLOVER") || CharacterId == TEXT("J_DIAMOND");
}

FString SettlementCharacterTexturePath(const FName CharacterId)
{
	const FString AssetName = FString::Printf(TEXT("T_UI_Loadout_Character_%s_Selected"), *CharacterId.ToString());
	return FString::Printf(TEXT("/Game/ReEcho/Textures/UI/LoadoutSelection/%s.%s"), *AssetName, *AssetName);
}

UTexture2D* LoadSettlementCharacterTexture(const FName CharacterId)
{
	const FName ResolvedId = IsSupportedSettlementCharacter(CharacterId) ? CharacterId : DefaultSettlementCharacterId;
	if (UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *SettlementCharacterTexturePath(ResolvedId)))
	{
		return Texture;
	}
	return ResolvedId == DefaultSettlementCharacterId
	           ? nullptr
	           : LoadObject<UTexture2D>(nullptr, *SettlementCharacterTexturePath(DefaultSettlementCharacterId));
}
} // namespace

TSharedRef<SWidget> UReEchoRestartWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildWidgetTree();
	}
	return Super::RebuildWidget();
}

void UReEchoRestartWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	BuildWidgetTree();
	EnsureAttackModeWidget();
	BindFormalResultButtonFeedback();

	if (ResumeButton)
	{
		ResumeButton->OnClicked.AddUniqueDynamic(this, &UReEchoRestartWidget::HandleResumeClicked);
	}
	if (RestartButton)
	{
		RestartButton->OnClicked.AddUniqueDynamic(this, &UReEchoRestartWidget::HandleRestartClicked);
	}
	if (VictoryContinueButton)
	{
		// Formal victory uses the same restart/continue contract as the legacy result button.
		VictoryContinueButton->OnClicked.AddUniqueDynamic(this, &UReEchoRestartWidget::HandleRestartClicked);
	}
	if (DefeatRestartButton)
	{
		DefeatRestartButton->OnClicked.AddUniqueDynamic(this, &UReEchoRestartWidget::HandleRestartClicked);
	}
	if (DefeatMainMenuButton)
	{
		DefeatMainMenuButton->OnClicked.AddUniqueDynamic(this, &UReEchoRestartWidget::HandleDefeatMainMenuClicked);
	}
	if (QuitButton)
	{
		QuitButton->OnClicked.AddUniqueDynamic(this, &UReEchoRestartWidget::HandleQuitClicked);
	}
	if (SettingsButton)
	{
		SettingsButton->OnClicked.AddUniqueDynamic(this, &UReEchoRestartWidget::HandleSettingsClicked);
	}
	if (PauseSettingsButton)
	{
		PauseSettingsButton->OnClicked.AddUniqueDynamic(this, &UReEchoRestartWidget::HandleSettingsClicked);
	}
	RefreshMenuMode();
	if (ScreenMode == EReEchoRestartScreenMode::Victory && VictoryContinueButton)
	{
		VictoryContinueButton->SetKeyboardFocus();
	}
	else if (ScreenMode == EReEchoRestartScreenMode::Death && DefeatRestartButton)
	{
		DefeatRestartButton->SetKeyboardFocus();
	}
	else if (ScreenMode != EReEchoRestartScreenMode::Pause && RestartButton)
	{
		RestartButton->SetKeyboardFocus();
	}
	else if (ResumeButton)
	{
		ResumeButton->SetKeyboardFocus();
	}
}

void UReEchoRestartWidget::BindFormalResultButtonFeedback()
{
	FormalResultButtonFeedback.Reset();
	auto BindVisual = [this](UButton* Button, UWidget* Visual)
	{
		if (!Button || !Visual)
		{
			return;
		}

		UReEchoButtonVisualFeedback* Feedback = NewObject<UReEchoButtonVisualFeedback>(this);
		Feedback->BindVisualOnly(Button, Visual);
		FormalResultButtonFeedback.Add(Feedback);
	};

	BindVisual(VictoryContinueButton, ArtVictoryContinueButtonFormal);
	BindVisual(VictoryContinueButton, VictoryContinueLabel);
	BindVisual(DefeatRestartButton, ArtDefeatRestartButtonFormal);
	BindVisual(DefeatRestartButton, DefeatRestartLabel);
	BindVisual(DefeatMainMenuButton, ArtDefeatMainMenuButtonFormal);
	BindVisual(DefeatMainMenuButton, DefeatMainMenuLabel);
}

namespace
{
	/** Ranks owned cards by tier and returns the icon paths for the authored settlement slots. */
	TArray<FString> BuildSettlementCardIconPaths(const TArray<FReEchoShopOffer>& OwnedCards, const int32 SlotCount)
	{
		TArray<FReEchoShopOffer> RankedCards = OwnedCards;
		RankedCards.StableSort(
		    [](const FReEchoShopOffer& Left, const FReEchoShopOffer& Right)
		    {
			    return Left.Tier > Right.Tier;
		    });
		const int32 VisibleCardCount = FMath::Min(SlotCount, RankedCards.Num());
		TArray<FString> Paths;
		Paths.Reserve(VisibleCardCount);
		for (int32 Index = 0; Index < VisibleCardCount; ++Index)
		{
			Paths.Add(RankedCards[Index].IconTexturePath);
		}
		return Paths;
	}
} // namespace

void UReEchoRestartWidget::SetDeathScreen(const bool bInDeathScreen,
                                          const int32 EncounterIndex,
                                          const int32 TimeShards,
                                          const int32 TraitCount,
                                          const FName InCharacterId)
{
	SetDeathScreen(bInDeathScreen, EncounterIndex, TimeShards, TraitCount, InCharacterId, {});
}

void UReEchoRestartWidget::SetDeathScreen(const bool bInDeathScreen,
                                          const int32 EncounterIndex,
                                          const int32 TimeShards,
                                          const int32 TraitCount,
                                          const FName InCharacterId,
                                          const TArray<FReEchoShopOffer>& OwnedCards)
{
	ScreenMode = bInDeathScreen ? EReEchoRestartScreenMode::Death : EReEchoRestartScreenMode::Pause;
	QuitPromptState = EReEchoQuitPromptState::None;
	DefeatEncounterIndex = FMath::Max(0, EncounterIndex);
	DefeatTimeShards = FMath::Max(0, TimeShards);
	DefeatTraitCount = FMath::Max(0, TraitCount);
	DefeatCardIconTexturePaths = BuildSettlementCardIconPaths(OwnedCards, DefeatCardSlotCount);
	VictoryCardIconTexturePaths.Reset();
	SettlementCharacterId = InCharacterId;
	CaptureRunStats();
	RefreshMenuMode();
	if (bInDeathScreen && DefeatRestartButton)
	{
		DefeatRestartButton->SetKeyboardFocus();
	}
}

void UReEchoRestartWidget::SetVictoryScreen(const int32 TimeShards,
                                            const int32 TraitCount,
                                            const FName InCharacterId)
{
	SetVictoryScreen(TimeShards, TraitCount, InCharacterId, {});
}

void UReEchoRestartWidget::SetVictoryScreen(const int32 TimeShards,
                                            const int32 TraitCount,
                                            const FName InCharacterId,
                                            const TArray<FReEchoShopOffer>& OwnedCards)
{
	ScreenMode = EReEchoRestartScreenMode::Victory;
	QuitPromptState = EReEchoQuitPromptState::None;
	VictoryTimeShards = FMath::Max(0, TimeShards);
	VictoryTraitCount = FMath::Max(0, TraitCount);
	VictoryCardIconTexturePaths = BuildSettlementCardIconPaths(OwnedCards, DefeatCardSlotCount);
	DefeatCardIconTexturePaths.Reset();
	SettlementCharacterId = InCharacterId;
	CaptureRunStats();
	RefreshMenuMode();
}

void UReEchoRestartWidget::CaptureRunStats()
{
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (const UReEchoRunStatsSubsystem* Stats = GameInstance->GetSubsystem<UReEchoRunStatsSubsystem>())
		{
			SettlementStats = Stats->GetRunStats();
			return;
		}
	}
	SettlementStats.Reset();
}

void UReEchoRestartWidget::RefreshRunStatsValues()
{
	const auto AssignDamage = [](const TObjectPtr<UTextBlock>& Widget, const float Value)
	{
		if (UTextBlock* Text = Widget.Get())
		{
			Text->SetText(FText::AsNumber(FMath::RoundToInt(Value)));
		}
	};
	const auto AssignCount = [](const TObjectPtr<UTextBlock>& Widget, const int32 Value)
	{
		if (UTextBlock* Text = Widget.Get())
		{
			Text->SetText(FText::AsNumber(Value));
		}
	};

	AssignDamage(VictoryEchoDamageValue, SettlementStats.EchoDamageTotal);
	AssignDamage(VictoryPlayerDamageValue, SettlementStats.PlayerDamageTotal);
	AssignCount(VictoryReactionCountValue, SettlementStats.ReactionTotal);
	AssignDamage(VictoryMaxHitValue, SettlementStats.MaxSingleHitDamage);
	AssignCount(VictoryKillCountValue, SettlementStats.KillTotal);

	AssignDamage(DefeatEchoDamageValue, SettlementStats.EchoDamageTotal);
	AssignDamage(DefeatPlayerDamageValue, SettlementStats.PlayerDamageTotal);
	AssignCount(DefeatReactionCountValue, SettlementStats.ReactionTotal);
	AssignDamage(DefeatMaxHitValue, SettlementStats.MaxSingleHitDamage);
	AssignCount(DefeatKillCountValue, SettlementStats.KillTotal);
}

void UReEchoRestartWidget::SetQuitConfirmation(const bool bInQuitConfirmation,
                                               const bool bInExitToMainMenu,
                                               const int32 InEncounterIndex)
{
	QuitPromptState = bInQuitConfirmation ? EReEchoQuitPromptState::Confirm : EReEchoQuitPromptState::None;
	bExitToMainMenu = bInQuitConfirmation && bInExitToMainMenu;
	PauseEncounterIndex = bInQuitConfirmation ? FMath::Max(0, InEncounterIndex) : 0;
	RefreshMenuMode();
	if (QuitPromptState == EReEchoQuitPromptState::Confirm && ResumeButton)
	{
		ResumeButton->SetKeyboardFocus();
	}
}

void UReEchoRestartWidget::ShowSaveFailure()
{
	QuitPromptState = EReEchoQuitPromptState::SaveFailed;
	RefreshMenuMode();
}

void UReEchoRestartWidget::SetAutomaticAttackMode(const bool bAutomatic)
{
	bAutomaticAttackMode = bAutomatic;
	EnsureAttackModeWidget();
	if (AttackModeWidget)
	{
		AttackModeWidget->SetAutomaticMode(bAutomaticAttackMode);
	}
}

void UReEchoRestartWidget::BuildWidgetTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MenuBackground"));
	Background->SetBrushColor(FLinearColor(0.015f, 0.02f, 0.035f, 0.92f));
	Background->SetPadding(FMargin(120.0f));
	Background->SetHorizontalAlignment(HAlign_Center);
	Background->SetVerticalAlignment(VAlign_Center);
	WidgetTree->RootWidget = Background;

	MenuContent = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MenuContent"));
	Background->SetContent(MenuContent);

	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MenuTitle"));
	TitleText->SetJustification(ETextJustify::Center);
	FSlateFontInfo TitleFont = TitleText->GetFont();
	TitleFont.Size = 44;
	TitleText->SetFont(TitleFont);
	UVerticalBoxSlot* TitleSlot = MenuContent->AddChildToVerticalBox(TitleText);
	TitleSlot->SetHorizontalAlignment(HAlign_Center);
	TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 18.0f));

	MessageText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MenuMessage"));
	MessageText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	MessageText->SetJustification(ETextJustify::Center);
	FSlateFontInfo MessageFont = MessageText->GetFont();
	MessageFont.Size = 21;
	MessageText->SetFont(MessageFont);
	UVerticalBoxSlot* MessageSlot = MenuContent->AddChildToVerticalBox(MessageText);
	MessageSlot->SetHorizontalAlignment(HAlign_Center);
	MessageSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 28.0f));

	ReEcho::UI::FMenuButtonStyle ButtonStyle{
	    FLinearColor(0.08f, 0.42f, 0.32f, 1.0f), FMargin(0.0f, 5.0f), FMargin(42.0f, 12.0f), 24};
	ResumeButton = ReEcho::UI::AddMenuButton(
	    *WidgetTree, *MenuContent, TEXT("ResumeButton"), FText::FromString(TEXT("返回游戏")), ButtonStyle);
	ButtonStyle.Color = FLinearColor(0.65f, 0.18f, 0.06f, 1.0f);
	RestartButton = ReEcho::UI::AddMenuButton(
	    *WidgetTree, *MenuContent, TEXT("RestartButton"), FText::FromString(TEXT("重新开始")), ButtonStyle);
	ButtonStyle.Color = FLinearColor(0.16f, 0.22f, 0.34f, 1.0f);
	SettingsButton = ReEcho::UI::AddMenuButton(
	    *WidgetTree, *MenuContent, TEXT("SettingsButton"), FText::FromString(TEXT("游戏设置")), ButtonStyle);
	ButtonStyle.Color = FLinearColor(0.55f, 0.05f, 0.08f, 1.0f);
	QuitButton = ReEcho::UI::AddMenuButton(
	    *WidgetTree, *MenuContent, TEXT("QuitButton"), FText::FromString(TEXT("退出游戏")), ButtonStyle);
	QuitButtonText = Cast<UTextBlock>(QuitButton->GetContent());
	RefreshMenuMode();
}

void UReEchoRestartWidget::RefreshMenuMode()
{
	const bool bVictoryScreen = ScreenMode == EReEchoRestartScreenMode::Victory;
	const bool bDeathScreen = ScreenMode == EReEchoRestartScreenMode::Death;
	const bool bQuitConfirmation = QuitPromptState == EReEchoQuitPromptState::Confirm;
	const bool bSaveFailed = QuitPromptState == EReEchoQuitPromptState::SaveFailed;
	const bool bFormalResult = bVictoryScreen || bDeathScreen;
	RefreshSettlementCharacterImages();
	RefreshDefeatCardSlots();
	RefreshRunStatsValues();

	if (VictoryCanvas)
	{
		VictoryCanvas->SetVisibility(bVictoryScreen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (VictoryEncounterValue)
	{
		VictoryEncounterValue->SetText(FText::AsNumber(GetDefault<UReEchoBalanceSettings>()->GetTotalEncounterCount()));
	}
	if (VictoryTimeShardsValue)
	{
		VictoryTimeShardsValue->SetText(FText::AsNumber(VictoryTimeShards));
	}
	if (VictoryTraitCountValue)
	{
		VictoryTraitCountValue->SetText(FText::AsNumber(VictoryTraitCount));
	}
	if (VictoryContinueButton)
	{
		VictoryContinueButton->SetVisibility(bVictoryScreen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (DefeatCanvas)
	{
		DefeatCanvas->SetVisibility(bDeathScreen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (DefeatEncounterValue)
	{
		DefeatEncounterValue->SetText(FText::AsNumber(DefeatEncounterIndex));
	}
	if (DefeatTimeShardsValue)
	{
		DefeatTimeShardsValue->SetText(FText::AsNumber(DefeatTimeShards));
	}
	if (DefeatTraitCountValue)
	{
		DefeatTraitCountValue->SetText(FText::AsNumber(DefeatTraitCount));
	}
	if (DefeatRestartButton)
	{
		DefeatRestartButton->SetVisibility(bDeathScreen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (DefeatMainMenuButton)
	{
		DefeatMainMenuButton->SetVisibility(bDeathScreen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (RootPanel)
	{
		RootPanel->SetVisibility(bFormalResult ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}

	if (TitleText)
	{
		const FString Title = bSaveFailed       ? TEXT("保存失败")
		                      : bQuitConfirmation ? (bExitToMainMenu ? TEXT("退出到主菜单?") : TEXT("退出游戏?"))
		                                          : TEXT("游戏暂停");
		TitleText->SetText(FText::FromString(Title));
		TitleText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		TitleText->SetVisibility(bFormalResult ? ESlateVisibility::Collapsed
		                                       : ESlateVisibility::HitTestInvisible);
	}
	if (MessageText)
	{
		if (bSaveFailed)
		{
			MessageText->SetText(FText::FromString(TEXT("未能保存退出前状态，游戏不会退出，请重试")));
		}
		else if (bQuitConfirmation)
		{
			MessageText->SetText(FText::Format(NSLOCTEXT("ReEcho", "PauseSavePoint", "存档点：第 {0} 关"),
			                                   FText::AsNumber(PauseEncounterIndex)));
		}
		else
		{
			MessageText->SetText(FText::FromString(TEXT("游戏已暂停 · 按 P 可继续")));
		}
		MessageText->SetVisibility(bFormalResult ? ESlateVisibility::Collapsed
		                           : !bQuitConfirmation && !bSaveFailed
		                               ? ESlateVisibility::Collapsed
		                               : ESlateVisibility::HitTestInvisible);
	}
	const bool bPauseMenu = ScreenMode == EReEchoRestartScreenMode::Pause;
	const bool bPausePrompt = bPauseMenu && (bQuitConfirmation || bSaveFailed);
	const bool bPauseRootVisible = bPauseMenu;
	if (ArtPauseDimmer)
	{
		ArtPauseDimmer->SetVisibility(bPauseRootVisible ? ESlateVisibility::HitTestInvisible
		                                                : ESlateVisibility::Hidden);
	}
	auto SetPauseArtVisibility = [](UImage* Image, const bool bVisible)
	{
		if (Image)
		{
			Image->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
	};
	SetPauseArtVisibility(ArtPausePrimaryButton, bPauseMenu);
	SetPauseArtVisibility(ArtPauseSecondaryButton, bPauseMenu);
	SetPauseArtVisibility(ArtPauseTertiaryButton, bPauseMenu);
	SetPauseArtVisibility(ArtPauseSettings, bPauseMenu && !bPausePrompt);
	if (ArtRestartDialogPanel)
	{
		// The pause exit prompts are composed directly over the dimmed game scene.
		// The restart-dialog plate belongs to the save-failure fallback, not to either exit confirmation.
		ArtRestartDialogPanel->SetVisibility(bSaveFailed ? ESlateVisibility::HitTestInvisible
		                                                 : ESlateVisibility::Hidden);
	}
	if (ResumeButton)
	{
		ResumeButton->SetVisibility(bDeathScreen || bVictoryScreen ? ESlateVisibility::Collapsed
		                                                           : ESlateVisibility::Visible);
	}
	if (RestartButton)
	{
		RestartButton->SetVisibility(bFormalResult ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
	if (QuitButton)
	{
		QuitButton->SetVisibility(bFormalResult ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
	if (SettingsButton)
	{
		const bool bShowSettingsEntry =
		    !PauseSettingsButton && !bDeathScreen && !bVictoryScreen && !bQuitConfirmation && !bSaveFailed;
		SettingsButton->SetVisibility(bShowSettingsEntry ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (PauseSettingsButton)
	{
		PauseSettingsButton->SetVisibility(bPauseMenu && !bPausePrompt ? ESlateVisibility::Visible
		                                                               : ESlateVisibility::Collapsed);
	}
	const FLinearColor ButtonBackground = bPauseMenu ? FLinearColor::Transparent : FLinearColor::White;
	for (UButton* Button : {ResumeButton.Get(), RestartButton.Get(), QuitButton.Get()})
	{
		if (Button)
		{
			Button->SetBackgroundColor(ButtonBackground);
		}
	}
	if (ResumeButtonLabel)
	{
		ResumeButtonLabel->SetText(FText::FromString(bPausePrompt ? TEXT("保存并退出") : TEXT("继续游戏")));
		ResumeButtonLabel->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (RestartButtonLabel)
	{
		const FString RestartLabel = bPausePrompt ? TEXT("不保存并退出")
		                             : bPauseMenu ? TEXT("退出至主菜单")
		                                          : TEXT("重新开始");
		RestartButtonLabel->SetText(FText::FromString(RestartLabel));
		RestartButtonLabel->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (QuitButtonText)
	{
		const FString QuitLabel = bPausePrompt ? TEXT("返回") : TEXT("退出游戏");
		QuitButtonText->SetText(FText::FromString(QuitLabel));
		QuitButtonText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (AttackModeWidget)
	{
		const bool bShowAttackMode = ScreenMode == EReEchoRestartScreenMode::Pause &&
		                             QuitPromptState == EReEchoQuitPromptState::None && !ArtPausePrimaryButton;
		AttackModeWidget->SetVisibility(bShowAttackMode ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		AttackModeWidget->SetAutomaticMode(bAutomaticAttackMode);
	}
}

void UReEchoRestartWidget::RefreshSettlementCharacterImages()
{
	UTexture2D* CharacterTexture = LoadSettlementCharacterTexture(SettlementCharacterId);
	if (!CharacterTexture)
	{
		// Preserve the WBP-authored red-hood Brush if even the packaged fallback is unavailable.
		return;
	}
	for (UImage* CharacterImage : {ArtVictoryCharacterFormal.Get(), ArtDefeatCharacterFormal.Get()})
	{
		if (CharacterImage)
		{
			CharacterImage->SetBrushFromTexture(CharacterTexture, false);
		}
	}
}

void UReEchoRestartWidget::RefreshDefeatCardSlots()
{
	RefreshCardIconSlots(DefeatCardIconTexturePaths, TEXT("DesignerDefeatCardIcon"));
	RefreshCardIconSlots(VictoryCardIconTexturePaths, TEXT("DesignerVictoryCardIcon"));
}

void UReEchoRestartWidget::RefreshCardIconSlots(const TArray<FString>& TexturePaths, const TCHAR* SlotNamePrefix)
{
	UTexture2D* DefaultCardTexture = nullptr;
	for (int32 Index = 0; Index < DefeatCardSlotCount; ++Index)
	{
		UImage* CardIcon = Cast<UImage>(GetWidgetFromName(*FString::Printf(TEXT("%s%d"), SlotNamePrefix, Index)));
		if (!CardIcon)
		{
			continue;
		}
		if (!TexturePaths.IsValidIndex(Index))
		{
			CardIcon->SetVisibility(ESlateVisibility::Hidden);
			continue;
		}

		UTexture2D* CardTexture = nullptr;
		const FString& TexturePath = TexturePaths[Index];
		if (!TexturePath.IsEmpty())
		{
			CardTexture = LoadObject<UTexture2D>(nullptr, *TexturePath);
		}
		if (!CardTexture)
		{
			if (!DefaultCardTexture)
			{
				DefaultCardTexture = LoadObject<UTexture2D>(nullptr, DefaultShopCardIconTexturePath);
			}
			CardTexture = DefaultCardTexture;
		}

		if (CardTexture)
		{
			CardIcon->SetBrushFromTexture(CardTexture, false);
			CardIcon->SetColorAndOpacity(FLinearColor::White);
			CardIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			CardIcon->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void UReEchoRestartWidget::EnsureAttackModeWidget()
{
	if (AttackModeWidget || !WidgetTree)
	{
		return;
	}
	if (!MenuContent)
	{
		MenuContent = Cast<UVerticalBox>(WidgetTree->FindWidget(TEXT("MenuContent")));
	}
	if (!MenuContent && SettingsButton)
	{
		MenuContent = Cast<UVerticalBox>(SettingsButton->GetParent());
	}
	if (!MenuContent)
	{
		return;
	}
	AttackModeWidget = WidgetTree->ConstructWidget<UReEchoAttackModeWidget>(UReEchoAttackModeWidget::StaticClass(),
	                                                                        TEXT("AttackModePanel"));
	MenuContent->AddChildToVerticalBox(AttackModeWidget)->SetPadding(FMargin(4.0f, 8.0f));
	AttackModeWidget->OnAutomaticRequested.AddDynamic(this, &UReEchoRestartWidget::HandleAutomaticAttackClicked);
	AttackModeWidget->OnManualRequested.AddDynamic(this, &UReEchoRestartWidget::HandleManualAttackClicked);
	AttackModeWidget->SetAutomaticMode(bAutomaticAttackMode);
}

void UReEchoRestartWidget::HandleResumeClicked()
{
	if (QuitPromptState == EReEchoQuitPromptState::None)
	{
		OnResumeRequested.Broadcast();
		return;
	}
	OnQuitRequested.Broadcast();
}

void UReEchoRestartWidget::HandleRestartClicked()
{
	if (QuitPromptState != EReEchoQuitPromptState::None)
	{
		OnExitWithoutSavingRequested.Broadcast();
		return;
	}
	if (ScreenMode == EReEchoRestartScreenMode::Pause)
	{
		OnExitToMainMenuRequested.Broadcast();
		return;
	}
	OnRestartRequested.Broadcast();
}

void UReEchoRestartWidget::HandleQuitClicked()
{
	if (QuitPromptState != EReEchoQuitPromptState::None)
	{
		OnCancelExitRequested.Broadcast();
		return;
	}
	OnQuitRequested.Broadcast();
}

void UReEchoRestartWidget::HandleDefeatMainMenuClicked()
{
	OnExitToMainMenuRequested.Broadcast();
}

void UReEchoRestartWidget::HandleSettingsClicked()
{
	OnSettingsRequested.Broadcast();
}

void UReEchoRestartWidget::HandleAutomaticAttackClicked()
{
	OnAutomaticAttackRequested.Broadcast();
}

void UReEchoRestartWidget::HandleManualAttackClicked()
{
	OnManualAttackRequested.Broadcast();
}
