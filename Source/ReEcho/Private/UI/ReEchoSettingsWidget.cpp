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
#include "ReEchoAudioEvents.h"
#include "ReEchoAudioService.h"
#include "Components/Slider.h"
#include "Components/CheckBox.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ComboBoxString.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"

TConstArrayView<EReEchoAudioBus> UReEchoSettingsWidget::GetCompactEffectsBuses()
{
	static constexpr EReEchoAudioBus EffectsBuses[] = {
	    EReEchoAudioBus::Ambience, EReEchoAudioBus::CombatSfx, EReEchoAudioBus::UiSfx};
	return EffectsBuses;
}

bool UReEchoSettingsWidget::UsesCompactAudioLayout() const
{
	// The delivered three-slider WBP owns this visual overlay. The optional
	// dedicated sliders can still exist as inherited Blueprint members, so their
	// pointer presence is not a reliable way to identify the visible layout.
	return WidgetTree && WidgetTree->FindWidget(TEXT("CombatVolumeVisualOverlay"));
}

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
	InitialDisplayGamma = GEngine ? GEngine->DisplayGamma : 2.2f;
	PendingBrightness = FMath::Clamp(InitialDisplayGamma - 1.55f, 0.0f, 1.0f);
	BuildInteractiveSettingsControls();
	bAudioSettingsApplied = false;
	bGraphicsSettingsApplied = false;
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
	if (SettingsCloseButton)
	{
		SettingsCloseButton->OnClicked.AddUniqueDynamic(this, &UReEchoSettingsWidget::HandleCloseClicked);
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
	if (!bGraphicsSettingsApplied)
	{
		if (GEngine)
		{
			GEngine->DisplayGamma = InitialDisplayGamma;
		}
		if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
		{
			Settings->LoadSettings(false);
			Settings->ApplyNonResolutionSettings();
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

UComboBoxString* UReEchoSettingsWidget::AddComboBoxOverlay(const FName ComboName,
	const FName FieldName,
	const FName ValueName,
	const FName ArrowName)
{
	UWidget* FieldWidget = WidgetTree ? WidgetTree->FindWidget(FieldName) : nullptr;
	UPanelWidget* Parent = FieldWidget ? FieldWidget->GetParent() : nullptr;
	if (!FieldWidget || !Parent)
	{
		return nullptr;
	}

	UComboBoxString* ComboBox = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), ComboName);
	ComboBox->SetHasDownArrow(false);
	ComboBox->SetMaxListHeight(320.0f);
	ComboBox->SetContentPadding(FMargin(24.0f, 0.0f));
	ComboBox->OnGenerateWidgetEvent.BindDynamic(this, &UReEchoSettingsWidget::GenerateComboBoxItem);

	FComboBoxStyle ComboStyle = ComboBox->GetWidgetStyle();
	FButtonStyle& ButtonStyle = ComboStyle.ComboButtonStyle.ButtonStyle;
	ButtonStyle.Normal.DrawAs = ESlateBrushDrawType::NoDrawType;
	ButtonStyle.Hovered.DrawAs = ESlateBrushDrawType::NoDrawType;
	ButtonStyle.Pressed.DrawAs = ESlateBrushDrawType::NoDrawType;
	ButtonStyle.Disabled.DrawAs = ESlateBrushDrawType::NoDrawType;
	ComboStyle.ComboButtonStyle.DownArrowImage.DrawAs = ESlateBrushDrawType::NoDrawType;
	ComboStyle.ComboButtonStyle.MenuBorderBrush.DrawAs = ESlateBrushDrawType::Box;
	ComboStyle.ComboButtonStyle.MenuBorderBrush.TintColor = FSlateColor(FLinearColor::White);
	ComboBox->SetWidgetStyle(ComboStyle);
	FTableRowStyle ItemStyle = ComboBox->GetItemStyle();
	FSlateBrush RowBrush;
	RowBrush.DrawAs = ESlateBrushDrawType::Box;
	RowBrush.TintColor = FSlateColor(FLinearColor::White);
	FSlateBrush HoveredRowBrush = RowBrush;
	HoveredRowBrush.TintColor = FSlateColor(FLinearColor(0.78f, 0.88f, 0.92f, 1.0f));
	ItemStyle.SetEvenRowBackgroundBrush(RowBrush)
		.SetOddRowBackgroundBrush(RowBrush)
		.SetEvenRowBackgroundHoveredBrush(HoveredRowBrush)
		.SetOddRowBackgroundHoveredBrush(HoveredRowBrush)
		.SetActiveBrush(HoveredRowBrush)
		.SetActiveHoveredBrush(HoveredRowBrush)
		.SetInactiveBrush(HoveredRowBrush)
		.SetInactiveHoveredBrush(HoveredRowBrush)
		.SetTextColor(FSlateColor(FLinearColor(0.12f, 0.12f, 0.12f, 1.0f)))
		.SetSelectedTextColor(FSlateColor(FLinearColor(0.12f, 0.12f, 0.12f, 1.0f)));
	ComboBox->SetItemStyle(ItemStyle);

	UPanelSlot* NewSlot = nullptr;
	if (const UHorizontalBoxSlot* SourceHorizontalSlot = Cast<UHorizontalBoxSlot>(FieldWidget->Slot))
	{
		UHorizontalBox* Row = Cast<UHorizontalBox>(Parent);
		if (!Row)
		{
			return nullptr;
		}
		const int32 FieldIndex = Row->GetChildIndex(FieldWidget);
		const FSlateChildSize FieldSize = SourceHorizontalSlot->GetSize();
		const FMargin FieldPadding = SourceHorizontalSlot->GetPadding();
		const EHorizontalAlignment FieldHAlign = SourceHorizontalSlot->GetHorizontalAlignment();
		const EVerticalAlignment FieldVAlign = SourceHorizontalSlot->GetVerticalAlignment();

		Row->RemoveChild(FieldWidget);
		UOverlay* FieldOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(),
			*FString::Printf(TEXT("%sOverlay"), *ComboName.ToString()));
		Row->InsertChildAt(FieldIndex, FieldOverlay);
		if (UHorizontalBoxSlot* OverlayRowSlot = Cast<UHorizontalBoxSlot>(FieldOverlay->Slot))
		{
			OverlayRowSlot->SetSize(FieldSize);
			OverlayRowSlot->SetPadding(FieldPadding);
			OverlayRowSlot->SetHorizontalAlignment(FieldHAlign);
			OverlayRowSlot->SetVerticalAlignment(FieldVAlign);
		}
		UOverlaySlot* FieldSlot = FieldOverlay->AddChildToOverlay(FieldWidget);
		FieldSlot->SetHorizontalAlignment(HAlign_Fill);
		FieldSlot->SetVerticalAlignment(VAlign_Fill);
		NewSlot = FieldOverlay->AddChild(ComboBox);
		if (UOverlaySlot* ComboOverlaySlot = Cast<UOverlaySlot>(NewSlot))
		{
			ComboOverlaySlot->SetHorizontalAlignment(HAlign_Fill);
			ComboOverlaySlot->SetVerticalAlignment(VAlign_Fill);
		}
		if (UWidget* ArrowWidget = WidgetTree->FindWidget(ArrowName))
		{
			if (UPanelWidget* ArrowParent = ArrowWidget->GetParent())
			{
				ArrowParent->RemoveChild(ArrowWidget);
			}
			UOverlaySlot* ArrowSlot = FieldOverlay->AddChildToOverlay(ArrowWidget);
			ArrowSlot->SetHorizontalAlignment(HAlign_Right);
			ArrowSlot->SetVerticalAlignment(VAlign_Center);
			ArrowSlot->SetPadding(FMargin(0.0f, 0.0f, 18.0f, 0.0f));
		}
	}
	else
	{
		NewSlot = Parent->AddChild(ComboBox);
	}
	if (const UCanvasPanelSlot* SourceCanvasSlot = Cast<UCanvasPanelSlot>(FieldWidget->Slot))
	{
		if (UCanvasPanelSlot* TargetCanvasSlot = Cast<UCanvasPanelSlot>(NewSlot))
		{
			TargetCanvasSlot->SetLayout(SourceCanvasSlot->GetLayout());
			TargetCanvasSlot->SetAlignment(SourceCanvasSlot->GetAlignment());
			TargetCanvasSlot->SetAutoSize(SourceCanvasSlot->GetAutoSize());
			TargetCanvasSlot->SetZOrder(SourceCanvasSlot->GetZOrder() + 10);
		}
	}
	else if (const UOverlaySlot* SourceOverlaySlot = Cast<UOverlaySlot>(FieldWidget->Slot))
	{
		if (UOverlaySlot* TargetOverlaySlot = Cast<UOverlaySlot>(NewSlot))
		{
			TargetOverlaySlot->SetPadding(SourceOverlaySlot->GetPadding());
			TargetOverlaySlot->SetHorizontalAlignment(SourceOverlaySlot->GetHorizontalAlignment());
			TargetOverlaySlot->SetVerticalAlignment(SourceOverlaySlot->GetVerticalAlignment());
		}
	}
	else if (!Cast<UHorizontalBoxSlot>(FieldWidget->Slot))
	{
		UE_LOG(LogTemp,
		       Warning,
		       TEXT("[ReEchoSettings] Cannot overlay %s on %s: parent=%s slot=%s"),
		       *ComboName.ToString(),
		       *FieldName.ToString(),
		       *GetNameSafe(Parent),
		       *GetNameSafe(FieldWidget->Slot));
		Parent->RemoveChild(ComboBox);
		return nullptr;
	}

	if (UWidget* ValueWidget = WidgetTree->FindWidget(ValueName))
	{
		ValueWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (UWidget* ArrowWidget = WidgetTree->FindWidget(ArrowName))
	{
		ArrowWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	FieldWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	return ComboBox;
}

USlider* UReEchoSettingsWidget::AddSliderOverlay(const FName SliderName, const FName TrackName)
{
	UWidget* TrackWidget = WidgetTree ? WidgetTree->FindWidget(TrackName) : nullptr;
	UPanelWidget* Parent = TrackWidget ? TrackWidget->GetParent() : nullptr;
	if (!TrackWidget || !Parent)
	{
		return nullptr;
	}

	USlider* Slider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), SliderName);
	if (MasterVolumeSlider)
	{
		Slider->SetWidgetStyle(MasterVolumeSlider->GetWidgetStyle());
	}
	UPanelSlot* NewSlot = Parent->AddChild(Slider);
	if (const UCanvasPanelSlot* SourceCanvasSlot = Cast<UCanvasPanelSlot>(TrackWidget->Slot))
	{
		if (UCanvasPanelSlot* TargetCanvasSlot = Cast<UCanvasPanelSlot>(NewSlot))
		{
			TargetCanvasSlot->SetLayout(SourceCanvasSlot->GetLayout());
			TargetCanvasSlot->SetAlignment(SourceCanvasSlot->GetAlignment());
			TargetCanvasSlot->SetAutoSize(SourceCanvasSlot->GetAutoSize());
			TargetCanvasSlot->SetZOrder(SourceCanvasSlot->GetZOrder() + 10);
		}
	}
	else if (const UOverlaySlot* SourceOverlaySlot = Cast<UOverlaySlot>(TrackWidget->Slot))
	{
		if (UOverlaySlot* TargetOverlaySlot = Cast<UOverlaySlot>(NewSlot))
		{
			TargetOverlaySlot->SetPadding(SourceOverlaySlot->GetPadding());
			TargetOverlaySlot->SetHorizontalAlignment(SourceOverlaySlot->GetHorizontalAlignment());
			TargetOverlaySlot->SetVerticalAlignment(SourceOverlaySlot->GetVerticalAlignment());
		}
	}
	else
	{
		Parent->RemoveChild(Slider);
		return nullptr;
	}
	TrackWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	return Slider;
}

