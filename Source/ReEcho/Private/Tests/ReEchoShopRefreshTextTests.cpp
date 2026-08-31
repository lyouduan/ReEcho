#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/Button.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "UI/ReEchoInventoryShopWidget.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoShopAuthoredRefreshTextTest,
                                 "ReEcho.UI.Shop.AuthoredRefreshText",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoShopAuthoredRefreshTextTest::RunTest(const FString& Parameters)
{
	UClass* ShopClass = LoadClass<UReEchoInventoryShopWidget>(
	    nullptr, TEXT("/Game/ReEcho/UI/WBP_ReEchoInventoryShopScreen.WBP_ReEchoInventoryShopScreen_C"));
	if (!TestNotNull(TEXT("Authored shop class loads"), ShopClass))
	{
		return false;
	}

	for (const bool bCustomize : {false, true})
	{
		UReEchoInventoryShopWidget* Widget = NewObject<UReEchoInventoryShopWidget>(GetTransientPackage(), ShopClass);
		if (!TestTrue(TEXT("Shop initializes before Slate construction"), Widget->Initialize()))
		{
			return false;
		}
		UTextBlock* Label = Cast<UTextBlock>(Widget->GetWidgetFromName(TEXT("ShopRefreshCountText")));
		UButton* Button = Cast<UButton>(Widget->GetWidgetFromName(TEXT("ShopRefreshButton")));
		UImage* Art = Cast<UImage>(Widget->GetWidgetFromName(TEXT("DesignerRefreshArt")));
		UWidget* Box = Widget->GetWidgetFromName(TEXT("DesignerRefreshTextBox"));
		if (!TestNotNull(TEXT("Label exists before runtime and is editable in Designer"), Label) ||
		    !TestNotNull(TEXT("Authored refresh button exists"), Button) ||
		    !TestNotNull(TEXT("Authored refresh artwork exists"), Art) ||
		    !TestNotNull(TEXT("Draggable text frame exists"), Box))
		{
			return false;
		}
		TestFalse(TEXT("Designer has sample refresh text"), Label->GetText().IsEmpty());
		UCanvasPanelSlot* BoxSlot = Cast<UCanvasPanelSlot>(Box->Slot);
		UOverlaySlot* LabelSlot = Cast<UOverlaySlot>(Label->Slot);
		if (!TestNotNull(TEXT("Text frame has editable Canvas geometry"), BoxSlot) ||
		    !TestNotNull(TEXT("Text exposes vertical alignment inside its frame"), LabelSlot))
		{
			return false;
		}
		UWidget* Content = Button->GetContent();
		UPanelWidget* LabelParent = Label->GetParent();
		if (bCustomize)
		{
			FSlateFontInfo Font = Label->GetFont();
			Font.Size = 23;
			Label->SetFont(Font);
			Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.2f, 0.5f, 0.7f, 0.8f)));
			Label->SetAutoWrapText(true);
			Label->SetWrapTextAt(137.0f);
			Label->SetJustification(ETextJustify::Right);
			Label->SetClipping(EWidgetClipping::Inherit);
			Label->SetRenderTranslation(FVector2D(3.0f, -2.0f));
			BoxSlot->SetAnchors(FAnchors(0.5f));
			BoxSlot->SetPosition(FVector2D(11.0f, 7.0f));
			BoxSlot->SetSize(FVector2D(143.0f, 37.0f));
			BoxSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			LabelSlot->SetVerticalAlignment(VAlign_Bottom);
			LabelSlot->SetHorizontalAlignment(HAlign_Right);
			LabelSlot->SetPadding(FMargin(2.0f, 3.0f));
		}

		struct FAuthoredProperty
		{
			UObject* Owner;
			FProperty* Property;
			FString Value;
		};

		TArray<FAuthoredProperty> Saved;
		const auto Capture = [this, &Saved](UObject* Owner, const TCHAR* Name)
		{
			FProperty* Property = FindFProperty<FProperty>(Owner->GetClass(), Name);
			if (TestNotNull(Name, Property))
			{
				FString Value;
				Property->ExportText_InContainer(0, Value, Owner, nullptr, Owner, PPF_None);
				Saved.Add({Owner, Property, Value});
			}
		};
		for (const TCHAR* Name : {TEXT("Font"),
		                          TEXT("ColorAndOpacity"),
		                          TEXT("Justification"),
		                          TEXT("AutoWrapText"),
		                          TEXT("WrapTextAt"),
		                          TEXT("Clipping"),
		                          TEXT("RenderTransform"),
		                          TEXT("RenderTransformPivot"),
		                          TEXT("Visibility")})
		{
			Capture(Label, Name);
		}
		Capture(BoxSlot, TEXT("LayoutData"));
		Capture(BoxSlot, TEXT("bAutoSize"));
		Capture(BoxSlot, TEXT("ZOrder"));
		Capture(LabelSlot, TEXT("HorizontalAlignment"));
		Capture(LabelSlot, TEXT("VerticalAlignment"));
		Capture(LabelSlot, TEXT("Padding"));
		Capture(Button, TEXT("WidgetStyle"));
		Capture(Button->Slot, TEXT("LayoutData"));
		Capture(Art, TEXT("Brush"));
		const auto Verify = [this, &Saved, Widget, Button, Content, Label, LabelParent](const TCHAR* Phase)
		{
			TestTrue(TEXT("Runtime reuses the exact authored text widget"),
			         Widget->GetWidgetFromName(TEXT("ShopRefreshCountText")) == Label);
			TestTrue(TEXT("Runtime preserves button content and text parent"),
			         Button->GetContent() == Content && Label->GetParent() == LabelParent);
			for (const FAuthoredProperty& Entry : Saved)
			{
				FString Actual;
				Entry.Property->ExportText_InContainer(0, Actual, Entry.Owner, nullptr, Entry.Owner, PPF_None);
				TestEqual(
				    *FString::Printf(
				        TEXT("%s: %s.%s stays authored"), Phase, *Entry.Owner->GetName(), *Entry.Property->GetName()),
				    Actual,
				    Entry.Value);
			}
		};

		FReEchoWeaponPartShopView View;
		View.WeaponRuneRefreshesRemaining = 2;
		View.WeaponRuneRefreshCost = 5;
		View.bWeaponRuneRefreshAllowed = true;
		Widget->SetWeaponPartShopView(View);
		Widget->ShowShop(100, {});
		Verify(TEXT("DataBeforeConstruct"));
		Widget->TakeWidget();
		Verify(TEXT("Construct"));
		TestEqual(TEXT("Paid refresh label still uses live view data"),
		          Label->GetText().ToString(),
		          FString(TEXT("刷新|2次·5碎片")));
		Widget->ShowShop(100, {}, 0.0f, 3);
		TestEqual(TEXT("Free allowance remains visible"),
		          Label->GetText().ToString(),
		          FString(TEXT("刷新|3次免费|2次·5碎片")));
		Widget->ShowShop(100, {}, 0.0f, 0, true, true, 0, true);
		TestEqual(TEXT("Unlimited free refresh remains visible"),
		          Label->GetText().ToString(),
		          FString(TEXT("刷新|∞次免费|2次·5碎片")));
		View.bWeaponRuneRefreshUnlimited = true;
		Widget->SetWeaponPartShopView(View);
		Widget->ShowShop(100, {});
		TestEqual(TEXT("Unlimited paid refresh remains visible"),
		          Label->GetText().ToString(),
		          FString(TEXT("刷新|∞次·5碎片")));
		Widget->ShowShop(100, {}, 0.0f, 0, false);
		TestFalse(TEXT("Run refresh prohibition still disables the button"), Button->GetIsEnabled());
		Widget->ShowShop(100, {});
		View.bWeaponRuneRefreshAllowed = false;
		Widget->SetWeaponPartShopView(View);
		TestFalse(TEXT("View refresh prohibition still disables the button"), Button->GetIsEnabled());
		View.bWeaponRuneRefreshAllowed = true;
		View.bWeaponRuneRefreshUnlimited = false;
		View.WeaponRuneRefreshesRemaining = 1;
		View.WeaponRuneRefreshCost = 7;
		Widget->SetWeaponPartShopView(View);
		TestEqual(TEXT("Updated budget/cost are projected without rebuilding text"),
		          Label->GetText().ToString(),
		          FString(TEXT("刷新|1次·7碎片")));
		Verify(TEXT("Refresh"));
		Widget->ReleaseSlateResources(true);
		Widget->TakeWidget();
		Verify(TEXT("Reconstruct"));
		int32 RefreshRequests = 0;
		Widget->OnRefreshRequested.AddLambda(
		    [&RefreshRequests]()
		    {
			    ++RefreshRequests;
		    });
		Button->OnClicked.Broadcast();
		TestEqual(TEXT("Reconstruct binds exactly one existing refresh command"), RefreshRequests, 1);
		Widget->OnRefreshRequested.Clear();
	}
	return true;
}

#endif
