#include "UI/ReEchoSettingsWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/ReEchoIndexedButton.h"
#include "UI/ReEchoMenuWidgetHelpers.h"
#include "ReEchoAudioService.h"
#include "Components/Slider.h"
#include "Components/CheckBox.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

TSharedRef<SWidget> UReEchoSettingsWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		BuildWidgetTree();
	}
	return Super::RebuildWidget();
}

void UReEchoSettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	BuildWidgetTree();
	BuildAudioPanel();
	BindAudioControls();
	bAudioSettingsApplied = false;
	CategoryButtons = {GraphicsSettingsButton, AudioSettingsButton, ControlsSettingsButton};
	CategoryButtons.Remove(nullptr);
	for (int32 CategoryIndex = 0; CategoryIndex < CategoryButtons.Num(); ++CategoryIndex)
	{
		CategoryButtons[CategoryIndex]->SetEntryIndex(CategoryIndex);
		CategoryButtons[CategoryIndex]->OnIndexedClicked.AddUniqueDynamic(
		    this, &UReEchoSettingsWidget::HandleCategoryClicked);
	}
	if (RestoreDefaultsButton)
	{
		RestoreDefaultsButton->OnClicked.AddUniqueDynamic(this, &UReEchoSettingsWidget::HandleRestoreDefaultsClicked);
	}
	if (ApplyAndReturnButton)
	{
		ApplyAndReturnButton->OnClicked.AddUniqueDynamic(this, &UReEchoSettingsWidget::HandleApplyAndReturnClicked);
	}
	RefreshCategory();
	if (!CategoryButtons.IsEmpty())
	{
		CategoryButtons[0]->SetKeyboardFocus();
	}
}

void UReEchoSettingsWidget::NativeDestruct()
{
	if (!bAudioSettingsApplied)
	{
		if (UReEchoAudioService* AudioService = GetAudioService())
		{
			AudioService->RevertUserSettings();
		}
	}
	Super::NativeDestruct();
}