void UReEchoSettingsWidget::BuildInteractiveSettingsControls()
{
	if (GraphicsResolutionComboBox)
	{
		RefreshGraphicsControls();
		RefreshAudioControls();
		return;
	}
	static const FName NonInteractiveArt[] = {TEXT("MasterVolumeTrack"),
	                                          TEXT("MasterVolumeFill"),
	                                          TEXT("MusicVolumeTrack"),
	                                          TEXT("MusicVolumeFill"),
	                                          TEXT("CombatVolumeTrack"),
	                                          TEXT("CombatVolumeFill"),
	                                          TEXT("GraphicsBrightnessTrack"),
	                                          TEXT("GraphicsBrightnessFill")};
	for (const FName WidgetName : NonInteractiveArt)
	{
		if (UWidget* ArtWidget = WidgetTree ? WidgetTree->FindWidget(WidgetName) : nullptr)
		{
			ArtWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}
	// These overlays contain the real sliders. SelfHitTestInvisible keeps the
	// decorative container out of the hit path while allowing its Slider child
	// to receive mouse/controller input. HitTestInvisible would disable the
	// complete subtree and make the visible volume bars impossible to drag.
	static const FName SliderOverlays[] = {
	    TEXT("MasterVolumeVisualOverlay"), TEXT("MusicVolumeVisualOverlay"), TEXT("CombatVolumeVisualOverlay")};
	for (const FName WidgetName : SliderOverlays)
	{
		if (UWidget* OverlayWidget = WidgetTree ? WidgetTree->FindWidget(WidgetName) : nullptr)
		{
			OverlayWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
	}
	// The delivered WBP authored the transparent Slider children with Left/Top
	// alignment. Their desired width is tiny, so the wide visible track was not
	// actually covered by an input widget. Stretch every designed slider across
	// its overlay so the complete black/white bar is draggable.
	USlider* const DesignedVolumeSliders[] = {MasterVolumeSlider, MusicVolumeSlider, CombatSfxVolumeSlider};
	for (USlider* Slider : DesignedVolumeSliders)
	{
		if (Slider)
		{
			Slider->SetVisibility(ESlateVisibility::Visible);
			Slider->SetIsEnabled(true);
			Slider->SetLocked(false);
			if (UOverlaySlot* SliderSlot = Cast<UOverlaySlot>(Slider->Slot))
			{
				SliderSlot->SetHorizontalAlignment(HAlign_Fill);
				SliderSlot->SetVerticalAlignment(VAlign_Fill);
			}
		}
	}

	GraphicsResolutionComboBox = AddComboBoxOverlay(TEXT("GraphicsResolutionComboBox"), TEXT("GraphicsDropdown0"), TEXT("GraphicsValue0"), TEXT("GraphicsArrow0"));
	GraphicsDisplayModeComboBox = AddComboBoxOverlay(TEXT("GraphicsDisplayModeComboBox"), TEXT("GraphicsDropdown1"), TEXT("GraphicsValue1"), TEXT("GraphicsArrow1"));
	GraphicsQualityComboBox = AddComboBoxOverlay(TEXT("GraphicsQualityComboBox"), TEXT("GraphicsDropdown2"), TEXT("GraphicsValue2"), TEXT("GraphicsArrow2"));
	GraphicsVSyncComboBox = AddComboBoxOverlay(TEXT("GraphicsVSyncComboBox"), TEXT("GraphicsDropdown3"), TEXT("GraphicsValue3"), TEXT("GraphicsArrow3"));
	AudioOutputComboBox = AddComboBoxOverlay(TEXT("AudioOutputComboBox"), TEXT("AudioOutputField"), TEXT("AudioOutputValue"), TEXT("AudioOutputArrow"));

	if (GraphicsResolutionComboBox)
	{
		GraphicsResolutionComboBox->OnSelectionChanged.AddUniqueDynamic(this, &UReEchoSettingsWidget::HandleResolutionChanged);
	}
	if (GraphicsDisplayModeComboBox)
	{
		GraphicsDisplayModeComboBox->OnSelectionChanged.AddUniqueDynamic(this, &UReEchoSettingsWidget::HandleDisplayModeChanged);
	}
	if (GraphicsQualityComboBox)
	{
		GraphicsQualityComboBox->OnSelectionChanged.AddUniqueDynamic(this, &UReEchoSettingsWidget::HandleGraphicsQualityChanged);
	}
	if (GraphicsVSyncComboBox)
	{
		GraphicsVSyncComboBox->OnSelectionChanged.AddUniqueDynamic(this, &UReEchoSettingsWidget::HandleVSyncChanged);
	}
	if (AudioOutputComboBox)
	{
		AudioOutputComboBox->AddOption(TEXT("系统默认"));
		AudioOutputComboBox->SetSelectedOption(TEXT("系统默认"));
		AudioOutputComboBox->OnSelectionChanged.AddUniqueDynamic(this, &UReEchoSettingsWidget::HandleAudioOutputChanged);
	}

	GraphicsBrightnessSlider = AddSliderOverlay(TEXT("GraphicsBrightnessSlider"), TEXT("GraphicsBrightnessTrack"));
	if (GraphicsBrightnessSlider)
	{
		GraphicsBrightnessSlider->SetValue(PendingBrightness);
		GraphicsBrightnessSlider->OnValueChanged.AddUniqueDynamic(this, &UReEchoSettingsWidget::HandleBrightnessChanged);
		GraphicsBrightnessSlider->OnMouseCaptureEnd.AddUniqueDynamic(
			this, &UReEchoSettingsWidget::HandleSliderInteractionFinished);
		GraphicsBrightnessSlider->OnControllerCaptureEnd.AddUniqueDynamic(
			this, &UReEchoSettingsWidget::HandleSliderInteractionFinished);
	}
	RefreshGraphicsControls();
}

void UReEchoSettingsWidget::RefreshGraphicsControls()
{
	UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!Settings)
	{
		return;
	}
	TGuardValue<bool> RefreshGuard(bRefreshingGraphicsControls, true);

	if (GraphicsResolutionComboBox)
	{
		GraphicsResolutionComboBox->ClearOptions();
		const FString CurrentResolution = FString::Printf(TEXT("%d*%d"), Settings->GetScreenResolution().X, Settings->GetScreenResolution().Y);
		const FString ResolutionOptions[] = {TEXT("1280*720"), TEXT("1600*900"), TEXT("1920*1080"), TEXT("2560*1440"), TEXT("3840*2160")};
		for (const FString& Option : ResolutionOptions)
		{
			GraphicsResolutionComboBox->AddOption(Option);
		}
		if (GraphicsResolutionComboBox->FindOptionIndex(CurrentResolution) == INDEX_NONE)
		{
			GraphicsResolutionComboBox->AddOption(CurrentResolution);
		}
		GraphicsResolutionComboBox->SetSelectedOption(CurrentResolution);
	}
	if (GraphicsDisplayModeComboBox)
	{
		GraphicsDisplayModeComboBox->ClearOptions();
		GraphicsDisplayModeComboBox->AddOption(TEXT("全屏"));
		GraphicsDisplayModeComboBox->AddOption(TEXT("无边框窗口"));
		GraphicsDisplayModeComboBox->AddOption(TEXT("窗口"));
		const FString Mode = Settings->GetFullscreenMode() == EWindowMode::Fullscreen ? TEXT("全屏")
			: Settings->GetFullscreenMode() == EWindowMode::WindowedFullscreen ? TEXT("无边框窗口") : TEXT("窗口");
		GraphicsDisplayModeComboBox->SetSelectedOption(Mode);
	}
	if (GraphicsQualityComboBox)
	{
		GraphicsQualityComboBox->ClearOptions();
		GraphicsQualityComboBox->AddOption(TEXT("低"));
		GraphicsQualityComboBox->AddOption(TEXT("中"));
		GraphicsQualityComboBox->AddOption(TEXT("高"));
		GraphicsQualityComboBox->AddOption(TEXT("极高"));
		const int32 Quality = Settings->GetOverallScalabilityLevel();
		GraphicsQualityComboBox->SetSelectedOption(Quality <= 0 ? TEXT("低") : Quality == 1 ? TEXT("中") : Quality == 2 ? TEXT("高") : TEXT("极高"));
	}
	if (GraphicsVSyncComboBox)
	{
		GraphicsVSyncComboBox->ClearOptions();
		GraphicsVSyncComboBox->AddOption(TEXT("关闭"));
		GraphicsVSyncComboBox->AddOption(TEXT("开启"));
		GraphicsVSyncComboBox->SetSelectedOption(Settings->IsVSyncEnabled() ? TEXT("开启") : TEXT("关闭"));
	}
	if (GraphicsBrightnessSlider)
	{
		GraphicsBrightnessSlider->SetValue(PendingBrightness);
	}
	UpdateVolumeVisual(GraphicsBrightnessFill, GraphicsValue4, PendingBrightness);
}

UWidget* UReEchoSettingsWidget::GenerateComboBoxItem(FString Item)
{
	UTextBlock* ItemText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	ItemText->SetText(FText::FromString(Item));
	ItemText->SetJustification(ETextJustify::Center);
	ItemText->SetColorAndOpacity(FSlateColor(FLinearColor(0.12f, 0.12f, 0.12f, 1.0f)));
	FSlateFontInfo ItemFont = ItemText->GetFont();
	ItemFont.Size = 28;
	ItemText->SetFont(ItemFont);
	return ItemText;
}

void UReEchoSettingsWidget::UpdateVolumeVisual(UImage* FillImage, UTextBlock* PercentText, const float Value) const
{
	const float ClampedValue = FMath::Clamp(Value, 0.0f, 1.0f);
	if (FillImage)
	{
		// The delivered fill art was authored at 65%; scale from that baseline so
		// 0..100% maps to the complete visible track without replacing the asset.
		FillImage->SetRenderTransformPivot(FVector2D(0.0f, 0.5f));
		FillImage->SetRenderScale(FVector2D(ClampedValue / 0.65f, 1.0f));
	}
	if (PercentText)
	{
		PercentText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(ClampedValue * 100.0f))));
	}
}

