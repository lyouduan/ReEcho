#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
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

	Widget->CharacterOptionIds = {FName(TEXT("J_HEART"))};
	Widget->HandleCharacterHovered(0);
	TestEqual(TEXT("Mouse hover previews the matching character"), Widget->SelectedCharacterId, FName(TEXT("J_HEART")));
	TestFalse(TEXT("Mouse hover delegates description placement to the native tooltip"),
	          Widget->bShowAnchoredDescription);
	Widget->HandleCharacterPreviewed(0);
	TestTrue(TEXT("Keyboard focus retains an anchored description fallback"), Widget->bShowAnchoredDescription);
	Widget->HandleCharacterClicked(0);
	TestFalse(TEXT("Mouse click does not duplicate the native tooltip with an anchored panel"),
	          Widget->bShowAnchoredDescription);

	Widget->SelectedWeaponId = TEXT("W_J_09");
	Widget->HandleConfirmClicked();
	TestTrue(TEXT("First confirmation advances to the weapon stage"),
	         Widget->SelectionStage == UReEchoLoadoutSelectionWidget::ESelectionStage::Weapon);
	TestEqual(
	    TEXT("Character selection survives the stage change"), Widget->SelectedCharacterId, FName(TEXT("J_HEART")));
	TestTrue(TEXT("Weapon preview is reset when weapon selection begins"), Widget->SelectedWeaponId.IsNone());
	TestFalse(TEXT("First confirmation does not submit a final loadout"), Widget->bFinalConfirmationBroadcast);

	Widget->SelectedWeaponId = TEXT("W_J_04");
	Widget->HandleConfirmClicked();
	TestTrue(TEXT("Second confirmation submits the final loadout exactly once"), Widget->bFinalConfirmationBroadcast);
	TestEqual(
	    TEXT("Final confirmation retains the selected character"), Widget->SelectedCharacterId, FName(TEXT("J_HEART")));
	TestEqual(TEXT("Final confirmation retains the selected weapon"), Widget->SelectedWeaponId, FName(TEXT("W_J_04")));

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
	TestNotNull(TEXT("Authored loadout entry class is loadable"),
	            LoadClass<UReEchoLoadoutEntryWidget>(
	                nullptr, TEXT("/Game/ReEcho/UI/WBP_ReEchoLoadoutEntry.WBP_ReEchoLoadoutEntry_C")));
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

	if (SelectionClass)
	{
		UReEchoLoadoutSelectionWidget* AuthoredWidget =
		    NewObject<UReEchoLoadoutSelectionWidget>(GetTransientPackage(), SelectionClass);
		TestTrue(TEXT("Authored selection widget initializes its Designer tree"), AuthoredWidget->Initialize());
		if (AuthoredWidget->CharacterRow && AuthoredWidget->WeaponRow)
		{
			TestEqual(
			    TEXT("Designer tree owns four character entries"), AuthoredWidget->CharacterRow->GetChildrenCount(), 4);
			TestEqual(TEXT("Designer tree owns four weapon entries"), AuthoredWidget->WeaponRow->GetChildrenCount(), 4);
			UWidget* FirstAuthoredCharacter = AuthoredWidget->CharacterRow->GetChildAt(0);
			UWidget* FirstAuthoredWeapon = AuthoredWidget->WeaponRow->GetChildAt(0);
			AuthoredWidget->LoadOptions();
			AuthoredWidget->BuildOptionEntries();
			TestTrue(TEXT("Runtime reuses the first Designer-authored character entry"),
			         AuthoredWidget->CharacterEntries.IsValidIndex(0) &&
			             AuthoredWidget->CharacterEntries[0].Get() == FirstAuthoredCharacter);
			TestTrue(TEXT("Runtime reuses the first Designer-authored weapon entry"),
			         AuthoredWidget->WeaponEntries.IsValidIndex(0) &&
			             AuthoredWidget->WeaponEntries[0].Get() == FirstAuthoredWeapon);
			TestNotNull(TEXT("Designer tree owns the first character selection arrow"),
			            AuthoredWidget->CharacterSelectionArrow0.Get());
			TestNotNull(TEXT("Designer tree owns the last weapon selection arrow"),
			            AuthoredWidget->WeaponSelectionArrow3.Get());
			AuthoredWidget->SelectionStage = UReEchoLoadoutSelectionWidget::ESelectionStage::Character;
			AuthoredWidget->SelectedCharacterId = TEXT("J_HEART");
			AuthoredWidget->RefreshSelectionArrow();
			TestEqual(TEXT("Character selection displays its Designer-owned arrow"),
			          AuthoredWidget->CharacterSelectionArrow0->GetVisibility(),
			          ESlateVisibility::HitTestInvisible);
			TestEqual(TEXT("Other Designer arrows remain collapsed"),
			          AuthoredWidget->SelectionArrow->GetVisibility(),
			          ESlateVisibility::Collapsed);
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