void UReEchoSettingsWidget::BuildWidgetTree()
{
	if (DetailText || !WidgetTree)
	{
		return;
	}

	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SettingsBackground"));
	Background->SetBrushColor(FLinearColor(0.015f, 0.02f, 0.035f, 0.96f));
	Background->SetPadding(FMargin(120.0f));
	Background->SetHorizontalAlignment(HAlign_Center);
	Background->SetVerticalAlignment(VAlign_Center);
	WidgetTree->RootWidget = Background;

	UVerticalBox* Content =
	    WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SettingsContent"));
	Background->SetContent(Content);

	UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SettingsTitle"));
	TitleText->SetText(FText::FromString(TEXT("游戏设置")));
	TitleText->SetJustification(ETextJustify::Center);
	TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.4f, 0.85f, 1.0f)));
	FSlateFontInfo TitleFont = TitleText->GetFont();
	TitleFont.Size = 44;
	TitleText->SetFont(TitleFont);
	UVerticalBoxSlot* TitleSlot = Content->AddChildToVerticalBox(TitleText);
	TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 18.0f));

	UHorizontalBox* SettingsColumns =
	    WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SettingsColumns"));
	UVerticalBoxSlot* ColumnsSlot = Content->AddChildToVerticalBox(SettingsColumns);
	ColumnsSlot->SetHorizontalAlignment(HAlign_Fill);
	ColumnsSlot->SetVerticalAlignment(VAlign_Fill);

	UBorder* NavigationBorder =
	    WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SettingsNavigationBorder"));
	NavigationBorder->SetBrushColor(FLinearColor(0.025f, 0.04f, 0.065f, 0.98f));
	NavigationBorder->SetPadding(FMargin(18.0f));
	UHorizontalBoxSlot* NavigationSlot = SettingsColumns->AddChildToHorizontalBox(NavigationBorder);
	NavigationSlot->SetPadding(FMargin(0.0f, 0.0f, 18.0f, 0.0f));
	NavigationSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

	UVerticalBox* NavigationPanel =
	    WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SettingsNavigation"));
	NavigationBorder->SetContent(NavigationPanel);
	CategoryButtons.Reset();
	const ReEcho::UI::FMenuButtonStyle NavigationButtonStyle{
	    FLinearColor(0.12f, 0.32f, 0.48f, 1.0f), FMargin(0.0f, 5.0f), FMargin(42.0f, 12.0f), 24};
	static const TPair<FName, FString> CategoryDefinitions[] = {{TEXT("GraphicsSettingsButton"), TEXT("画面")},
	                                                            {TEXT("AudioSettingsButton"), TEXT("声音")},
	                                                            {TEXT("ControlsSettingsButton"), TEXT("操作（键位）")}};
	for (int32 CategoryIndex = 0; CategoryIndex < UE_ARRAY_COUNT(CategoryDefinitions); ++CategoryIndex)
	{
		const TPair<FName, FString>& Definition = CategoryDefinitions[CategoryIndex];
		UReEchoIndexedButton* CategoryButton = ReEcho::UI::AddIndexedMenuButton(*WidgetTree,
		                                                                        *NavigationPanel,
		                                                                        Definition.Key,
		                                                                        FText::FromString(Definition.Value),
		                                                                        CategoryIndex,
		                                                                        NavigationButtonStyle);
		CategoryButtons.Add(CategoryButton);
	}
	GraphicsSettingsButton = CategoryButtons[0];
	AudioSettingsButton = CategoryButtons[1];
	ControlsSettingsButton = CategoryButtons[2];

	UBorder* DetailBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SettingsDetailBorder"));
	DetailBorder->SetBrushColor(FLinearColor(0.035f, 0.05f, 0.075f, 0.98f));
	DetailBorder->SetPadding(FMargin(32.0f));
	UHorizontalBoxSlot* DetailColumnSlot = SettingsColumns->AddChildToHorizontalBox(DetailBorder);
	DetailColumnSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	UVerticalBox* DetailContentBox =
	    WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SettingsDetailContent"));
	DetailBorder->SetContent(DetailContentBox);
	// Plan34: keep a handle to the detail column so the audio panel can be added into it.
	DetailContent = DetailContentBox;

	CategoryTitleText =
	    WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SettingsCategoryTitle"));
	CategoryTitleText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	FSlateFontInfo CategoryTitleFont = CategoryTitleText->GetFont();
	CategoryTitleFont.Size = 30;
	CategoryTitleText->SetFont(CategoryTitleFont);
	UVerticalBoxSlot* CategoryTitleSlot = DetailContentBox->AddChildToVerticalBox(CategoryTitleText);
	CategoryTitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 20.0f));

	DetailText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SettingsDetail"));
	DetailText->SetJustification(ETextJustify::Left);
	DetailText->SetColorAndOpacity(FSlateColor(FLinearColor(0.82f, 0.86f, 0.92f)));
	FSlateFontInfo DetailFont = DetailText->GetFont();
	DetailFont.Size = 20;
	DetailText->SetFont(DetailFont);
	UVerticalBoxSlot* DetailSlot = DetailContentBox->AddChildToVerticalBox(DetailText);
	DetailSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	DetailSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 24.0f));

	ReEcho::UI::FMenuButtonStyle ActionButtonStyle{
	    FLinearColor(0.24f, 0.24f, 0.28f, 1.0f), FMargin(0.0f, 5.0f), FMargin(42.0f, 12.0f), 24};
	RestoreDefaultsButton = ReEcho::UI::AddMenuButton(*WidgetTree,
	                                                  *DetailContentBox,
	                                                  TEXT("RestoreDefaultsButton"),
	                                                  FText::FromString(TEXT("恢复默认")),
	                                                  ActionButtonStyle);
	ActionButtonStyle.Color = FLinearColor(0.08f, 0.42f, 0.32f, 1.0f);
	ApplyAndReturnButton = ReEcho::UI::AddMenuButton(*WidgetTree,
	                                                 *DetailContentBox,
	                                                 TEXT("ApplyAndReturnButton"),
	                                                 FText::FromString(TEXT("应用并返回")),
	                                                 ActionButtonStyle);
}