void UReEchoSettingsWidget::PostUiEvent(const FName EventId) const
{
	if (UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(this))
	{
		if (UReEchoAudioService* AudioService = GameInstance->GetSubsystem<UReEchoAudioService>())
		{
			AudioService->PostEventById(GameInstance, EventId);
		}
	}
}

void UReEchoSettingsWidget::RefreshCategory()
{
	if (!CategoryTitleText)
	{
		return;
	}

	// The delivered layout uses one persistent modal title. The active tab already
	// communicates which category is open, so changing this heading caused the
	// title to drift into the white panel and diverge from the reference artwork.
	CategoryTitleText->SetText(FText::FromString(TEXT("游戏设置")));
	if (GraphicsPanel)
	{
		GraphicsPanel->SetVisibility(SelectedCategory == 0 ? ESlateVisibility::SelfHitTestInvisible
		                                                   : ESlateVisibility::Collapsed);
	}
	if (ControlsPanel)
	{
		ControlsPanel->SetVisibility(SelectedCategory == 2 ? ESlateVisibility::SelfHitTestInvisible
		                                                   : ESlateVisibility::Collapsed);
	}

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
			const bool bHasDesignedPanel = SelectedCategory == 2 ? ControlsPanel != nullptr : GraphicsPanel != nullptr;
			DetailText->SetVisibility(bHasDesignedPanel ? ESlateVisibility::Collapsed
			                                              : ESlateVisibility::SelfHitTestInvisible);
			DetailText->SetText(FText::FromString(Detail));
		}
		if (AudioPanel)
		{
			AudioPanel->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// Tab artwork is authored as a white active note. Tint inactive notes grey so
	// the same button remains keyboard/gamepad accessible while matching the
	// selected/inactive visual states from the delivery.
	const FLinearColor SettingsSelectedColor = FLinearColor::White;
	const FLinearColor SettingsNormalColor(0.72f, 0.72f, 0.72f, 1.0f);
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
	if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		Settings->SetToDefaults();
	}
	PendingBrightness = 0.65f;
	if (GEngine)
	{
		GEngine->DisplayGamma = FMath::Lerp(1.55f, 2.55f, PendingBrightness);
	}
	if (UReEchoAudioService* AudioService = GetAudioService())
	{
		AudioService->PreviewDefaultSettings();
	}
	RefreshGraphicsControls();
	RefreshAudioControls();
}

