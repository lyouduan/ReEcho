#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ComboBoxString.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "ReEchoAudioTypes.h"
#include "UI/ReEchoIndexedButton.h"
#include "UI/ReEchoSettingsWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoSettingsInteractionTest,
                                 "ReEcho.UI.SettingsInteraction",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoSettingsInteractionTest::RunTest(const FString& Parameters)
{
	UClass* SettingsClass =
	    LoadClass<UReEchoSettingsWidget>(nullptr, TEXT("/Game/ReEcho/UI/WBP_ReEchoSettings.WBP_ReEchoSettings_C"));
	TestNotNull(TEXT("Designed settings widget class loads"), SettingsClass);
	if (!SettingsClass)
	{
		return false;
	}

	UReEchoSettingsWidget* SettingsWidget = NewObject<UReEchoSettingsWidget>(GetTransientPackage(), SettingsClass);
	TestNotNull(TEXT("Designed settings widget can be instantiated"), SettingsWidget);
	if (!SettingsWidget)
	{
		return false;
	}
	TestTrue(TEXT("Designed settings widget initializes its widget tree"), SettingsWidget->Initialize());
	SettingsWidget->TakeWidget();

	UReEchoIndexedButton* GraphicsTab =
	    Cast<UReEchoIndexedButton>(SettingsWidget->GetWidgetFromName(TEXT("GraphicsSettingsButton")));
	UReEchoIndexedButton* AudioTab =
	    Cast<UReEchoIndexedButton>(SettingsWidget->GetWidgetFromName(TEXT("AudioSettingsButton")));
	UReEchoIndexedButton* ControlsTab =
	    Cast<UReEchoIndexedButton>(SettingsWidget->GetWidgetFromName(TEXT("ControlsSettingsButton")));
	UTexture2D* LightTabTexture = LoadObject<UTexture2D>(
	    nullptr, TEXT("/Game/ReEcho/Textures/UI/InteractionPlaceholder/Settings/T_UI_Settings_TabLight"));
	UTexture2D* DarkTabTexture = LoadObject<UTexture2D>(
	    nullptr, TEXT("/Game/ReEcho/Textures/UI/InteractionPlaceholder/Settings/T_UI_Settings_TabDark"));
	UTexture2D* SliderThumbTexture = LoadObject<UTexture2D>(
	    nullptr, TEXT("/Game/ReEcho/Textures/UI/InteractionPlaceholder/Settings/T_UI_Settings_SliderThumb"));
	UTexture2D* LightActionTexture = LoadObject<UTexture2D>(
	    nullptr, TEXT("/Game/ReEcho/Textures/UI/InteractionPlaceholder/Settings/T_UI_Settings_ActionButtonLight"));
	UTexture2D* DarkActionTexture = LoadObject<UTexture2D>(
	    nullptr, TEXT("/Game/ReEcho/Textures/UI/InteractionPlaceholder/Settings/T_UI_Settings_ActionButtonDark"));
	TestNotNull(TEXT("Graphics tab exists"), GraphicsTab);
	TestNotNull(TEXT("Audio tab exists"), AudioTab);
	TestNotNull(TEXT("Controls tab exists"), ControlsTab);
	TestNotNull(TEXT("Light Settings tab art loads"), LightTabTexture);
	TestNotNull(TEXT("Dark Settings tab art loads"), DarkTabTexture);
	TestNotNull(TEXT("Settings slider thumb art loads"), SliderThumbTexture);
	TestNotNull(TEXT("Light Settings action-button art loads"), LightActionTexture);
	TestNotNull(TEXT("Dark Settings action-button art loads"), DarkActionTexture);
	if (GraphicsTab && AudioTab && ControlsTab && LightTabTexture && DarkTabTexture)
	{
		TestEqual(TEXT("Current graphics page uses light tab art"),
		          GraphicsTab->GetStyle().Normal.GetResourceObject(),
		          static_cast<UObject*>(LightTabTexture));
		TestEqual(TEXT("Inactive audio page uses dark tab art"),
		          AudioTab->GetStyle().Normal.GetResourceObject(),
		          static_cast<UObject*>(DarkTabTexture));
		TestEqual(TEXT("Inactive controls page uses dark tab art"),
		          ControlsTab->GetStyle().Normal.GetResourceObject(),
		          static_cast<UObject*>(DarkTabTexture));

		AudioTab->OnIndexedClicked.Broadcast(1);
		TestEqual(TEXT("Inactive graphics page switches to dark tab art"),
		          GraphicsTab->GetStyle().Normal.GetResourceObject(),
		          static_cast<UObject*>(DarkTabTexture));
		TestEqual(TEXT("Current audio page switches to light tab art"),
		          AudioTab->GetStyle().Normal.GetResourceObject(),
		          static_cast<UObject*>(LightTabTexture));
	}
	TestEqual(TEXT("Designed settings keeps its authored root"),
	          SettingsWidget->WidgetTree->RootWidget->GetFName(),
	          FName(TEXT("CanvasPanel_0")));
	const UBorder* SettingsDimmer = Cast<UBorder>(SettingsWidget->GetWidgetFromName(TEXT("SettingsDimmer")));
	TestNotNull(TEXT("Settings keeps its authored full-screen dimmer"), SettingsDimmer);
	TestTrue(TEXT("Settings dimmer is opaque black instead of a colored panel"),
	         SettingsDimmer && SettingsDimmer->GetBrushColor().R <= KINDA_SMALL_NUMBER &&
	             SettingsDimmer->GetBrushColor().G <= KINDA_SMALL_NUMBER &&
	             SettingsDimmer->GetBrushColor().B <= KINDA_SMALL_NUMBER &&
	             FMath::IsNearlyEqual(SettingsDimmer->GetBrushColor().A, 1.00f, 0.01f));
	UButton* RestoreDefaults = Cast<UButton>(SettingsWidget->GetWidgetFromName(TEXT("RestoreDefaultsButton")));
	UButton* ApplyAndReturn = Cast<UButton>(SettingsWidget->GetWidgetFromName(TEXT("ApplyAndReturnButton")));
	TestNotNull(TEXT("Restore-defaults action button exists"), RestoreDefaults);
	TestNotNull(TEXT("Apply action button exists"), ApplyAndReturn);
	const UCanvasPanelSlot* RestoreDesignerSlot =
	    RestoreDefaults ? Cast<UCanvasPanelSlot>(RestoreDefaults->Slot) : nullptr;
	const UCanvasPanelSlot* ApplyDesignerSlot = ApplyAndReturn ? Cast<UCanvasPanelSlot>(ApplyAndReturn->Slot) : nullptr;
	TestNotNull(TEXT("Restore-defaults position is authored in a Designer Canvas slot"), RestoreDesignerSlot);
	TestNotNull(TEXT("Apply position is authored in a Designer Canvas slot"), ApplyDesignerSlot);
	TestEqual(TEXT("Restore-defaults remains directly adjustable in SettingsLayoutCanvas"),
	          RestoreDefaults && RestoreDefaults->GetParent() ? RestoreDefaults->GetParent()->GetFName() : NAME_None,
	          FName(TEXT("SettingsLayoutCanvas")));
	TestEqual(TEXT("Apply remains directly adjustable in SettingsLayoutCanvas"),
	          ApplyAndReturn && ApplyAndReturn->GetParent() ? ApplyAndReturn->GetParent()->GetFName() : NAME_None,
	          FName(TEXT("SettingsLayoutCanvas")));
	TestEqual(TEXT("Restore-defaults uses the delivered light button art"),
	          RestoreDefaults ? RestoreDefaults->GetStyle().Normal.GetResourceObject() : nullptr,
	          static_cast<UObject*>(LightActionTexture));
	TestEqual(TEXT("Apply uses the delivered dark button art"),
	          ApplyAndReturn ? ApplyAndReturn->GetStyle().Normal.GetResourceObject() : nullptr,
	          static_cast<UObject*>(DarkActionTexture));
	for (const FName LegacyWidgetName : {FName(TEXT("ArtSettingsPanel")),
	                                     FName(TEXT("RootPanel")),
	                                     FName(TEXT("CategoryTitleText")),
	                                     FName(TEXT("DetailText")),
	                                     FName(TEXT("RestoreDefaultsButtonLabel")),
	                                     FName(TEXT("ApplyAndReturnButtonLabel")),
	                                     FName(TEXT("GraphicsPlaceholderNotice")),
	                                     FName(TEXT("ControlsPlaceholderNotice")),
	                                     FName(TEXT("Graphic_1")),
	                                     FName(TEXT("Graphic")),
	                                     FName(TEXT("Audio_1")),
	                                     FName(TEXT("Audio")),
	                                     FName(TEXT("Input_1")),
	                                     FName(TEXT("Input")),
	                                     FName(TEXT("GraphicsSettingsButtonLabel")),
	                                     FName(TEXT("AudioSettingsButtonLabel")),
	                                     FName(TEXT("ControlsSettingsButtonLabel")),
	                                     FName(TEXT("HorizontalBox_128")),
	                                     FName(TEXT("HorizontalBox")),
	                                     FName(TEXT("HorizontalBox_1")),
	                                     FName(TEXT("HorizontalBox_2")),
	                                     FName(TEXT("HorizontalBox_3"))})
	{
		TestNull(*FString::Printf(TEXT("Legacy Settings node %s was removed"), *LegacyWidgetName.ToString()),
		         SettingsWidget->GetWidgetFromName(LegacyWidgetName));
	}

	UComboBoxString* ResolutionCombo =
	    Cast<UComboBoxString>(SettingsWidget->GetWidgetFromName(TEXT("GraphicsResolutionComboBox")));
	UComboBoxString* OutputCombo =
	    Cast<UComboBoxString>(SettingsWidget->GetWidgetFromName(TEXT("AudioOutputComboBox")));
	UComboBoxString* DisplayModeCombo =
	    Cast<UComboBoxString>(SettingsWidget->GetWidgetFromName(TEXT("GraphicsDisplayModeComboBox")));
	UComboBoxString* QualityCombo =
	    Cast<UComboBoxString>(SettingsWidget->GetWidgetFromName(TEXT("GraphicsQualityComboBox")));
	UComboBoxString* VSyncCombo =
	    Cast<UComboBoxString>(SettingsWidget->GetWidgetFromName(TEXT("GraphicsVSyncComboBox")));
	TestNotNull(TEXT("Resolution field has a real combo box interaction layer"), ResolutionCombo);
	TestTrue(TEXT("Resolution combo box exposes selectable options"),
	         ResolutionCombo && ResolutionCombo->GetOptionCount() >= 4);
	TestNotNull(TEXT("Display mode field has a real combo box interaction layer"), DisplayModeCombo);
	TestNotNull(TEXT("Quality field has a real combo box interaction layer"), QualityCombo);
	TestNotNull(TEXT("VSync field has a real combo box interaction layer"), VSyncCombo);
	TestNotNull(TEXT("Audio output field has a real combo box interaction layer"), OutputCombo);
	TestTrue(TEXT("Audio output combo box contains the platform default endpoint"),
	         OutputCombo && OutputCombo->FindOptionIndex(TEXT("系统默认")) != INDEX_NONE);
	UCanvasPanel* AudioDesignerCanvas =
	    Cast<UCanvasPanel>(SettingsWidget->GetWidgetFromName(TEXT("AudioDesignerCanvas")));
	UOverlay* AudioOutputField = Cast<UOverlay>(SettingsWidget->GetWidgetFromName(TEXT("AudioOutputField")));
	const UCanvasPanelSlot* AudioOutputFieldSlot =
	    AudioOutputField ? Cast<UCanvasPanelSlot>(AudioOutputField->Slot) : nullptr;
	const UCanvasPanelSlot* AudioOutputInteractionSlot =
	    OutputCombo ? Cast<UCanvasPanelSlot>(OutputCombo->Slot) : nullptr;
	UImage* AudioOutputFieldBackground =
	    Cast<UImage>(SettingsWidget->GetWidgetFromName(TEXT("AudioOutputFieldBackground")));
	TestNotNull(TEXT("Audio page exposes a designer-owned Canvas"), AudioDesignerCanvas);
	TestNotNull(TEXT("Audio output frame has a freely adjustable Canvas slot"), AudioOutputFieldSlot);
	TestNotNull(TEXT("Audio output interaction follows the authored Canvas slot"), AudioOutputInteractionSlot);
	TestTrue(TEXT("Audio output interaction copies the authored frame position"),
	         AudioOutputFieldSlot && AudioOutputInteractionSlot &&
	             AudioOutputFieldSlot->GetPosition().Equals(AudioOutputInteractionSlot->GetPosition(), 0.1f));
	TestTrue(TEXT("Audio output interaction copies the authored frame size"),
	         AudioOutputFieldSlot && AudioOutputInteractionSlot &&
	             AudioOutputFieldSlot->GetSize().Equals(AudioOutputInteractionSlot->GetSize(), 0.1f));
	TestTrue(TEXT("Audio output frame matches the 802 px Graphics dropdown width"),
	         AudioOutputFieldSlot && FMath::IsNearlyEqual(AudioOutputFieldSlot->GetSize().X, 802.0f) &&
	             AudioOutputFieldBackground &&
	             FMath::IsNearlyEqual(AudioOutputFieldBackground->GetBrush().GetImageSize().X, 802.0f));
	for (const FName DesignerAudioControl : {FName(TEXT("MasterVolumeLabel")),
	                                         FName(TEXT("MasterVolumeVisualOverlay")),
	                                         FName(TEXT("MasterVolumePercent")),
	                                         FName(TEXT("MasterMuteCheckBox")),
	                                         FName(TEXT("MusicVolumeLabel")),
	                                         FName(TEXT("MusicVolumeVisualOverlay")),
	                                         FName(TEXT("MusicVolumePercent")),
	                                         FName(TEXT("MusicMuteCheckBox")),
	                                         FName(TEXT("CombatSfxVolumeLabel")),
	                                         FName(TEXT("CombatVolumeVisualOverlay")),
	                                         FName(TEXT("CombatVolumePercent")),
	                                         FName(TEXT("CombatSfxMuteCheckBox")),
	                                         FName(TEXT("AudioOutputLabel")),
	                                         FName(TEXT("AudioOutputField"))})
	{
		UWidget* Control = SettingsWidget->GetWidgetFromName(DesignerAudioControl);
		TestNotNull(*FString::Printf(TEXT("Designer audio control %s exists"), *DesignerAudioControl.ToString()),
		            Control);
		TestTrue(*FString::Printf(TEXT("%s can be freely moved in the UMG Designer"), *DesignerAudioControl.ToString()),
		         Control && Cast<UCanvasPanelSlot>(Control->Slot) != nullptr &&
		             Control->GetParent() == AudioDesignerCanvas);
	}
	TArray<UWidget*> AllSettingsWidgets;
	SettingsWidget->WidgetTree->GetAllWidgets(AllSettingsWidgets);
	for (const FName StableInteractionName : {FName(TEXT("GraphicsResolutionComboBox")),
	                                          FName(TEXT("GraphicsDisplayModeComboBox")),
	                                          FName(TEXT("GraphicsQualityComboBox")),
	                                          FName(TEXT("GraphicsVSyncComboBox")),
	                                          FName(TEXT("AudioOutputComboBox")),
	                                          FName(TEXT("GraphicsBrightnessSlider"))})
	{
		int32 MatchCount = 0;
		for (const UWidget* Widget : AllSettingsWidgets)
		{
			MatchCount += Widget && Widget->GetFName() == StableInteractionName ? 1 : 0;
		}
		TestEqual(*FString::Printf(TEXT("%s has one idempotent interaction layer"), *StableInteractionName.ToString()),
		          MatchCount,
		          1);
	}

	UImage* MasterTrack = Cast<UImage>(SettingsWidget->GetWidgetFromName(TEXT("MasterVolumeTrack")));
	UImage* MasterFill = Cast<UImage>(SettingsWidget->GetWidgetFromName(TEXT("MasterVolumeFill")));
	TestNotNull(TEXT("Master volume art track exists"), MasterTrack);
	TestNotNull(TEXT("Master volume art fill exists"), MasterFill);
	TestEqual(TEXT("Art track cannot intercept slider pointer events"),
	          MasterTrack ? MasterTrack->GetVisibility() : ESlateVisibility::Visible,
	          ESlateVisibility::HitTestInvisible);

	USlider* MasterSlider = Cast<USlider>(SettingsWidget->GetWidgetFromName(TEXT("MasterVolumeSlider")));
	USlider* BrightnessSlider = Cast<USlider>(SettingsWidget->GetWidgetFromName(TEXT("GraphicsBrightnessSlider")));
	UOverlay* MasterOverlay = Cast<UOverlay>(SettingsWidget->GetWidgetFromName(TEXT("MasterVolumeVisualOverlay")));
	UOverlay* CombatOverlay = Cast<UOverlay>(SettingsWidget->GetWidgetFromName(TEXT("CombatVolumeVisualOverlay")));
	UTextBlock* MasterPercent = Cast<UTextBlock>(SettingsWidget->GetWidgetFromName(TEXT("MasterVolumePercent")));
	TestNotNull(TEXT("Master volume uses a real slider"), MasterSlider);
	TestNotNull(TEXT("Brightness uses a real slider"), BrightnessSlider);
	if (MasterSlider && BrightnessSlider && SliderThumbTexture)
	{
		TestEqual(TEXT("Master volume uses the delivered slider thumb"),
		          MasterSlider->GetWidgetStyle().NormalThumbImage.GetResourceObject(),
		          static_cast<UObject*>(SliderThumbTexture));
		TestEqual(TEXT("Brightness inherits the delivered slider thumb"),
		          BrightnessSlider->GetWidgetStyle().NormalThumbImage.GetResourceObject(),
		          static_cast<UObject*>(SliderThumbTexture));
		TestEqual(TEXT("Authored audio slider is no longer an invisible interaction overlay"),
		          MasterSlider->GetRenderOpacity(),
		          1.0f);
		TestEqual(TEXT("Authored audio slider thumb tint is visible"), MasterSlider->GetSliderHandleColor().A, 1.0f);
		TestFalse(TEXT("Audio thumb reaches both authored track endpoints"), MasterSlider->HasIndentHandle());
		TestFalse(TEXT("Brightness thumb uses the same full endpoint range"), BrightnessSlider->HasIndentHandle());
	}
	TestNotNull(TEXT("Delivered settings widget uses the compact effects layout"), CombatOverlay);
	const TConstArrayView<EReEchoAudioBus> EffectsBuses = UReEchoSettingsWidget::GetCompactEffectsBuses();
	TestTrue(TEXT("Compact effects slider controls ambience loops"), EffectsBuses.Contains(EReEchoAudioBus::Ambience));
	TestTrue(TEXT("Compact effects slider controls combat SFX"), EffectsBuses.Contains(EReEchoAudioBus::CombatSfx));
	TestTrue(TEXT("Compact effects slider controls UI SFX"), EffectsBuses.Contains(EReEchoAudioBus::UiSfx));
	TestNotNull(TEXT("Master volume slider overlay exists"), MasterOverlay);
	const UCanvasPanelSlot* MasterOverlayDesignerSlot =
	    MasterOverlay ? Cast<UCanvasPanelSlot>(MasterOverlay->Slot) : nullptr;
	TestNotNull(TEXT("Master volume overlay has a freely adjustable Canvas slot"), MasterOverlayDesignerSlot);
	TestTrue(TEXT("Audio track keeps its authored 802 px Designer width"),
	         MasterOverlayDesignerSlot && FMath::IsNearlyEqual(MasterOverlayDesignerSlot->GetSize().X, 802.0f));
	TestTrue(TEXT("Audio track art keeps the same 802 px width as the brightness track"),
	         MasterTrack && FMath::IsNearlyEqual(MasterTrack->GetBrush().GetImageSize().X, 802.0f));
	TestEqual(TEXT("Slider overlay preserves child pointer input"),
	          MasterOverlay ? MasterOverlay->GetVisibility() : ESlateVisibility::Visible,
	          ESlateVisibility::SelfHitTestInvisible);
	TestTrue(TEXT("Master volume slider remains enabled"), MasterSlider && MasterSlider->GetIsEnabled());
	TestFalse(TEXT("Master volume slider remains unlocked"), MasterSlider && MasterSlider->IsLocked());
	const UOverlaySlot* MasterSliderSlot = MasterSlider ? Cast<UOverlaySlot>(MasterSlider->Slot) : nullptr;
	TestNotNull(TEXT("Master volume slider uses an overlay slot"), MasterSliderSlot);
	TestEqual(TEXT("Master volume slider covers the full track width"),
	          MasterSliderSlot ? MasterSliderSlot->GetHorizontalAlignment() : HAlign_Left,
	          HAlign_Fill);
	TestEqual(TEXT("Master volume slider covers the full track height"),
	          MasterSliderSlot ? MasterSliderSlot->GetVerticalAlignment() : VAlign_Top,
	          VAlign_Fill);
	const FMargin MasterSliderPadding = MasterSliderSlot ? MasterSliderSlot->GetPadding() : FMargin(-1.0f);
	TestTrue(TEXT("Master volume slider travel range has no legacy horizontal padding"),
	         MasterSliderSlot && FMath::IsNearlyZero(MasterSliderPadding.Left) &&
	             FMath::IsNearlyZero(MasterSliderPadding.Right));
	TestNotNull(TEXT("Master volume percentage text exists"), MasterPercent);
	if (MasterSlider)
	{
		MasterSlider->OnValueChanged.Broadcast(0.42f);
	}
	TestEqual(TEXT("Dragging the slider updates its delivered percentage label"),
	          MasterPercent ? MasterPercent->GetText().ToString() : FString(),
	          FString(TEXT("42%")));
	constexpr float ExpectedFillScaleAt42Percent = ((25.0f / 802.0f) + 0.42f * (1.0f - 50.0f / 802.0f)) / 0.65f;
	TestTrue(TEXT("Decorative fill boundary follows the full-range thumb center"),
	         MasterFill &&
	             FMath::IsNearlyEqual(MasterFill->GetRenderTransform().Scale.X, ExpectedFillScaleAt42Percent));
	return true;
}

#endif
