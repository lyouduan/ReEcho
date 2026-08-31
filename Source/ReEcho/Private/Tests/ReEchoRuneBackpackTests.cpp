#if WITH_DEV_AUTOMATION_TESTS

#include "UI/ReEchoRuneBackpackWidget.h"

#include "Components/Border.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "Misc/AutomationTest.h"
#include "UI/ReEchoIndexedButton.h"
#include "UI/ReEchoInventoryShopWidget.h"
#include "UI/ReEchoShopTooltipWidget.h"

namespace
{
UReEchoRuneBackpackWidget* MakeRuneBackpack(const bool bDesigner = false)
{
	UClass* Class = LoadClass<UReEchoRuneBackpackWidget>(
	    nullptr, TEXT("/Game/ReEcho/UI/WBP_ReEchoRuneBackpack.WBP_ReEchoRuneBackpack_C"));
	if (!Class)
	{
		return nullptr;
	}
	UReEchoRuneBackpackWidget* Widget = NewObject<UReEchoRuneBackpackWidget>(GetTransientPackage(), Class);
#if WITH_EDITOR
	if (bDesigner)
	{
		Widget->SetDesignerFlags(EWidgetDesignFlags::Designing | EWidgetDesignFlags::ExecutePreConstruct);
	}
#endif
	return Widget->Initialize() ? Widget : nullptr;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoRuneBackpackAuthoredTest,
                                 "ReEcho.UI.Shop.RuneBackpackAuthored",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoRuneBackpackAuthoredTest::RunTest(const FString&)
{
	UReEchoRuneBackpackWidget* Panel = MakeRuneBackpack();
	if (!TestNotNull(TEXT("Panel Blueprint loads"), Panel))
	{
		return false;
	}
	TSharedPtr<SWidget> Slate = Panel->TakeWidget();
	TArray<UReEchoRuneBackpackEntryWidget*> Entries = Panel->GetEntryWidgets();
	if (!TestTrue(TEXT("Panel contains three real authored rows"), Entries.Num() >= 3))
	{
		return false;
	}
	for (UReEchoRuneBackpackEntryWidget* Entry : Entries)
	{
		TestEqual(
		    TEXT("Samples are hidden until runtime data arrives"), Entry->GetVisibility(), ESlateVisibility::Collapsed);
		TestFalse(TEXT("Samples cannot equip"), Entry->GetSelectButton()->GetIsEnabled());
	}
	UReEchoRuneBackpackEntryWidget* First = Entries[0];
	USizeBox* Root = Cast<USizeBox>(Panel->GetWidgetFromName(TEXT("BackpackRootSizeBox")));
	USizeBox* EntryRoot = Cast<USizeBox>(First->GetWidgetFromName(TEXT("EntryRootSizeBox")));
	USizeBox* IconSize = Cast<USizeBox>(First->GetWidgetFromName(TEXT("RuneIconSize")));
	UTextBlock* Name = Cast<UTextBlock>(First->GetWidgetFromName(TEXT("NameText")));
	UTextBlock* Count = Cast<UTextBlock>(First->GetWidgetFromName(TEXT("CountText")));
	UBorder* Frame = Cast<UBorder>(Panel->GetWidgetFromName(TEXT("BackpackFrame")));
	if (!Root || !EntryRoot || !IconSize || !Name || !Count || !Frame)
	{
		AddError(TEXT("Required authored nodes missing"));
		return false;
	}
	Root->SetWidthOverride(420.0f);
	Root->SetHeightOverride(510.0f);
	EntryRoot->SetHeightOverride(124.0f);
	IconSize->SetWidthOverride(90.0f);
	IconSize->SetHeightOverride(50.0f);
	FSlateFontInfo Font = Name->GetFont();
	Font.Size = 28;
	Name->SetFont(Font);
	Name->SetColorAndOpacity(FLinearColor::Green);
	Frame->SetPadding(FMargin(7.0f));
	UVerticalBoxSlot* FirstSlot = CastChecked<UVerticalBoxSlot>(First->Slot);
	FirstSlot->SetPadding(FMargin(0.0f, 6.0f));
	const FButtonStyle ButtonStyle = First->GetSelectButton()->GetStyle();
	TArray<FReEchoRuneBackpackEntryView> Views;
	for (int32 Index = 0; Index < 8; ++Index)
	{
		FReEchoRuneBackpackEntryView View = First->DesignerPreview;
		View.Name = FText::FromString(FString::Printf(TEXT("真实符文 %d"), Index));
		View.Count = Index + 1;
		Views.Add(View);
	}
	Panel->ConfigureEntries(Views);
	Panel->ForceLayoutPrepass();
	Entries = Panel->GetEntryWidgets();
	TestEqual(TEXT("Eight real rows, not a fixed preview limit"), Entries.Num(), 8);
	TestTrue(TEXT("First authored row is reused"), First == Entries[0]);
	TestEqual(TEXT("Panel uses authored size"), Panel->GetDesiredSize(), FVector2D(420.0f, 510.0f));
	TestTrue(TEXT("Runtime name replaces sample"), Name->GetText().EqualTo(Views[0].Name));
	TestEqual(TEXT("Single copy count is hidden"), Count->GetVisibility(), ESlateVisibility::Collapsed);
	for (int32 Index = 3; Index < Entries.Num(); ++Index)
	{
		TestEqual(TEXT("Extra rows use same Blueprint class"), Entries[Index]->GetClass(), First->GetClass());
		TestEqual(TEXT("Extra rows inherit authored list spacing"),
		          CastChecked<UVerticalBoxSlot>(Entries[Index]->Slot)->GetPadding(),
		          FirstSlot->GetPadding());
	}
	Views.SetNum(1);
	Views[0].Count = 4;
	Panel->ConfigureEntries(Views);
	// A Slate rebuild must not replace saved presentation or recapture sample data.
	Slate.Reset();
	Panel->ReleaseSlateResources(true);
	Slate = Panel->TakeWidget();
	Panel->ForceLayoutPrepass();
	TestEqual(TEXT("Height survives rebuild"), Root->GetHeightOverride(), 510.0f);
	TestEqual(TEXT("Entry height survives refresh"), EntryRoot->GetHeightOverride(), 124.0f);
	TestEqual(TEXT("Icon authored width survives"), IconSize->GetWidthOverride(), 90.0f);
	TestEqual(TEXT("Icon authored height survives"), IconSize->GetHeightOverride(), 50.0f);
	TestEqual(TEXT("Font survives refresh"), Name->GetFont().Size, 28.0f);
	TestTrue(TEXT("Color survives refresh"), Name->GetColorAndOpacity() == FSlateColor(FLinearColor::Green));
	TestEqual(TEXT("Frame inset survives"), Frame->GetPadding(), FMargin(7.0f));
	TestTrue(TEXT("Button style survives"), First->GetSelectButton()->GetStyle().Normal == ButtonStyle.Normal);
	TestTrue(TEXT("Count updates independently"), Count->GetText().ToString().Contains(TEXT("4")));
	for (int32 Index = 1; Index < Entries.Num(); ++Index)
	{
		TestEqual(TEXT("Excess rows collapse"), Entries[Index]->GetVisibility(), ESlateVisibility::Collapsed);
		TestFalse(TEXT("Excess rows cannot send a stale click"), Entries[Index]->GetSelectButton()->GetIsEnabled());
		TestNull(TEXT("Excess row tooltips clear"), Entries[Index]->GetSelectButton()->GetToolTip());
	}
	Panel->ConfigureEntries({});
	TestTrue(TEXT("Cleared name cannot show fake preview data"), Name->GetText().IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoRuneBackpackRoutingTest,
                                 "ReEcho.UI.Shop.RuneBackpackRouting",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoRuneBackpackRoutingTest::RunTest(const FString&)
{
	UClass* ShopClass = LoadClass<UReEchoInventoryShopWidget>(
	    nullptr, TEXT("/Game/ReEcho/UI/WBP_ReEchoInventoryShopScreen.WBP_ReEchoInventoryShopScreen_C"));
	if (!TestNotNull(TEXT("Shop class loads"), ShopClass))
	{
		return false;
	}
	UReEchoInventoryShopWidget* Shop = NewObject<UReEchoInventoryShopWidget>(GetTransientPackage(), ShopClass);
	Shop->Initialize();
	const TSharedRef<SWidget> Slate = Shop->TakeWidget();
	FReEchoWeaponPartShopView View;
	View.WeaponId = TEXT("W_J_02");
	View.WeaponTypeId = TEXT("LongSword");
	FReEchoWeaponSlotShopView Slot;
	Slot.SlotTypeId = TEXT("Core");
	Slot.Capacity = 1;
	View.Slots.Add(Slot);
	Slot.SlotTypeId = TEXT("SwordBlade");
	Slot.Capacity = 2;
	View.Slots.Add(Slot);
	Slot.SlotTypeId = TEXT("Grip");
	View.Slots.Add(Slot);
	FReEchoShopOffer Offer;
	Offer.ItemId = TEXT("P_LONGSWORD_HASTE_GRIP_I");
	Offer.ContentId = Offer.ItemId;
	Offer.WeaponTypeId = View.WeaponTypeId;
	Offer.SlotTypeId = TEXT("Grip");
	Offer.Type = EReEchoShopOfferType::WeaponPart;
	Offer.DisplayName = FText::FromString(TEXT("测试剑柄"));
	Offer.EffectText = FText::FromString(TEXT("真实符文说明"));
	Offer.BackpackCount = 2;
	View.OwnedParts.Add(Offer);
	FReEchoShopOffer Empty = Offer;
	Empty.ItemId = TEXT("P_LONGSWORD_HEAVY_GRIP_I");
	Empty.ContentId = Empty.ItemId;
	Empty.BackpackCount = 0;
	View.OwnedParts.Add(Empty);
	Shop->SetWeaponPartShopView(View);
	Shop->ShowShop(50, {});
	UButton* Grip = Cast<UButton>(Shop->GetWidgetFromName(TEXT("DesignerDualAttachmentSlot4")));
	if (!TestNotNull(TEXT("Second grip slot exists"), Grip))
	{
		return false;
	}
	FName Equipped;
	int32 Occurrence = INDEX_NONE;
	int32 Requests = 0;
	int32 Purchases = 0;
	Shop->OnOwnedPartEquipRequested.AddLambda(
	    [&](FName Id, int32 Index)
	    {
		    Equipped = Id;
		    Occurrence = Index;
		    ++Requests;
	    });
	Shop->OnPurchaseRequested.AddLambda(
	    [&](FName)
	    {
		    ++Purchases;
	    });
	Grip->OnClicked.Broadcast();
	UReEchoRuneBackpackWidget* Panel =
	    Cast<UReEchoRuneBackpackWidget>(Shop->GetWidgetFromName(TEXT("BackpackPopupPanel")));
	if (!TestNotNull(TEXT("Real shop uses new panel"), Panel))
	{
		return false;
	}
	const auto Entries = Panel->GetEntryWidgets();
	if (!TestTrue(TEXT("Real entries exist"), Entries.Num() >= 2))
	{
		return false;
	}
	TestEqual(TEXT("Zero backpack count omitted"), Entries[1]->GetVisibility(), ESlateVisibility::Collapsed);
	TestNotNull(TEXT("Tooltip remains authored shared class"),
	            Cast<UReEchoShopTooltipWidget>(Entries[0]->GetSelectButton()->GetToolTip()));
	Entries[0]->GetSelectButton()->OnClicked.Broadcast();
	TestEqual(TEXT("Exactly one equip request"), Requests, 1);
	TestEqual(TEXT("Stable rune ID"), Equipped, Offer.ItemId);
	TestEqual(TEXT("Second slot occurrence retained"), Occurrence, 1);
	TestEqual(TEXT("No purchase transaction"), Purchases, 0);
	USizeBox* Root = CastChecked<USizeBox>(Panel->GetWidgetFromName(TEXT("BackpackRootSizeBox")));
	Root->SetWidthOverride(460.0f);
	Root->SetHeightOverride(520.0f);
	Grip->OnClicked.Broadcast();
	TestEqual(TEXT("Second click closes"), Panel->GetVisibility(), ESlateVisibility::Collapsed);
	Entries[0]->GetSelectButton()->OnClicked.Broadcast();
	TestEqual(TEXT("Closed row sends no stale request"), Requests, 1);
	Grip->OnClicked.Broadcast();
	Panel->ForceLayoutPrepass();
	TestTrue(TEXT("Reopening reuses the same panel"), Shop->GetWidgetFromName(TEXT("BackpackPopupPanel")) == Panel);
	TestTrue(TEXT("Host uses automatic authored sizing"), CastChecked<UCanvasPanelSlot>(Panel->Slot)->GetAutoSize());
	TestEqual(TEXT("Host never resets user width/height"), Panel->GetDesiredSize(), FVector2D(460.0f, 520.0f));
	Entries[0]->GetSelectButton()->OnClicked.Broadcast();
	TestEqual(TEXT("Reopen binds once"), Requests, 2);
	return true;
}

#if WITH_EDITOR
#include "Brushes/SlateColorBrush.h"
#include "Engine/TextureRenderTarget2D.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Slate/WidgetRenderer.h"
#include "Widgets/Layout/SBorder.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoRuneBackpackRenderingTest,
                                 "ReEcho.UI.RuneBackpackRendering.DesignerParity",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter |
                                     EAutomationTestFlags::NonNullRHI)

bool FReEchoRuneBackpackRenderingTest::RunTest(const FString&)
{
	UReEchoRuneBackpackWidget* Designer = MakeRuneBackpack(true);
	UReEchoRuneBackpackWidget* Runtime = MakeRuneBackpack();
	if (!TestNotNull(TEXT("Designer panel"), Designer) || !TestNotNull(TEXT("Runtime panel"), Runtime))
	{
		return false;
	}
	TArray<FReEchoRuneBackpackEntryView> Views;
	// Transient stress sample: longest production name plus a three-digit stack.
	// Do not alter the authored preview assets just to exercise this regression.
	UReEchoRuneBackpackEntryWidget* LongNameRow = Designer->GetEntryWidgets().Last();
	LongNameRow->DesignerPreview.Name = FText::FromString(TEXT("【握柄】镰刃命中攻速握柄III"));
	LongNameRow->DesignerPreview.Count = 999;
	for (UReEchoRuneBackpackEntryWidget* Row : Designer->GetEntryWidgets())
	{
		Views.Add(Row->DesignerPreview);
	}
	Runtime->ConfigureEntries(Views);
	FWidgetRenderer Renderer(true, true);
	const FSlateColorBrush Background(FLinearColor(0.12f, 0.08f, 0.06f, 1.0f));
	TArray<FColor> DesignerPixels;
	FVector2D DesignerSize = FVector2D::ZeroVector;
	IFileManager::Get().MakeDirectory(*(FPaths::ProjectSavedDir() / TEXT("Automation/Plan161")), true);
	for (UReEchoRuneBackpackWidget* Panel : {Designer, Runtime})
	{
		TSharedRef<SWidget> Canvas = SNew(SBorder)
		                                 .BorderImage(&Background)
		                                 .Padding(24.0f)
		                                 .HAlign(HAlign_Left)
		                                 .VAlign(VAlign_Top)[Panel->TakeWidget()];
		const FVector2D Size(640, 620);
		UTextureRenderTarget2D* Target = Renderer.DrawWidget(Canvas, Size);
		if (!TestNotNull(TEXT("Render target"), Target))
		{
			return false;
		}
		Renderer.DrawWidget(Target, Canvas, Size, 0.0f);
		TArray<FColor> Pixels;
		FReadSurfaceDataFlags ReadFlags;
		ReadFlags.SetLinearToGamma(false);
		if (!TestTrue(TEXT("Pixels read"), Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels, ReadFlags)))
		{
			return false;
		}
		TArray64<uint8> Png;
		FImageUtils::PNGCompressImageArray(640, 620, MakeArrayView(Pixels), Png);
		const FString Filename =
		    Panel == Designer ? TEXT("RuneBackpack-Designer.png") : TEXT("RuneBackpack-Runtime.png");
		TestTrue(
		    TEXT("Preview evidence saved"),
		    FFileHelper::SaveArrayToFile(Png, *(FPaths::ProjectSavedDir() / TEXT("Automation/Plan161") / Filename)));
		TestTrue(TEXT("Frame edge visibly brighter than inner surface"),
		         Pixels[25 * 640 + 100].R > Pixels[32 * 640 + 100].R + 40);
		for (UReEchoRuneBackpackEntryWidget* Entry : Panel->GetEntryWidgets())
		{
			const UTextBlock* Name = CastChecked<UTextBlock>(Entry->GetWidgetFromName(TEXT("NameText")));
			TestFalse(TEXT("Name stays on one line"), Name->GetAutoWrapText());
			TestTrue(TEXT("Complete name fits beside icon and stack count"),
			         Name->GetDesiredSize().X <= Name->GetCachedGeometry().GetLocalSize().X + 1.0f);
			const UImage* Icon = CastChecked<UImage>(Entry->GetWidgetFromName(TEXT("RuneIcon")));
			const FVector2D Geometry = Icon->GetCachedGeometry().GetLocalSize();
			TestTrue(TEXT("Icon has visible nonzero geometry"), Geometry.X > 1 && Geometry.Y > 1);
			const FVector2D NativeSize = Icon->GetBrush().ImageSize;
			TestTrue(TEXT("Icon has native aspect ratio"),
			         FMath::IsNearlyEqual(Geometry.X / Geometry.Y, NativeSize.X / NativeSize.Y, 0.02));
		}
		if (Panel == Designer)
		{
			DesignerPixels = Pixels;
			DesignerSize = Panel->GetDesiredSize();
			TestTrue(TEXT("Panel independent preview uses Desired Size"),
			         Panel->DesignSizeMode == EDesignPreviewSizeMode::Desired);
			TestTrue(TEXT("Entry independent preview uses Desired Size"),
			         Panel->GetEntryWidgets()[0]->DesignSizeMode == EDesignPreviewSizeMode::Desired);
		}
		else
		{
			int32 Different = 0;
			for (int32 Index = 0; Index < Pixels.Num(); ++Index)
			{
				Different += Pixels[Index] != DesignerPixels[Index] ? 1 : 0;
			}
			AddInfo(FString::Printf(TEXT("Designer/runtime pixel differences: %d / %d"), Different, Pixels.Num()));
			TestTrue(TEXT("Designer and runtime agree for the same contents"), Different < Pixels.Num() / 100);
			TestEqual(TEXT("Designer/runtime desired size identical"), Panel->GetDesiredSize(), DesignerSize);
		}
	}
	return true;
}
#endif
#endif