void UReEchoSettingsWidget::HandleApplyAndReturnClicked()
{
	if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		Settings->ApplySettings(false);
		bGraphicsSettingsApplied = true;
	}
	if (UReEchoAudioService* AudioService = GetAudioService())
	{
		bAudioSettingsApplied = AudioService->CommitUserSettings();
	}
	OnClosed.Broadcast();
}

void UReEchoSettingsWidget::HandleCloseClicked()
{
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
	USlider* const VolumeSliders[] = {
		MasterVolumeSlider, MusicVolumeSlider, AmbienceVolumeSlider, CombatSfxVolumeSlider, UiSfxVolumeSlider};
	for (USlider* Slider : VolumeSliders)
	{
		if (Slider)
		{
			Slider->OnMouseCaptureEnd.AddUniqueDynamic(this, &UReEchoSettingsWidget::HandleSliderInteractionFinished);
			Slider->OnControllerCaptureEnd.AddUniqueDynamic(
				this, &UReEchoSettingsWidget::HandleSliderInteractionFinished);
		}
	}
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
		const float Value = FMath::Clamp(AudioService->GetMasterVolume(), 0.0f, 1.0f);
		MasterVolumeSlider->SetValue(Value);
		UpdateVolumeVisual(MasterVolumeFill, MasterVolumePercent, Value);
	}
	if (MusicVolumeSlider)
	{
		const float Value = FMath::Clamp(AudioService->GetBusVolume(EReEchoAudioBus::Music), 0.0f, 1.0f);
		MusicVolumeSlider->SetValue(Value);
		UpdateVolumeVisual(MusicVolumeFill, MusicVolumePercent, Value);
	}
	if (AmbienceVolumeSlider)
	{
		AmbienceVolumeSlider->SetValue(FMath::Clamp(AudioService->GetBusVolume(EReEchoAudioBus::Ambience), 0.0f, 1.0f));
	}
	if (CombatSfxVolumeSlider)
	{
		const float Value = FMath::Clamp(AudioService->GetBusVolume(EReEchoAudioBus::CombatSfx), 0.0f, 1.0f);
		CombatSfxVolumeSlider->SetValue(Value);
		UpdateVolumeVisual(CombatVolumeFill, CombatVolumePercent, Value);
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
	UpdateVolumeVisual(MasterVolumeFill, MasterVolumePercent, Value);
	if (UReEchoAudioService* AudioService = GetAudioService())
	{
		AudioService->SetMasterVolume(Value);
	}
}