void UReEchoSettingsWidget::RefreshCategory()
{
	if (!CategoryTitleText)
	{
		return;
	}

	const FString CategoryTitle = SelectedCategory == 1   ? TEXT("声音设置")
	                              : SelectedCategory == 2 ? TEXT("操作（键位）")
	                                                      : TEXT("画面设置");
	CategoryTitleText->SetText(FText::FromString(CategoryTitle));

	if (SelectedCategory == 1)
	{
		// Plan34: audio owns live, persistent controls instead of the placeholder text.
		if (DetailText)
		{
			DetailText->SetVisibility(ESlateVisibility::Collapsed);
		}
		BuildAudioPanel();
		if (AudioPanel)
		{
			AudioPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		RefreshAudioControls();
	}
	else
	{
		const FString Detail = SelectedCategory == 2 ? TEXT("按键重绑定等具体选项暂未接入")
		                                             : TEXT("分辨率、显示模式、画质等具体选项暂未接入");
		if (DetailText)
		{
			DetailText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			DetailText->SetText(FText::FromString(Detail));
		}
		if (AudioPanel)
		{
			AudioPanel->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	const FLinearColor SettingsSelectedColor(0.08f, 0.42f, 0.62f, 1.0f);
	const FLinearColor SettingsNormalColor(0.12f, 0.22f, 0.32f, 1.0f);
	for (int32 CategoryIndex = 0; CategoryIndex < CategoryButtons.Num(); ++CategoryIndex)
	{
		CategoryButtons[CategoryIndex]->SetBackgroundColor(SelectedCategory == CategoryIndex ? SettingsSelectedColor
		                                                                                     : SettingsNormalColor);
	}
}

void UReEchoSettingsWidget::HandleCategoryClicked(const int32 CategoryIndex)
{
	if (!CategoryButtons.IsValidIndex(CategoryIndex))
	{
		return;
	}

	SelectedCategory = CategoryIndex;
	RefreshCategory();
}

void UReEchoSettingsWidget::HandleRestoreDefaultsClicked()
{
	if (UReEchoAudioService* AudioService = GetAudioService())
	{
		AudioService->PreviewDefaultSettings();
	}
	RefreshAudioControls();
}

void UReEchoSettingsWidget::HandleApplyAndReturnClicked()
{
	if (UReEchoAudioService* AudioService = GetAudioService())
	{
		bAudioSettingsApplied = AudioService->CommitUserSettings();
	}
	OnClosed.Broadcast();
}

UReEchoAudioService* UReEchoSettingsWidget::GetAudioService() const
{
	UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(this);
	return GameInstance ? GameInstance->GetSubsystem<UReEchoAudioService>() : nullptr;
}

void UReEchoSettingsWidget::BuildAudioPanel()
{
	if (AudioPanel || !DetailContent || !WidgetTree)
	{
		return;
	}

	AudioPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("AudioPanel"));
	DetailContent->AddChildToVerticalBox(AudioPanel);

	// Named exactly per Plan34 so a future WBP_ReEchoSettings can BindWidgetOptional them.
	auto AddVolumeRow = [&](const FString& Label, TObjectPtr<USlider>& OutSlider, FName SliderName) -> void
	{
		UVerticalBox* Row = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), SliderName);
		UTextBlock* LabelText = WidgetTree->ConstructWidget<UTextBlock>(
		    UTextBlock::StaticClass(), *FString::Printf(TEXT("%sLabel"), *SliderName.ToString()));
		LabelText->SetText(FText::FromString(Label));
		LabelText->SetColorAndOpacity(FSlateColor(FLinearColor(0.82f, 0.86f, 0.92f)));
		FSlateFontInfo LabelFont = LabelText->GetFont();
		LabelFont.Size = 20;
		LabelText->SetFont(LabelFont);
		Row->AddChildToVerticalBox(LabelText);

		USlider* Slider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), SliderName);
		Slider->SetValue(0.8f);
		Row->AddChildToVerticalBox(Slider);
		OutSlider = Slider;

		UVerticalBoxSlot* RowSlot = AudioPanel->AddChildToVerticalBox(Row);
		RowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));
	};

	auto AddMuteRow = [&](const FString& Label, TObjectPtr<UCheckBox>& OutCheck, FName CheckName) -> void
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), CheckName);
		UCheckBox* Check = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass(), CheckName);
		Row->AddChildToHorizontalBox(Check);
		UTextBlock* LabelText = WidgetTree->ConstructWidget<UTextBlock>(
		    UTextBlock::StaticClass(), *FString::Printf(TEXT("%sLabel"), *CheckName.ToString()));
		LabelText->SetText(FText::FromString(Label));
		LabelText->SetColorAndOpacity(FSlateColor(FLinearColor(0.82f, 0.86f, 0.92f)));
		FSlateFontInfo LabelFont = LabelText->GetFont();
		LabelFont.Size = 20;
		LabelText->SetFont(LabelFont);
		Row->AddChildToHorizontalBox(LabelText);
		OutCheck = Check;

		UVerticalBoxSlot* RowSlot = AudioPanel->AddChildToVerticalBox(Row);
		RowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	};

	AddVolumeRow(TEXT("主音量 (Master)"), MasterVolumeSlider, TEXT("MasterVolumeSlider"));
	MasterVolumeSlider->OnValueChanged.AddUniqueDynamic(this, &UReEchoSettingsWidget::HandleMasterVolumeChanged);
	AddVolumeRow(TEXT("音乐 (Music)"), MusicVolumeSlider, TEXT("MusicVolumeSlider"));
	MusicVolumeSlider->OnValueChanged.AddUniqueDynamic(this, &UReEchoSettingsWidget::HandleMusicVolumeChanged);
	AddVolumeRow(TEXT("环境 (Ambience)"), AmbienceVolumeSlider, TEXT("AmbienceVolumeSlider"));
	AmbienceVolumeSlider->OnValueChanged.AddUniqueDynamic(this, &UReEchoSettingsWidget::HandleAmbienceVolumeChanged);
	AddVolumeRow(TEXT("战斗音效 (Combat SFX)"), CombatSfxVolumeSlider, TEXT("CombatSfxVolumeSlider"));
	CombatSfxVolumeSlider->OnValueChanged.AddUniqueDynamic(this, &UReEchoSettingsWidget::HandleCombatSfxVolumeChanged);
	AddVolumeRow(TEXT("界面音效 (UI SFX)"), UiSfxVolumeSlider, TEXT("UiSfxVolumeSlider"));
	UiSfxVolumeSlider->OnValueChanged.AddUniqueDynamic(this, &UReEchoSettingsWidget::HandleUiSfxVolumeChanged);

	AddMuteRow(TEXT("静音 主音量"), MasterMuteCheckBox, TEXT("MasterMuteCheckBox"));
	MasterMuteCheckBox->OnCheckStateChanged.AddUniqueDynamic(this, &UReEchoSettingsWidget::HandleMasterMuteChanged);
	AddMuteRow(TEXT("静音 音乐"), MusicMuteCheckBox, TEXT("MusicMuteCheckBox"));
	MusicMuteCheckBox->OnCheckStateChanged.AddUniqueDynamic(this, &UReEchoSettingsWidget::HandleMusicMuteChanged);
	AddMuteRow(TEXT("静音 环境"), AmbienceMuteCheckBox, TEXT("AmbienceMuteCheckBox"));
	AmbienceMuteCheckBox->OnCheckStateChanged.AddUniqueDynamic(this, &UReEchoSettingsWidget::HandleAmbienceMuteChanged);
	AddMuteRow(TEXT("静音 战斗音效"), CombatSfxMuteCheckBox, TEXT("CombatSfxMuteCheckBox"));
	CombatSfxMuteCheckBox->OnCheckStateChanged.AddUniqueDynamic(this, &UReEchoSettingsWidget::HandleCombatSfxMuteChanged);
	AddMuteRow(TEXT("静音 界面音效"), UiSfxMuteCheckBox, TEXT("UiSfxMuteCheckBox"));
	UiSfxMuteCheckBox->OnCheckStateChanged.AddUniqueDynamic(this, &UReEchoSettingsWidget::HandleUiSfxMuteChanged);
	AddMuteRow(TEXT("诊断音 (440Hz，仅用于验证音量/静音)"), DiagnosticToneCheckBox, TEXT("DiagnosticToneCheckBox"));
	DiagnosticToneCheckBox->OnCheckStateChanged.AddUniqueDynamic(this, &UReEchoSettingsWidget::HandleDiagnosticToneChanged);
}

