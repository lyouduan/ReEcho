#include "Misc/AutomationTest.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Core/ReEchoBalanceSettings.h"
#include "UI/ReEchoRestartWidget.h"
#include "UObject/UnrealType.h"

namespace
{
template <typename WidgetType> WidgetType* FindRestartWidget(UReEchoRestartWidget* RestartWidget, const FName Name)
{
	return Cast<WidgetType>(RestartWidget->GetWidgetFromName(Name));
}

FReEchoShopOffer MakeOwnedCardOffer(const FName CardId, const int32 Tier, const bool bUseExistingIcon = true)
{
	FReEchoShopOffer Offer;
	Offer.ContentId = CardId;
	Offer.Tier = Tier;
	const FString AssetName = FString::Printf(TEXT("T_UI_CardIcon_%s"), *CardId.ToString());
	Offer.IconTexturePath =
	    bUseExistingIcon ? FString::Printf(TEXT("/Game/ReEcho/Textures/UI/Cards/Icon/%s.%s"), *AssetName, *AssetName)
	                     : TEXT("/Game/ReEcho/Textures/UI/Cards/Icon/T_UI_CardIcon_Missing.T_UI_CardIcon_Missing");
	return Offer;
}
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoRestartWidgetAuthoredStatsTest,
                                 "ReEcho.UI.RestartWidgetAuthoredStats",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoRestartWidgetAuthoredStatsTest::RunTest(const FString& Parameters)
{
	UClass* RestartClass =
	    LoadClass<UReEchoRestartWidget>(nullptr, TEXT("/Game/ReEcho/UI/WBP_ReEchoRestart.WBP_ReEchoRestart_C"));
	if (!TestNotNull(TEXT("Authored restart widget class loads"), RestartClass))
	{
		return false;
	}

	// Check both the saved Designer defaults and arbitrary edits made before runtime construction.
	for (const bool bCustomize : {false, true})
	{
		UReEchoRestartWidget* Widget = NewObject<UReEchoRestartWidget>(GetTransientPackage(), RestartClass);
		if (!TestTrue(TEXT("Authored restart widget initializes"), Widget->Initialize()))
		{
			return false;
		}

		struct FAuthoredProperty
		{
			UObject* Owner;
			FProperty* Property;
			FString Expected;
		};

		TArray<FAuthoredProperty> Snapshots;
		const auto Capture = [this, &Snapshots](UObject* Owner, const FName PropertyName)
		{
			FProperty* Property = FindFProperty<FProperty>(Owner->GetClass(), PropertyName);
			if (TestNotNull(*FString::Printf(TEXT("Authored property %s exists"), *PropertyName.ToString()), Property))
			{
				FString Value;
				Property->ExportText_InContainer(0, Value, Owner, nullptr, Owner, PPF_None);
				Snapshots.Add({Owner, Property, Value});
			}
		};

		int32 TextCount = 0;
		for (const TCHAR* Prefix : {TEXT("Victory"), TEXT("Defeat")})
		{
			for (const TCHAR* Suffix : {TEXT("EncounterLabel"),
			                            TEXT("EncounterValue"),
			                            TEXT("TraitCountLabel"),
			                            TEXT("TraitCountValue"),
			                            TEXT("TimeShardsLabel"),
			                            TEXT("TimeShardsValue"),
			                            TEXT("EchoDamageValueLabel"),
			                            TEXT("EchoDamageValue"),
			                            TEXT("PlayerDamageValueLabel"),
			                            TEXT("PlayerDamageValue"),
			                            TEXT("ReactionCountValueLabel"),
			                            TEXT("ReactionCountValue"),
			                            TEXT("MaxHitValueLabel"),
			                            TEXT("MaxHitValue"),
			                            TEXT("KillCountValueLabel"),
			                            TEXT("KillCountValue")})
			{
				const FString Name = FString::Printf(TEXT("%s%s"), Prefix, Suffix);
				UTextBlock* Text = FindRestartWidget<UTextBlock>(Widget, *Name);
				if (!TestNotNull(*Name, Text))
				{
					return false;
				}
				++TextCount;
				UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Text->Slot);
				if (bCustomize)
				{
					FSlateFontInfo Font = Text->GetFont();
					Font.Size = 29 + TextCount;
					Text->SetFont(Font);
					Text->SetColorAndOpacity(FSlateColor(FLinearColor(0.23f, 0.51f, 0.71f, 0.82f)));
					Text->SetAutoWrapText(true);
					Text->SetWrapTextAt(210.0f + TextCount);
					Text->SetJustification(ETextJustify::Right);
					Text->SetClipping(EWidgetClipping::Inherit);
					Text->SetRenderTranslation(FVector2D(7.0f, -3.0f));
					if (Slot)
					{
						Slot->SetPosition(FVector2D(73.0f + TextCount * 11.0f, 127.0f + TextCount * 7.0f));
						Slot->SetSize(FVector2D(220.0f + TextCount, 61.0f));
						Slot->SetAlignment(FVector2D(0.25f, 0.75f));
					}
				}

				for (const TCHAR* PropertyName : {TEXT("Font"),
				                                  TEXT("ColorAndOpacity"),
				                                  TEXT("Justification"),
				                                  TEXT("AutoWrapText"),
				                                  TEXT("WrapTextAt"),
				                                  TEXT("Clipping"),
				                                  TEXT("RenderTransform"),
				                                  TEXT("RenderTransformPivot")})
				{
					Capture(Text, PropertyName);
				}
				if (Name.EndsWith(TEXT("Label")))
				{
					Capture(Text, TEXT("Text"));
				}
				if (Slot)
				{
					Capture(Slot, TEXT("LayoutData"));
					Capture(Slot, TEXT("bAutoSize"));
					Capture(Slot, TEXT("ZOrder"));
				}
			}
		}
		TestEqual(TEXT("Both surfaces expose eight label/value pairs"), TextCount, 32);

		const auto Verify = [this, &Snapshots, bCustomize](const TCHAR* Phase)
		{
			for (const FAuthoredProperty& Snapshot : Snapshots)
			{
				FString Actual;
				Snapshot.Property->ExportText_InContainer(0, Actual, Snapshot.Owner, nullptr, Snapshot.Owner, PPF_None);
				TestEqual(*FString::Printf(TEXT("%s %s: %s.%s remains Designer-owned"),
				                           bCustomize ? TEXT("Edited") : TEXT("Saved"),
				                           Phase,
				                           *Snapshot.Owner->GetName(),
				                           *Snapshot.Property->GetName()),
				          Actual,
				          Snapshot.Expected);
			}
		};
		Widget->TakeWidget();
		Verify(TEXT("Construct"));
		Widget->SetDeathScreen(true, 4, 12345, 17);
		Verify(TEXT("Defeat"));
		TestEqual(TEXT("Defeat values still update"),
		          FindRestartWidget<UTextBlock>(Widget, TEXT("DefeatTimeShardsValue"))->GetText().ToString(),
		          FText::AsNumber(12345).ToString());
		Widget->SetVictoryScreen(9876, 23);
		Verify(TEXT("Victory"));
		TestEqual(TEXT("Victory values still update"),
		          FindRestartWidget<UTextBlock>(Widget, TEXT("VictoryTraitCountValue"))->GetText().ToString(),
		          FText::AsNumber(23).ToString());
		Widget->ReleaseSlateResources(true);
		Widget->TakeWidget();
		Verify(TEXT("Reconstruct"));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoRestartWidgetPresentationTest,
                                 "ReEcho.UI.RestartWidgetPresentation",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoRestartWidgetPresentationTest::RunTest(const FString& Parameters)
{
	UClass* RestartClass =
	    LoadClass<UReEchoRestartWidget>(nullptr, TEXT("/Game/ReEcho/UI/WBP_ReEchoRestart.WBP_ReEchoRestart_C"));
	TestNotNull(TEXT("Designed restart widget class loads"), RestartClass);
	if (!RestartClass)
	{
		return false;
	}

	UReEchoRestartWidget* RestartWidget = NewObject<UReEchoRestartWidget>(GetTransientPackage(), RestartClass);
	TestNotNull(TEXT("Designed restart widget can be instantiated"), RestartWidget);
	if (!RestartWidget)
	{
		return false;
	}
	TestTrue(TEXT("Designed restart widget initializes its widget tree"), RestartWidget->Initialize());
	RestartWidget->TakeWidget();
	TestEqual(TEXT("Runtime class is the authored restart WBP"),
	          RestartWidget->GetClass()->GetPathName(),
	          FString(TEXT("/Game/ReEcho/UI/WBP_ReEchoRestart.WBP_ReEchoRestart_C")));
	TestEqual(TEXT("Designed restart keeps its authored root"),
	          RestartWidget->WidgetTree->RootWidget->GetFName(),
	          FName(TEXT("CanvasPanel_0")));
	TestNull(TEXT("Legacy result character is removed"), RestartWidget->GetWidgetFromName(TEXT("ArtRestartCharacter")));
	TestNull(TEXT("Legacy result summary panel is removed"),
	         RestartWidget->GetWidgetFromName(TEXT("ArtResultSummaryPanel")));
	TestNull(TEXT("Legacy selected-cards panel is removed"),
	         RestartWidget->GetWidgetFromName(TEXT("ArtSelectedCardsPanel")));
	TestNull(TEXT("Legacy victory title is removed"), RestartWidget->GetWidgetFromName(TEXT("ArtVictoryTitle")));
	TestNull(TEXT("Legacy defeat title is removed"), RestartWidget->GetWidgetFromName(TEXT("ArtDefeatTitle")));

	UTextBlock* TitleText = FindRestartWidget<UTextBlock>(RestartWidget, TEXT("TitleText"));
	UVerticalBox* RootPanel = FindRestartWidget<UVerticalBox>(RestartWidget, TEXT("RootPanel"));
	UButton* ResumeButton = FindRestartWidget<UButton>(RestartWidget, TEXT("ResumeButton"));
	UButton* RestartButton = FindRestartWidget<UButton>(RestartWidget, TEXT("RestartButton"));
	UButton* QuitButton = FindRestartWidget<UButton>(RestartWidget, TEXT("QuitButton"));
	UButton* PauseSettingsButton = FindRestartWidget<UButton>(RestartWidget, TEXT("PauseSettingsButton"));
	UImage* ArtPauseDimmer = FindRestartWidget<UImage>(RestartWidget, TEXT("ArtPauseDimmer"));
	UImage* ArtPausePrimaryButton = FindRestartWidget<UImage>(RestartWidget, TEXT("ArtPausePrimaryButton"));
	UImage* ArtPauseSecondaryButton = FindRestartWidget<UImage>(RestartWidget, TEXT("ArtPauseSecondaryButton"));
	UImage* ArtPauseTertiaryButton = FindRestartWidget<UImage>(RestartWidget, TEXT("ArtPauseTertiaryButton"));
	UImage* ArtRestartDialogPanel = FindRestartWidget<UImage>(RestartWidget, TEXT("ArtRestartDialogPanel"));
	UCanvasPanel* VictoryCanvas = FindRestartWidget<UCanvasPanel>(RestartWidget, TEXT("VictoryCanvas"));
	UTextBlock* VictoryEncounterValue = FindRestartWidget<UTextBlock>(RestartWidget, TEXT("VictoryEncounterValue"));
	UTextBlock* VictoryTimeShardsValue = FindRestartWidget<UTextBlock>(RestartWidget, TEXT("VictoryTimeShardsValue"));
	UTextBlock* VictoryTraitCountValue = FindRestartWidget<UTextBlock>(RestartWidget, TEXT("VictoryTraitCountValue"));
	UButton* VictoryContinueButton = FindRestartWidget<UButton>(RestartWidget, TEXT("VictoryContinueButton"));
	UImage* ArtVictoryCharacterFormal =
	    FindRestartWidget<UImage>(RestartWidget, TEXT("ArtVictoryCharacterFormal"));
	UImage* ArtVictoryTimeShardFormal =
	    FindRestartWidget<UImage>(RestartWidget, TEXT("ArtVictoryTimeShardFormal"));
	UCanvasPanel* DefeatCanvas = FindRestartWidget<UCanvasPanel>(RestartWidget, TEXT("DefeatCanvas"));
	UTextBlock* DefeatEncounterValue = FindRestartWidget<UTextBlock>(RestartWidget, TEXT("DefeatEncounterValue"));
	UTextBlock* DefeatTimeShardsValue = FindRestartWidget<UTextBlock>(RestartWidget, TEXT("DefeatTimeShardsValue"));
	UTextBlock* DefeatTraitCountValue = FindRestartWidget<UTextBlock>(RestartWidget, TEXT("DefeatTraitCountValue"));
	UButton* DefeatRestartButton = FindRestartWidget<UButton>(RestartWidget, TEXT("DefeatRestartButton"));
	UButton* DefeatMainMenuButton = FindRestartWidget<UButton>(RestartWidget, TEXT("DefeatMainMenuButton"));
	UImage* ArtDefeatCharacterFormal = FindRestartWidget<UImage>(RestartWidget, TEXT("ArtDefeatCharacterFormal"));
	UImage* ArtDefeatTimeShardFormal =
	    FindRestartWidget<UImage>(RestartWidget, TEXT("ArtDefeatTimeShardFormal"));
	UImage* ArtVictoryContinueButtonFormal =
	    FindRestartWidget<UImage>(RestartWidget, TEXT("ArtVictoryContinueButtonFormal"));
	UImage* ArtDefeatRestartButtonFormal =
	    FindRestartWidget<UImage>(RestartWidget, TEXT("ArtDefeatRestartButtonFormal"));
	UImage* ArtDefeatMainMenuButtonFormal =
	    FindRestartWidget<UImage>(RestartWidget, TEXT("ArtDefeatMainMenuButtonFormal"));
	UTextBlock* VictoryContinueLabel = FindRestartWidget<UTextBlock>(RestartWidget, TEXT("VictoryContinueLabel"));
	UTextBlock* DefeatRestartLabel = FindRestartWidget<UTextBlock>(RestartWidget, TEXT("DefeatRestartLabel"));
	UTextBlock* DefeatMainMenuLabel = FindRestartWidget<UTextBlock>(RestartWidget, TEXT("DefeatMainMenuLabel"));
	TArray<UImage*> DefeatCardSlots;
	TArray<UImage*> DefeatCardIcons;
	for (int32 Index = 0; Index < 5; ++Index)
	{
		DefeatCardSlots.Add(
		    FindRestartWidget<UImage>(RestartWidget, *FString::Printf(TEXT("DesignerDefeatCardSlot%d"), Index)));
		DefeatCardIcons.Add(
		    FindRestartWidget<UImage>(RestartWidget, *FString::Printf(TEXT("DesignerDefeatCardIcon%d"), Index)));
	}

	TestNotNull(TEXT("Pause title exists"), TitleText);
	TestNotNull(TEXT("Pause root exists"), RootPanel);
	TestNotNull(TEXT("Resume button exists"), ResumeButton);
	TestNotNull(TEXT("Exit-to-menu button exists"), RestartButton);
	TestNotNull(TEXT("Exit-game button exists"), QuitButton);
	TestNotNull(TEXT("Pause settings button exists"), PauseSettingsButton);
	TestNotNull(TEXT("Pause dimmer exists"), ArtPauseDimmer);
	TestNotNull(TEXT("Pause primary button art exists"), ArtPausePrimaryButton);
	TestNotNull(TEXT("Pause secondary button art exists"), ArtPauseSecondaryButton);
	TestNotNull(TEXT("Pause tertiary button art exists"), ArtPauseTertiaryButton);
	TestNotNull(TEXT("Restart dialog panel exists"), ArtRestartDialogPanel);
	TestNotNull(TEXT("Formal victory canvas exists"), VictoryCanvas);
	TestNotNull(TEXT("Formal victory encounter value exists"), VictoryEncounterValue);
	TestNotNull(TEXT("Formal victory shard value exists"), VictoryTimeShardsValue);
	TestNotNull(TEXT("Formal victory build value exists"), VictoryTraitCountValue);
	TestNotNull(TEXT("Formal victory continue button exists"), VictoryContinueButton);
	TestNotNull(TEXT("Formal victory character image exists"), ArtVictoryCharacterFormal);
	TestNotNull(TEXT("Formal victory time-shard decoration exists"), ArtVictoryTimeShardFormal);
	TestNotNull(TEXT("Formal defeat canvas exists"), DefeatCanvas);
	TestNotNull(TEXT("Formal defeat encounter value exists"), DefeatEncounterValue);
	TestNotNull(TEXT("Formal defeat shard value exists"), DefeatTimeShardsValue);
	TestNotNull(TEXT("Formal defeat build value exists"), DefeatTraitCountValue);
	TestNotNull(TEXT("Formal defeat restart button exists"), DefeatRestartButton);
	TestNotNull(TEXT("Formal defeat main-menu button exists"), DefeatMainMenuButton);
	TestNotNull(TEXT("Formal defeat character image exists"), ArtDefeatCharacterFormal);
	TestNotNull(TEXT("Formal defeat time-shard decoration exists"), ArtDefeatTimeShardFormal);
	TestNotNull(TEXT("Formal victory continue art exists"), ArtVictoryContinueButtonFormal);
	TestNotNull(TEXT("Formal defeat restart art exists"), ArtDefeatRestartButtonFormal);
	TestNotNull(TEXT("Formal defeat main-menu art exists"), ArtDefeatMainMenuButtonFormal);
	TestNotNull(TEXT("Formal victory continue label exists"), VictoryContinueLabel);
	TestNotNull(TEXT("Formal defeat restart label exists"), DefeatRestartLabel);
	TestNotNull(TEXT("Formal defeat main-menu label exists"), DefeatMainMenuLabel);
	for (int32 Index = 0; Index < DefeatCardSlots.Num(); ++Index)
	{
		TestNotNull(*FString::Printf(TEXT("Formal defeat card slot %d exists"), Index), DefeatCardSlots[Index]);
		TestNotNull(*FString::Printf(TEXT("Formal defeat card icon %d exists"), Index), DefeatCardIcons[Index]);
	}
	if (!TitleText || !RootPanel || !ResumeButton || !RestartButton || !QuitButton || !PauseSettingsButton ||
	    !ArtPauseDimmer || !ArtPausePrimaryButton || !ArtPauseSecondaryButton || !ArtPauseTertiaryButton ||
	    !ArtRestartDialogPanel || !VictoryCanvas || !VictoryEncounterValue || !VictoryTimeShardsValue ||
	    !VictoryTraitCountValue || !VictoryContinueButton || !ArtVictoryCharacterFormal || !DefeatCanvas ||
	    !DefeatEncounterValue ||
	    !DefeatTimeShardsValue || !DefeatTraitCountValue || !DefeatRestartButton || !DefeatMainMenuButton ||
	    !ArtDefeatCharacterFormal || !ArtVictoryContinueButtonFormal || !ArtDefeatRestartButtonFormal ||
	    !ArtDefeatMainMenuButtonFormal ||
	    !VictoryContinueLabel || !DefeatRestartLabel || !DefeatMainMenuLabel || DefeatCardSlots.Contains(nullptr) ||
	    DefeatCardIcons.Contains(nullptr))
	{
		return false;
	}

	const FVector2D DefeatRestartRestingScale = ArtDefeatRestartButtonFormal->GetRenderTransform().Scale;
	const FVector2D DefeatRestartLabelRestingScale = DefeatRestartLabel->GetRenderTransform().Scale;
	DefeatRestartButton->OnHovered.Broadcast();
	TestTrue(TEXT("Formal defeat restart art scales on hover"),
	         ArtDefeatRestartButtonFormal->GetRenderTransform().Scale.X > DefeatRestartRestingScale.X);
	TestTrue(TEXT("Formal defeat restart label scales with its art"),
	         DefeatRestartLabel->GetRenderTransform().Scale.X > DefeatRestartLabelRestingScale.X);
	DefeatRestartButton->OnUnhovered.Broadcast();
	TestTrue(TEXT("Formal defeat restart art restores after hover"),
	         ArtDefeatRestartButtonFormal->GetRenderTransform().Scale.Equals(DefeatRestartRestingScale));

	const FVector2D DefeatMainMenuRestingScale = ArtDefeatMainMenuButtonFormal->GetRenderTransform().Scale;
	const FVector2D DefeatMainMenuLabelRestingScale = DefeatMainMenuLabel->GetRenderTransform().Scale;
	DefeatMainMenuButton->OnHovered.Broadcast();
	TestTrue(TEXT("Formal defeat main-menu art scales on hover"),
	         ArtDefeatMainMenuButtonFormal->GetRenderTransform().Scale.X > DefeatMainMenuRestingScale.X);
	TestTrue(TEXT("Formal defeat main-menu label scales with its art"),
	         DefeatMainMenuLabel->GetRenderTransform().Scale.X > DefeatMainMenuLabelRestingScale.X);
	DefeatMainMenuButton->OnUnhovered.Broadcast();
	TestTrue(TEXT("Formal defeat main-menu art restores after hover"),
	         ArtDefeatMainMenuButtonFormal->GetRenderTransform().Scale.Equals(DefeatMainMenuRestingScale));

	const FVector2D VictoryContinueRestingScale = ArtVictoryContinueButtonFormal->GetRenderTransform().Scale;
	const FVector2D VictoryContinueLabelRestingScale = VictoryContinueLabel->GetRenderTransform().Scale;
	VictoryContinueButton->OnHovered.Broadcast();
	TestTrue(TEXT("Formal victory continue art scales on hover"),
	         ArtVictoryContinueButtonFormal->GetRenderTransform().Scale.X > VictoryContinueRestingScale.X);
	TestTrue(TEXT("Formal victory continue label scales with its art"),
	         VictoryContinueLabel->GetRenderTransform().Scale.X > VictoryContinueLabelRestingScale.X);
	VictoryContinueButton->OnUnhovered.Broadcast();
	TestTrue(TEXT("Formal victory continue art restores after hover"),
	         ArtVictoryContinueButtonFormal->GetRenderTransform().Scale.Equals(VictoryContinueRestingScale));

	RestartWidget->SetDeathScreen(false);
	TestEqual(TEXT("Normal pause title"), TitleText->GetText().ToString(), FString(TEXT("游戏暂停")));
	TestEqual(TEXT("C++ preserves the authored pause composition position"),
	          RootPanel->GetRenderTransform().Translation,
	          FVector2D::ZeroVector);
	TestEqual(TEXT("Resume button is interactive"), ResumeButton->GetVisibility(), ESlateVisibility::Visible);
	TestEqual(TEXT("Exit-to-menu button is interactive"), RestartButton->GetVisibility(), ESlateVisibility::Visible);
	TestEqual(TEXT("Exit-game button is interactive"), QuitButton->GetVisibility(), ESlateVisibility::Visible);
	TestEqual(TEXT("Settings gear is interactive"), PauseSettingsButton->GetVisibility(), ESlateVisibility::Visible);
	TestEqual(TEXT("Pause dimmer does not intercept input"),
	          ArtPauseDimmer->GetVisibility(),
	          ESlateVisibility::HitTestInvisible);
	TestEqual(
	    TEXT("Primary art is visible"), ArtPausePrimaryButton->GetVisibility(), ESlateVisibility::HitTestInvisible);
	TestEqual(
	    TEXT("Secondary art is visible"), ArtPauseSecondaryButton->GetVisibility(), ESlateVisibility::HitTestInvisible);
	TestEqual(
	    TEXT("Tertiary art is visible"), ArtPauseTertiaryButton->GetVisibility(), ESlateVisibility::HitTestInvisible);
	TestTrue(TEXT("Primary art retains its source width"), ArtPausePrimaryButton->GetBrush().ImageSize.X >= 420.0f);
	TestTrue(TEXT("Primary art preserves the formal aspect height"),
	         ArtPausePrimaryButton->GetBrush().ImageSize.Y >= 140.0f);

	RestartWidget->SetQuitConfirmation(true, true, 4);
	TestEqual(TEXT("Exit-to-menu confirmation title"), TitleText->GetText().ToString(), FString(TEXT("退出到主菜单?")));
	TestEqual(TEXT("Exit-to-menu confirmation uses the current encounter as its save point"),
	          FindRestartWidget<UTextBlock>(RestartWidget, TEXT("MessageText"))->GetText().ToString(),
	          FString(TEXT("存档点：第 4 关")));
	TestEqual(TEXT("Exit-without-save button remains interactive"),
	          RestartButton->GetVisibility(),
	          ESlateVisibility::Visible);
	TestEqual(TEXT("Save-and-exit label"),
	          FindRestartWidget<UTextBlock>(RestartWidget, TEXT("ResumeButtonLabel"))->GetText().ToString(),
	          FString(TEXT("保存并退出")));
	TestEqual(TEXT("Exit-without-save label"),
	          FindRestartWidget<UTextBlock>(RestartWidget, TEXT("RestartButtonLabel"))->GetText().ToString(),
	          FString(TEXT("不保存并退出")));
	TestEqual(TEXT("Back label"),
	          FindRestartWidget<UTextBlock>(RestartWidget, TEXT("QuitButtonText"))->GetText().ToString(),
	          FString(TEXT("返回")));
	TestEqual(TEXT("Settings gear is hidden during confirmation"),
	          PauseSettingsButton->GetVisibility(),
	          ESlateVisibility::Collapsed);
	TestEqual(TEXT("Exit-to-menu confirmation has no dialog plate"),
	          ArtRestartDialogPanel->GetVisibility(),
	          ESlateVisibility::Hidden);

	RestartWidget->SetQuitConfirmation(true, false, 4);
	TestEqual(TEXT("Exit-game confirmation title"), TitleText->GetText().ToString(), FString(TEXT("退出游戏?")));
	TestEqual(TEXT("Exit-game confirmation has no dialog plate"),
	          ArtRestartDialogPanel->GetVisibility(),
	          ESlateVisibility::Hidden);

	RestartWidget->SetDeathScreen(true, 4, 126, 5);
	TestEqual(TEXT("Formal defeat canvas is visible"), DefeatCanvas->GetVisibility(), ESlateVisibility::Visible);
	TestEqual(TEXT("Formal defeat time-shard decoration is hidden"),
	          ArtDefeatTimeShardFormal->GetVisibility(),
	          ESlateVisibility::Collapsed);
	TestEqual(TEXT("Formal defeat encounter value is projected"),
	          DefeatEncounterValue->GetText().ToString(),
	          FString(TEXT("4")));
	TestEqual(TEXT("Formal defeat shard value is projected"),
	          DefeatTimeShardsValue->GetText().ToString(),
	          FString(TEXT("126")));
	TestEqual(TEXT("Formal defeat build value is projected"),
	          DefeatTraitCountValue->GetText().ToString(),
	          FString(TEXT("5")));
	TestEqual(TEXT("Formal defeat restart button is interactive"),
	          DefeatRestartButton->GetVisibility(),
	          ESlateVisibility::Visible);
	TestEqual(TEXT("Formal defeat main-menu button is interactive"),
	          DefeatMainMenuButton->GetVisibility(),
	          ESlateVisibility::Visible);
	TestEqual(TEXT("Legacy restart action is hidden during formal defeat"),
	          RestartButton->GetVisibility(),
	          ESlateVisibility::Collapsed);
	TestEqual(TEXT("Legacy quit action is hidden during formal defeat"),
	          QuitButton->GetVisibility(),
	          ESlateVisibility::Collapsed);

	const TArray<FReEchoShopOffer> OwnedCards = {
	    MakeOwnedCardOffer(TEXT("G_1_03"), 1),
	    MakeOwnedCardOffer(TEXT("G_3_22"), 3),
	    MakeOwnedCardOffer(TEXT("G_2_04"), 2),
	    MakeOwnedCardOffer(TEXT("G_3_17"), 3),
	    MakeOwnedCardOffer(TEXT("G_1_04"), 1),
	    MakeOwnedCardOffer(TEXT("G_2_05"), 2),
	};
	RestartWidget->SetDeathScreen(true, 4, 126, OwnedCards.Num(), TEXT("J_HEART"), OwnedCards);
	const FName ExpectedCardIds[] = {TEXT("G_3_22"), TEXT("G_3_17"), TEXT("G_2_04"), TEXT("G_2_05"), TEXT("G_1_03")};
	for (int32 Index = 0; Index < DefeatCardSlots.Num(); ++Index)
	{
		const FString AssetName = FString::Printf(TEXT("T_UI_CardIcon_%s"), *ExpectedCardIds[Index].ToString());
		const FString ExpectedPath =
		    FString::Printf(TEXT("/Game/ReEcho/Textures/UI/Cards/Icon/%s.%s"), *AssetName, *AssetName);
		TestEqual(*FString::Printf(TEXT("Ranked defeat card icon %d is visible"), Index),
		          DefeatCardIcons[Index]->GetVisibility(),
		          ESlateVisibility::HitTestInvisible);
		TestNotNull(*FString::Printf(TEXT("Ranked defeat card icon %d loads a texture"), Index),
		            DefeatCardIcons[Index]->GetBrush().GetResourceObject());
		if (DefeatCardIcons[Index]->GetBrush().GetResourceObject())
		{
			TestEqual(*FString::Printf(TEXT("Ranked defeat card icon %d uses the expected texture"), Index),
			          DefeatCardIcons[Index]->GetBrush().GetResourceObject()->GetPathName(),
			          ExpectedPath);
		}
	}

	const TArray<FReEchoShopOffer> MissingCard = {MakeOwnedCardOffer(TEXT("MISSING_CARD"), 3, false)};
	RestartWidget->SetDeathScreen(true, 4, 126, 1, TEXT("J_HEART"), MissingCard);
	TestNotNull(TEXT("Missing defeat card icon uses the shop fallback"),
	            DefeatCardIcons[0]->GetBrush().GetResourceObject());
	if (DefeatCardIcons[0]->GetBrush().GetResourceObject())
	{
		TestEqual(TEXT("Missing defeat card icon matches the shop fallback"),
		          DefeatCardIcons[0]->GetBrush().GetResourceObject()->GetPathName(),
		          FString(TEXT("/Game/ReEcho/Textures/UI/InteractionPlaceholder/InventoryShop/"
		                       "T_UI_Shop_CardIcon.T_UI_Shop_CardIcon")));
	}
	const FString EmptySlotPath =
	    TEXT("/Game/ReEcho/Textures/UI/Formal/RoundDefeat/T_UI_Defeat_CardSlot.T_UI_Defeat_CardSlot");
	for (int32 Index = 0; Index < DefeatCardSlots.Num(); ++Index)
	{
		TestNotNull(*FString::Printf(TEXT("Defeat card slot %d keeps its empty art"), Index),
		            DefeatCardSlots[Index]->GetBrush().GetResourceObject());
		if (DefeatCardSlots[Index]->GetBrush().GetResourceObject())
		{
			TestEqual(*FString::Printf(TEXT("Defeat card slot %d still matches empty art"), Index),
			          DefeatCardSlots[Index]->GetBrush().GetResourceObject()->GetPathName(),
			          EmptySlotPath);
		}
		if (Index > 0)
		{
			TestEqual(*FString::Printf(TEXT("Unused defeat card icon %d is hidden"), Index),
			          DefeatCardIcons[Index]->GetVisibility(),
			          ESlateVisibility::Hidden);
		}
	}

	RestartWidget->SetDeathScreen(true, 4, 126, 0);
	for (int32 Index = 0; Index < DefeatCardSlots.Num(); ++Index)
	{
		TestEqual(*FString::Printf(TEXT("Zero-card defeat icon %d is hidden"), Index),
		          DefeatCardIcons[Index]->GetVisibility(),
		          ESlateVisibility::Hidden);
	}

	RestartWidget->SetVictoryScreen(126, 5);
	TestEqual(TEXT("Formal defeat canvas is hidden during victory"),
	          DefeatCanvas->GetVisibility(),
	          ESlateVisibility::Collapsed);
	TestEqual(TEXT("Formal victory canvas is visible"), VictoryCanvas->GetVisibility(), ESlateVisibility::Visible);
	TestEqual(TEXT("Formal victory time-shard decoration is hidden"),
	          ArtVictoryTimeShardFormal->GetVisibility(),
	          ESlateVisibility::Collapsed);
	TestEqual(TEXT("Formal victory encounter is real balance data"),
	          VictoryEncounterValue->GetText().ToString(),
	          FText::AsNumber(GetDefault<UReEchoBalanceSettings>()->GetTotalEncounterCount()).ToString());
	TestEqual(TEXT("Formal victory shard value is projected"),
	          VictoryTimeShardsValue->GetText().ToString(),
	          FString(TEXT("126")));
	TestEqual(TEXT("Formal victory build value is projected"),
	          VictoryTraitCountValue->GetText().ToString(),
	          FString(TEXT("5")));
	TestEqual(TEXT("Formal victory continue button is interactive"),
	          VictoryContinueButton->GetVisibility(),
	          ESlateVisibility::Visible);
	TestEqual(TEXT("Legacy result action is hidden during formal victory"),
	          RestartButton->GetVisibility(),
	          ESlateVisibility::Collapsed);
	TestEqual(TEXT("Legacy exit action is hidden during formal victory"),
	          QuitButton->GetVisibility(),
	          ESlateVisibility::Collapsed);
	TestEqual(TEXT("Pause composition is hidden during formal victory"),
	          ArtPauseDimmer->GetVisibility(),
	          ESlateVisibility::Hidden);

	const TPair<FName, FString> CharacterCases[] = {
	    {TEXT("J_HEART"), TEXT("T_UI_Loadout_Character_J_HEART_Selected")},
	    {TEXT("J_SPADE"), TEXT("T_UI_Loadout_Character_J_SPADE_Selected")},
	    {TEXT("J_CLOVER"), TEXT("T_UI_Loadout_Character_J_CLOVER_Selected")},
	    {TEXT("J_DIAMOND"), TEXT("T_UI_Loadout_Character_J_DIAMOND_Selected")},
	    {NAME_None, TEXT("T_UI_Loadout_Character_J_HEART_Selected")},
	    {TEXT("UNKNOWN_CHARACTER"), TEXT("T_UI_Loadout_Character_J_HEART_Selected")},
	};
	for (const TPair<FName, FString>& CharacterCase : CharacterCases)
	{
		const FString ExpectedPath = FString::Printf(
		    TEXT("/Game/ReEcho/Textures/UI/LoadoutSelection/%s.%s"),
		    *CharacterCase.Value,
		    *CharacterCase.Value);
		RestartWidget->SetDeathScreen(true, 4, 126, 5, CharacterCase.Key);
		TestNotNull(*FString::Printf(TEXT("Defeat character texture loads for %s"), *CharacterCase.Key.ToString()),
		            ArtDefeatCharacterFormal->GetBrush().GetResourceObject());
		if (ArtDefeatCharacterFormal->GetBrush().GetResourceObject())
		{
			TestEqual(*FString::Printf(TEXT("Defeat character texture matches %s"), *CharacterCase.Key.ToString()),
			          ArtDefeatCharacterFormal->GetBrush().GetResourceObject()->GetPathName(),
			          ExpectedPath);
		}

		RestartWidget->SetVictoryScreen(126, 5, CharacterCase.Key);
		TestNotNull(*FString::Printf(TEXT("Victory character texture loads for %s"), *CharacterCase.Key.ToString()),
		            ArtVictoryCharacterFormal->GetBrush().GetResourceObject());
		if (ArtVictoryCharacterFormal->GetBrush().GetResourceObject())
		{
			TestEqual(*FString::Printf(TEXT("Victory character texture matches %s"), *CharacterCase.Key.ToString()),
			          ArtVictoryCharacterFormal->GetBrush().GetResourceObject()->GetPathName(),
			          ExpectedPath);
		}
	}

	return true;
}