void UReEchoSettingsWidget::HandleMusicVolumeChanged(const float Value)
{
	UpdateVolumeVisual(MusicVolumeFill, MusicVolumePercent, Value);
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
	UpdateVolumeVisual(CombatVolumeFill, CombatVolumePercent, Value);
	if (UReEchoAudioService* AudioService = GetAudioService())
	{
		// The delivered settings artwork exposes one compact "音效音量"
		// control instead of separate Ambience/Combat/UI sliders. When that
		// compact visual layout is active, treat this slider as the aggregate
		// non-music control so loops such as Ambience_Rain follow it too. The
		// five-slider C++ fallback keeps its independent bus semantics.
		if (UsesCompactAudioLayout())
		{
			for (const EReEchoAudioBus Bus : GetCompactEffectsBuses())
			{
				AudioService->SetBusVolume(Bus, Value);
			}
		}
		else
		{
			AudioService->SetBusVolume(EReEchoAudioBus::CombatSfx, Value);
		}
	}
}

void UReEchoSettingsWidget::HandleUiSfxVolumeChanged(const float Value)
{
	if (UReEchoAudioService* AudioService = GetAudioService())
	{
		AudioService->SetBusVolume(EReEchoAudioBus::UiSfx, Value);
	}
}

void UReEchoSettingsWidget::HandleResolutionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (bRefreshingGraphicsControls || SelectionType == ESelectInfo::Direct)
	{
		return;
	}
	FString WidthText;
	FString HeightText;
	if (SelectedItem.Split(TEXT("*"), &WidthText, &HeightText))
	{
		if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
		{
			Settings->SetScreenResolution(FIntPoint(FCString::Atoi(*WidthText), FCString::Atoi(*HeightText)));
		}
	}
	PostUiEvent(FReEchoAudioEvents::UiConfirm);
}