void UReEchoSettingsWidget::BindAudioControls()
{
	if (MasterVolumeSlider) MasterVolumeSlider->OnValueChanged.AddUniqueDynamic(this, &UReEchoSettingsWidget::HandleMasterVolumeChanged);
	if (MusicVolumeSlider) MusicVolumeSlider->OnValueChanged.AddUniqueDynamic(this, &UReEchoSettingsWidget::HandleMusicVolumeChanged);
	if (AmbienceVolumeSlider) AmbienceVolumeSlider->OnValueChanged.AddUniqueDynamic(this, &UReEchoSettingsWidget::HandleAmbienceVolumeChanged);
	if (CombatSfxVolumeSlider) CombatSfxVolumeSlider->OnValueChanged.AddUniqueDynamic(this, &UReEchoSettingsWidget::HandleCombatSfxVolumeChanged);
	if (UiSfxVolumeSlider) UiSfxVolumeSlider->OnValueChanged.AddUniqueDynamic(this, &UReEchoSettingsWidget::HandleUiSfxVolumeChanged);
	if (MasterMuteCheckBox) MasterMuteCheckBox->OnCheckStateChanged.AddUniqueDynamic(this, &UReEchoSettingsWidget::HandleMasterMuteChanged);
	if (MusicMuteCheckBox) MusicMuteCheckBox->OnCheckStateChanged.AddUniqueDynamic(this, &UReEchoSettingsWidget::HandleMusicMuteChanged);
	if (AmbienceMuteCheckBox) AmbienceMuteCheckBox->OnCheckStateChanged.AddUniqueDynamic(this, &UReEchoSettingsWidget::HandleAmbienceMuteChanged);
	if (CombatSfxMuteCheckBox) CombatSfxMuteCheckBox->OnCheckStateChanged.AddUniqueDynamic(this, &UReEchoSettingsWidget::HandleCombatSfxMuteChanged);
	if (UiSfxMuteCheckBox) UiSfxMuteCheckBox->OnCheckStateChanged.AddUniqueDynamic(this, &UReEchoSettingsWidget::HandleUiSfxMuteChanged);
	if (DiagnosticToneCheckBox) DiagnosticToneCheckBox->OnCheckStateChanged.AddUniqueDynamic(this, &UReEchoSettingsWidget::HandleDiagnosticToneChanged);
}

