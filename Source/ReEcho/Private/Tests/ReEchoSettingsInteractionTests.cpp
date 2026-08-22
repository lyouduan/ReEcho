#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Blueprint/WidgetTree.h"
#include "Components/ComboBoxString.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "ReEchoAudioTypes.h"
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
	TestEqual(TEXT("Designed settings keeps its authored root"),
	          SettingsWidget->WidgetTree->RootWidget->GetFName(),
	          FName(TEXT("CanvasPanel_0")));

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
	TestNotNull(TEXT("Master volume art track exists"), MasterTrack);
	TestEqual(TEXT("Art track cannot intercept slider pointer events"),
	          MasterTrack ? MasterTrack->GetVisibility() : ESlateVisibility::Visible,
	          ESlateVisibility::HitTestInvisible);

	USlider* MasterSlider = Cast<USlider>(SettingsWidget->GetWidgetFromName(TEXT("MasterVolumeSlider")));
	UOverlay* MasterOverlay = Cast<UOverlay>(SettingsWidget->GetWidgetFromName(TEXT("MasterVolumeVisualOverlay")));
	UOverlay* CombatOverlay = Cast<UOverlay>(SettingsWidget->GetWidgetFromName(TEXT("CombatVolumeVisualOverlay")));
	UTextBlock* MasterPercent = Cast<UTextBlock>(SettingsWidget->GetWidgetFromName(TEXT("MasterVolumePercent")));
	TestNotNull(TEXT("Master volume uses a real slider"), MasterSlider);
	TestNotNull(TEXT("Delivered settings widget uses the compact effects layout"), CombatOverlay);
	const TConstArrayView<EReEchoAudioBus> EffectsBuses = UReEchoSettingsWidget::GetCompactEffectsBuses();
	TestTrue(TEXT("Compact effects slider controls ambience loops"), EffectsBuses.Contains(EReEchoAudioBus::Ambience));
	TestTrue(TEXT("Compact effects slider controls combat SFX"), EffectsBuses.Contains(EReEchoAudioBus::CombatSfx));
	TestTrue(TEXT("Compact effects slider controls UI SFX"), EffectsBuses.Contains(EReEchoAudioBus::UiSfx));
	TestNotNull(TEXT("Master volume slider overlay exists"), MasterOverlay);
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
	TestNotNull(TEXT("Master volume percentage text exists"), MasterPercent);
	if (MasterSlider)
	{
		MasterSlider->OnValueChanged.Broadcast(0.42f);
	}
	TestEqual(TEXT("Dragging the slider updates its delivered percentage label"),
	          MasterPercent ? MasterPercent->GetText().ToString() : FString(),
	          FString(TEXT("42%")));
	return true;
}

#endif