void UReEchoSettingsWidget::HandleDisplayModeChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (bRefreshingGraphicsControls || SelectionType == ESelectInfo::Direct)
	{
		return;
	}
	if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		const EWindowMode::Type Mode = SelectedItem == TEXT("全屏") ? EWindowMode::Fullscreen
			: SelectedItem == TEXT("无边框窗口") ? EWindowMode::WindowedFullscreen : EWindowMode::Windowed;
		Settings->SetFullscreenMode(Mode);
	}
	PostUiEvent(FReEchoAudioEvents::UiConfirm);
}

void UReEchoSettingsWidget::HandleGraphicsQualityChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (bRefreshingGraphicsControls || SelectionType == ESelectInfo::Direct)
	{
		return;
	}
	if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		const int32 Quality = SelectedItem == TEXT("低") ? 0 : SelectedItem == TEXT("中") ? 1 : SelectedItem == TEXT("高") ? 2 : 3;
		Settings->SetOverallScalabilityLevel(Quality);
	}
	PostUiEvent(FReEchoAudioEvents::UiConfirm);
}

void UReEchoSettingsWidget::HandleVSyncChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (bRefreshingGraphicsControls || SelectionType == ESelectInfo::Direct)
	{
		return;
	}
	if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		Settings->SetVSyncEnabled(SelectedItem == TEXT("开启"));
	}
	PostUiEvent(FReEchoAudioEvents::UiConfirm);
}

void UReEchoSettingsWidget::HandleAudioOutputChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	// The current audio service exposes the platform default endpoint only. The
	// ComboBox is intentionally real and extensible; device enumeration can add
	// options here without changing the delivered layout.
	if (SelectionType != ESelectInfo::Direct)
	{
		PostUiEvent(FReEchoAudioEvents::UiConfirm);
	}
}

void UReEchoSettingsWidget::HandleBrightnessChanged(const float Value)
{
	if (bRefreshingGraphicsControls)
	{
		return;
	}
	PendingBrightness = FMath::Clamp(Value, 0.0f, 1.0f);
	UpdateVolumeVisual(GraphicsBrightnessFill, GraphicsValue4, PendingBrightness);
	if (GEngine)
	{
		GEngine->DisplayGamma = FMath::Lerp(1.55f, 2.55f, PendingBrightness);
	}
}

void UReEchoSettingsWidget::HandleSliderInteractionFinished()
{
	PostUiEvent(FReEchoAudioEvents::UiConfirm);
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