void UReEchoSettingsWidget::RefreshAudioControls()
{
	UReEchoAudioService* AudioService = GetAudioService();
	if (!AudioService)
	{
		return;
	}
	TGuardValue<bool> RefreshGuard(bRefreshingAudioControls, true);
	if (MasterVolumeSlider)
	{
		MasterVolumeSlider->SetValue(FMath::Clamp(AudioService->GetMasterVolume(), 0.0f, 1.0f));
	}
	if (MusicVolumeSlider)
	{
		MusicVolumeSlider->SetValue(FMath::Clamp(AudioService->GetBusVolume(EReEchoAudioBus::Music), 0.0f, 1.0f));
	}
	if (AmbienceVolumeSlider)
	{
		AmbienceVolumeSlider->SetValue(FMath::Clamp(AudioService->GetBusVolume(EReEchoAudioBus::Ambience), 0.0f, 1.0f));
	}
	if (CombatSfxVolumeSlider)
	{
		CombatSfxVolumeSlider->SetValue(FMath::Clamp(AudioService->GetBusVolume(EReEchoAudioBus::CombatSfx), 0.0f, 1.0f));
	}
	if (UiSfxVolumeSlider)
	{
		UiSfxVolumeSlider->SetValue(FMath::Clamp(AudioService->GetBusVolume(EReEchoAudioBus::UiSfx), 0.0f, 1.0f));
	}
	if (MasterMuteCheckBox)
	{
		MasterMuteCheckBox->SetIsChecked(AudioService->IsBusMuted(EReEchoAudioBus::Master));
	}
	if (MusicMuteCheckBox)
	{
		MusicMuteCheckBox->SetIsChecked(AudioService->IsBusMuted(EReEchoAudioBus::Music));
	}
	if (AmbienceMuteCheckBox)
	{
		AmbienceMuteCheckBox->SetIsChecked(AudioService->IsBusMuted(EReEchoAudioBus::Ambience));
	}
	if (CombatSfxMuteCheckBox)
	{
		CombatSfxMuteCheckBox->SetIsChecked(AudioService->IsBusMuted(EReEchoAudioBus::CombatSfx));
	}
	if (UiSfxMuteCheckBox)
	{
		UiSfxMuteCheckBox->SetIsChecked(AudioService->IsBusMuted(EReEchoAudioBus::UiSfx));
	}
	if (DiagnosticToneCheckBox)
	{
		DiagnosticToneCheckBox->SetIsChecked(false);
	}
}

