#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/Texture2D.h"
#include "UI/ReEchoLoadoutEntryWidget.h"
#include "UI/ReEchoLoadoutSelectionWidget.h"
#include "UI/ReEchoLoadoutTooltipWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoLoadoutSelectionFlowTest,
                                 "ReEcho.UI.LoadoutSelection.Flow",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoLoadoutSelectionFlowTest::RunTest(const FString& Parameters)
{
	UReEchoLoadoutSelectionWidget* Widget = NewObject<UReEchoLoadoutSelectionWidget>(GetTransientPackage());
	if (!TestNotNull(TEXT("Two-stage loadout widget can be constructed"), Widget))
	{
		return false;
	}

	Widget->CharacterOptionIds = {FName(TEXT("J_HEART")), FName(TEXT("J_SPADE"))};
	Widget->HandleCharacterHovered(0);
	TestTrue(TEXT("Mouse hover does not select a character before a click"), Widget->SelectedCharacterId.IsNone());
	TestFalse(TEXT("Mouse hover delegates description placement to the native tooltip"),
	          Widget->bShowAnchoredDescription);
	Widget->HandleCharacterClicked(0);
	TestEqual(TEXT("Mouse click selects the matching character"), Widget->SelectedCharacterId, FName(TEXT("J_HEART")));
	Widget->HandleCharacterHovered(1);
	TestEqual(TEXT("Hovering another character preserves the clicked character"),
	          Widget->SelectedCharacterId,
	          FName(TEXT("J_HEART")));
	Widget->HandleCharacterPreviewed(1);
	TestEqual(TEXT("Keyboard focus still previews the focused character"),
	          Widget->SelectedCharacterId,
	          FName(TEXT("J_SPADE")));
	TestTrue(TEXT("Keyboard focus retains an anchored description fallback"), Widget->bShowAnchoredDescription);
	Widget->HandleCharacterClicked(0);
	TestEqual(TEXT("A later character click replaces the focused candidate"),
	          Widget->SelectedCharacterId,
	          FName(TEXT("J_HEART")));
	TestFalse(TEXT("Mouse click does not duplicate the native tooltip with an anchored panel"),
	          Widget->bShowAnchoredDescription);

	Widget->HandleConfirmClicked();
	TestTrue(TEXT("First confirmation advances to the weapon stage"),
	         Widget->SelectionStage == UReEchoLoadoutSelectionWidget::ESelectionStage::Weapon);
	TestEqual(
	    TEXT("Character selection survives the stage change"), Widget->SelectedCharacterId, FName(TEXT("J_HEART")));
	TestTrue(TEXT("Weapon preview is reset when weapon selection begins"), Widget->SelectedWeaponId.IsNone());
	TestFalse(TEXT("First confirmation does not submit a final loadout"), Widget->bFinalConfirmationBroadcast);

	Widget->WeaponOptionIds = {FName(TEXT("W_J_04")), FName(TEXT("W_J_09"))};
	Widget->HandleWeaponHovered(1);
	TestTrue(TEXT("Mouse hover does not select a weapon before a click"), Widget->SelectedWeaponId.IsNone());
	Widget->HandleWeaponClicked(0);
	TestEqual(TEXT("Mouse click selects the matching weapon"), Widget->SelectedWeaponId, FName(TEXT("W_J_04")));
	Widget->HandleWeaponHovered(1);
	TestEqual(TEXT("Hovering another weapon preserves the clicked weapon"),
	          Widget->SelectedWeaponId,
	          FName(TEXT("W_J_04")));
	Widget->HandleWeaponClicked(1);
	TestEqual(TEXT("A later weapon click replaces the selected candidate"),
	          Widget->SelectedWeaponId,
	          FName(TEXT("W_J_09")));
	Widget->HandleConfirmClicked();
	TestTrue(TEXT("Second confirmation submits the final loadout exactly once"), Widget->bFinalConfirmationBroadcast);
	TestEqual(
	    TEXT("Final confirmation retains the selected character"), Widget->SelectedCharacterId, FName(TEXT("J_HEART")));
	TestEqual(TEXT("Final confirmation retains the selected weapon"), Widget->SelectedWeaponId, FName(TEXT("W_J_09")));

	Widget->HandleConfirmClicked();
	TestTrue(TEXT("Repeated confirmation remains guarded after submission"), Widget->bFinalConfirmationBroadcast);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoLoadoutSelectionAssetContractTest,
                                 "ReEcho.UI.LoadoutSelection.Assets",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoLoadoutSelectionAssetContractTest::RunTest(const FString& Parameters)
{
	UClass* SelectionClass = LoadClass<UReEchoLoadoutSelectionWidget>(
	    nullptr, TEXT("/Game/ReEcho/UI/WBP_ReEchoLoadoutSelection.WBP_ReEchoLoadoutSelection_C"));
	TestNotNull(TEXT("Authored loadout selection class is loadable"), SelectionClass);
	UClass* EntryClass = LoadClass<UReEchoLoadoutEntryWidget>(
	    nullptr, TEXT("/Game/ReEcho/UI/WBP_ReEchoLoadoutEntry.WBP_ReEchoLoadoutEntry_C"));
	TestNotNull(TEXT("Authored loadout entry class is loadable"), EntryClass);
	UClass* TooltipClass = LoadClass<UReEchoLoadoutTooltipWidget>(
	    nullptr, TEXT("/Game/ReEcho/UI/WBP_ReEchoLoadoutTooltip.WBP_ReEchoLoadoutTooltip_C"));
	TestNotNull(TEXT("Authored loadout tooltip class is loadable"), TooltipClass);
	if (TooltipClass)
	{
		UReEchoLoadoutTooltipWidget* Tooltip =
		    NewObject<UReEchoLoadoutTooltipWidget>(GetTransientPackage(), TooltipClass);
		TestTrue(TEXT("Authored tooltip initializes its Designer tree"), Tooltip->Initialize());
		UBorder* TooltipFrame = Cast<UBorder>(Tooltip->GetWidgetFromName(TEXT("TooltipFrame")));
		TestNotNull(TEXT("Authored tooltip exposes its delivery frame"), TooltipFrame);
		if (TooltipFrame)
		{
			PRAGMA_DISABLE_DEPRECATION_WARNINGS
			TestTrue(TEXT("Tooltip frame keeps the delivery texture at runtime"),
			         TooltipFrame->Background.GetResourceObject() != nullptr);
			TestEqual(TEXT("Tooltip frame uses scalable nine-slice drawing"),
			          TooltipFrame->Background.DrawAs,
			          ESlateBrushDrawType::Box);
			PRAGMA_ENABLE_DEPRECATION_WARNINGS
		}
	}
	if (EntryClass)
	{
		UReEchoLoadoutEntryWidget* StandaloneEntry =
		    NewObject<UReEchoLoadoutEntryWidget>(GetTransientPackage(), EntryClass);
		TestTrue(TEXT("Standalone Entry initializes its Designer tree"), StandaloneEntry->Initialize());
		TestTrue(TEXT("Standalone Entry uses a content-sized Designer preview"),
		         StandaloneEntry->HasDesiredDesignerPreview());
		UImage* StandaloneArrow =
		    Cast<UImage>(StandaloneEntry->GetWidgetFromName(TEXT("EntrySelectionArrow")));
		TestNotNull(TEXT("Standalone Entry exposes the selection arrow in Designer"), StandaloneArrow);
		if (StandaloneArrow)
		{
			TestEqual(TEXT("Standalone Entry authors the arrow visible for WYSIWYG editing"),
			          StandaloneArrow->GetVisibility(),
			          ESlateVisibility::HitTestInvisible);
		}
	}

	if (SelectionClass)
	{
		UReEchoLoadoutSelectionWidget* AuthoredWidget =
		    NewObject<UReEchoLoadoutSelectionWidget>(GetTransientPackage(), SelectionClass);
		TestTrue(TEXT("Authored selection widget initializes its Designer tree"), AuthoredWidget->Initialize());
		TestNotNull(TEXT("Designer tree owns the stage switcher"), AuthoredWidget->StageSwitcher.Get());
		if (AuthoredWidget->StageSwitcher)
		{
			TestEqual(TEXT("Stage switcher owns both authored pages"),
			          AuthoredWidget->StageSwitcher->GetNumWidgets(),
			          2);
			TestTrue(TEXT("Character page remains the first Designer page"),
			         AuthoredWidget->StageSwitcher->GetWidgetAtIndex(0) == AuthoredWidget->CharacterStagePanel);
			TestTrue(TEXT("Weapon page remains the second Designer page"),
			         AuthoredWidget->StageSwitcher->GetWidgetAtIndex(1) == AuthoredWidget->WeaponStagePanel);
			AuthoredWidget->SetSelectionStage(UReEchoLoadoutSelectionWidget::ESelectionStage::Weapon);
			TestEqual(TEXT("Runtime selects the authored weapon page"),
			          AuthoredWidget->StageSwitcher->GetActiveWidgetIndex(),
			          1);
		}
		if (AuthoredWidget->CharacterRow && AuthoredWidget->WeaponRow)
		{
			TestEqual(
			    TEXT("Designer tree owns four character entries"), AuthoredWidget->CharacterRow->GetChildrenCount(), 4);
			TestEqual(TEXT("Designer tree owns four weapon entries"), AuthoredWidget->WeaponRow->GetChildrenCount(), 4);
			UWidget* FirstAuthoredCharacter = AuthoredWidget->CharacterRow->GetChildAt(0);
			UWidget* FirstAuthoredWeapon = AuthoredWidget->WeaponRow->GetChildAt(0);
			UReEchoLoadoutEntryWidget* FirstCharacterEntry =
			    Cast<UReEchoLoadoutEntryWidget>(FirstAuthoredCharacter);
			UReEchoLoadoutEntryWidget* FirstWeaponEntry = Cast<UReEchoLoadoutEntryWidget>(FirstAuthoredWeapon);
			USizeBox* CharacterRootSize = FirstCharacterEntry
			                                  ? Cast<USizeBox>(FirstCharacterEntry->GetWidgetFromName(
			                                        TEXT("EntryRootSizeBox")))
			                                  : nullptr;
			USizeBox* CharacterPortraitSize = FirstCharacterEntry
			                                      ? Cast<USizeBox>(FirstCharacterEntry->GetWidgetFromName(
			                                            TEXT("PortraitSize")))
			                                      : nullptr;
			const float AuthoredRootHeight = CharacterRootSize ? CharacterRootSize->GetHeightOverride() : 0.0f;
			const float AuthoredPortraitHeight =
			    CharacterPortraitSize ? CharacterPortraitSize->GetHeightOverride() : 0.0f;
			AuthoredWidget->LoadOptions();
			AuthoredWidget->BuildOptionEntries();
			TestTrue(TEXT("Runtime reuses the first Designer-authored character entry"),
			         AuthoredWidget->CharacterEntries.IsValidIndex(0) &&
			             AuthoredWidget->CharacterEntries[0].Get() == FirstAuthoredCharacter);
			TestTrue(TEXT("Runtime reuses the first Designer-authored weapon entry"),
			         AuthoredWidget->WeaponEntries.IsValidIndex(0) &&
			             AuthoredWidget->WeaponEntries[0].Get() == FirstAuthoredWeapon);
			if (CharacterRootSize)
			{
				TestEqual(TEXT("Runtime configuration preserves the Blueprint-authored Entry height"),
				          CharacterRootSize->GetHeightOverride(),
				          AuthoredRootHeight);
			}
			if (CharacterPortraitSize)
			{
				TestEqual(TEXT("Runtime configuration preserves the Blueprint-authored portrait height"),
				          CharacterPortraitSize->GetHeightOverride(),
				          AuthoredPortraitHeight);
			}
			UImage* CharacterArrow = FirstCharacterEntry
			                             ? Cast<UImage>(FirstCharacterEntry->GetWidgetFromName(TEXT("EntrySelectionArrow")))
			                             : nullptr;
			UImage* WeaponArrow = FirstWeaponEntry
			                          ? Cast<UImage>(FirstWeaponEntry->GetWidgetFromName(TEXT("EntrySelectionArrow")))
			                          : nullptr;
			TestNotNull(TEXT("Each authored character entry owns its selection arrow"), CharacterArrow);
			TestNotNull(TEXT("Each authored weapon entry owns its selection arrow"), WeaponArrow);
			AuthoredWidget->SelectionStage = UReEchoLoadoutSelectionWidget::ESelectionStage::Character;
			AuthoredWidget->SelectedCharacterId = TEXT("J_HEART");
			AuthoredWidget->RefreshSelection();
			if (CharacterArrow)
			{
				TestEqual(TEXT("Selected entry displays the arrow inside its hover-scaled visual root"),
				          CharacterArrow->GetVisibility(),
				          ESlateVisibility::HitTestInvisible);
			}
			if (WeaponArrow)
			{
				TestEqual(TEXT("Inactive entry arrow remains collapsed"),
				          WeaponArrow->GetVisibility(),
				          ESlateVisibility::Collapsed);
			}
			if (AuthoredWidget->WeaponEntries.IsValidIndex(1))
			{
				UImage* GunPortrait = Cast<UImage>(
				    AuthoredWidget->WeaponEntries[1]->GetWidgetFromName(TEXT("PortraitImage")));
				TestNotNull(TEXT("Gun Entry exposes its portrait image"), GunPortrait);
				UScaleBoxSlot* GunPortraitSlot =
				    GunPortrait ? Cast<UScaleBoxSlot>(GunPortrait->Slot) : nullptr;
				TestNotNull(TEXT("Gun portrait remains a ScaleBox child"), GunPortraitSlot);
				if (GunPortraitSlot)
				{
					TestEqual(TEXT("Gun portrait is horizontally centered instead of stretched"),
					          GunPortraitSlot->GetHorizontalAlignment(),
					          HAlign_Center);
					TestEqual(TEXT("Gun portrait is vertically centered instead of stretched"),
					          GunPortraitSlot->GetVerticalAlignment(),
					          VAlign_Center);
				}
				if (GunPortrait)
				{
					PRAGMA_DISABLE_DEPRECATION_WARNINGS
					const FVector2D GunBrushSize = GunPortrait->GetBrush().ImageSize;
					PRAGMA_ENABLE_DEPRECATION_WARNINGS
					TestEqual(TEXT("Gun brush keeps the source width"), GunBrushSize.X, 249.0);
					TestEqual(TEXT("Gun brush keeps the source height"), GunBrushSize.Y, 227.0);
				}
			}
		}
		else
		{
			AddError(TEXT("Authored selection widget did not bind its preview rows"));
		}
	}

	const TCHAR* CharacterIds[] = {TEXT("J_HEART"), TEXT("J_SPADE"), TEXT("J_CLOVER"), TEXT("J_DIAMOND")};
	const TCHAR* WeaponIds[] = {TEXT("W_J_04"), TEXT("W_J_09"), TEXT("W_J_01"), TEXT("W_J_08")};
	const TCHAR* States[] = {TEXT("Selected"), TEXT("Unselected")};
	for (const TCHAR* CharacterId : CharacterIds)
	{
		for (const TCHAR* State : States)
		{
			const FString Name = FString::Printf(TEXT("T_UI_Loadout_Character_%s_%s"), CharacterId, State);
			const FString Path = FString::Printf(TEXT("/Game/ReEcho/Textures/UI/LoadoutSelection/%s.%s"), *Name, *Name);
			TestNotNull(*FString::Printf(TEXT("Character texture %s is loadable"), *Name),
			            LoadObject<UTexture2D>(nullptr, *Path));
		}
	}
	for (const TCHAR* WeaponId : WeaponIds)
	{
		for (const TCHAR* State : States)
		{
			const FString Name = FString::Printf(TEXT("T_UI_Loadout_Weapon_%s_%s"), WeaponId, State);
			const FString Path = FString::Printf(TEXT("/Game/ReEcho/Textures/UI/LoadoutSelection/%s.%s"), *Name, *Name);
			TestNotNull(*FString::Printf(TEXT("Weapon texture %s is loadable"), *Name),
			            LoadObject<UTexture2D>(nullptr, *Path));
		}
	}
	TestNotNull(TEXT("Description panel texture is loadable"),
	            LoadObject<UTexture2D>(nullptr,
	                                   TEXT("/Game/ReEcho/Textures/UI/LoadoutSelection/T_UI_Loadout_DescriptionPanel."
	                                        "T_UI_Loadout_DescriptionPanel")));
	TestNotNull(TEXT("Selection arrow texture is loadable"),
	            LoadObject<UTexture2D>(nullptr,
	                                   TEXT("/Game/ReEcho/Textures/UI/LoadoutSelection/T_UI_Loadout_SelectionArrow."
	                                        "T_UI_Loadout_SelectionArrow")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