void UReEchoSettingsWidget::HandleMasterVolumeChanged(const float Value)
{
	if (UReEchoAudioService* AudioService = GetAudioService())
	{
		AudioService->SetMasterVolume(Value);
	}
}

void UReEchoSettingsWidget::HandleMusicVolumeChanged(const float Value)
{
	if (UReEchoAudioService* AudioService = GetAudioService())
	{
		AudioService->SetBusVolume(EReEchoAudioBus::Music, Value);
	}
}

void UReEchoSettingsWidget::HandleAmbienceVolumeChanged(const float Value)
{
	if (UReEchoAudioService* AudioService = GetAudioService())
	{
		AudioService->SetBusVolume(EReEchoAudioBus::Ambience, Value);
	}
}

void UReEchoSettingsWidget::HandleCombatSfxVolumeChanged(const float Value)
{
	if (UReEchoAudioService* AudioService = GetAudioService())
	{
		AudioService->SetBusVolume(EReEchoAudioBus::CombatSfx, Value);
	}
}

void UReEchoSettingsWidget::HandleUiSfxVolumeChanged(const float Value)
{
	if (UReEchoAudioService* AudioService = GetAudioService())
	{
		AudioService->SetBusVolume(EReEchoAudioBus::UiSfx, Value);
	}
}

void UReEchoSettingsWidget::HandleMasterMuteChanged(const bool bChecked)
{
	if (UReEchoAudioService* AudioService = GetAudioService())
	{
		AudioService->SetBusMuted(EReEchoAudioBus::Master, bChecked);
	}
}

void UReEchoSettingsWidget::HandleMusicMuteChanged(const bool bChecked)
{
	if (UReEchoAudioService* AudioService = GetAudioService())
	{
		AudioService->SetBusMuted(EReEchoAudioBus::Music, bChecked);
	}
}

void UReEchoSettingsWidget::HandleAmbienceMuteChanged(const bool bChecked)
{
	if (UReEchoAudioService* AudioService = GetAudioService())
	{
		AudioService->SetBusMuted(EReEchoAudioBus::Ambience, bChecked);
	}
}

void UReEchoSettingsWidget::HandleCombatSfxMuteChanged(const bool bChecked)
{
	if (UReEchoAudioService* AudioService = GetAudioService())
	{
		AudioService->SetBusMuted(EReEchoAudioBus::CombatSfx, bChecked);
	}
}

void UReEchoSettingsWidget::HandleUiSfxMuteChanged(const bool bChecked)
{
	if (UReEchoAudioService* AudioService = GetAudioService())
	{
		AudioService->SetBusMuted(EReEchoAudioBus::UiSfx, bChecked);
	}
}

void UReEchoSettingsWidget::HandleDiagnosticToneChanged(const bool bChecked)
{
	if (bRefreshingAudioControls || !bChecked)
	{
		return;
	}
	if (UReEchoAudioService* AudioService = GetAudioService())
	{
		AudioService->PlayDiagnosticTone(this);
	}
	TGuardValue<bool> RefreshGuard(bRefreshingAudioControls, true);
	if (DiagnosticToneCheckBox)
	{
		DiagnosticToneCheckBox->SetIsChecked(false);
	}
}
