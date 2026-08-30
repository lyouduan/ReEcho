#if WITH_DEV_AUTOMATION_TESTS

#include "UI/ReEchoShopTooltipWidget.h"
#include "UI/ReEchoAttributeTooltipWidget.h"
#include "UI/ReEchoInventoryShopWidget.h"
#include "Components/Border.h"
#include "Components/BorderSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Data/ReEchoCsvDataRegistry.h"
#include "Misc/AutomationTest.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "HAL/FileManager.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Slate/WidgetRenderer.h"
#include "Brushes/SlateColorBrush.h"
#include "UI/ReEchoLoadoutTooltipWidget.h"
#include "Widgets/Layout/SBorder.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoShopTooltipBlueprintsTest,
                                 "ReEcho.UI.Shop.TooltipBlueprints",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// Deliberately outside the NullRHI Shop suite: run with a real rendering backend.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoTooltipBackplateRenderTest,
                                 "ReEcho.UI.TooltipRendering.Backplates",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter |
                                     EAutomationTestFlags::NonNullRHI)

bool FReEchoTooltipBackplateRenderTest::RunTest(const FString& Parameters)
{
	if (!FApp::CanEverRender())
	{
		AddError(TEXT("Backplate rendering requires a GPU backend, not -NullRHI"));
		return false;
	}
	FWidgetRenderer Renderer(true, true);
	IFileManager::Get().MakeDirectory(*(FPaths::ProjectSavedDir() / TEXT("Automation/Plan157")), true);
	const FSlateColorBrush CanvasBrush(FLinearColor(0.14f, 0.10f, 0.08f, 1.0f));
	for (const TCHAR* Name : {TEXT("ShopTooltip"), TEXT("AttributeTooltip"), TEXT("LoadoutTooltip")})
	{
		const FString Path = FString::Printf(TEXT("/Game/ReEcho/UI/WBP_ReEcho%s.WBP_ReEcho%s_C"), Name, Name);
		UClass* Class = LoadClass<UUserWidget>(nullptr, *Path);
		if (!TestNotNull(TEXT("Render candidate loads"), Class))
		{
			return false;
		}
		UUserWidget* Widget = NewObject<UUserWidget>(GetTransientPackage(), Class);
		Widget->Initialize();
		const FText Title = FText::FromString(TEXT("边框验证"));
		const FText Body = FText::FromString(TEXT("保留已调整的字体与大小。\n检查文字四周的边框是否清晰可见。"));
		if (UReEchoShopTooltipWidget* Shop = Cast<UReEchoShopTooltipWidget>(Widget))
		{
			Shop->Configure(Title, Body, FText::FromString(TEXT("物理攻击力 +2\n元素攻击力 +2")));
		}
		else if (UReEchoLoadoutTooltipWidget* Loadout = Cast<UReEchoLoadoutTooltipWidget>(Widget))
		{
			Loadout->Configure(Title, Body);
		}
		else if (UReEchoAttributeTooltipWidget* Attributes = Cast<UReEchoAttributeTooltipWidget>(Widget))
		{
			TArray<FReEchoAttributeRowView> Rows;
			const auto Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
			if (!TestTrue(TEXT("Attribute render catalog exists"), Snapshot.IsValid()))
			{
				return false;
			}
			for (const FName& Id : Snapshot->GetAttributeOrder())
			{
				const FReEchoCsvAttributeRow* Definition = Snapshot->FindAttribute(Id);
				if (!Definition)
				{
					AddError(TEXT("Attribute render definition missing"));
					return false;
				}
				FReEchoAttributeRowView& Row = Rows.AddDefaulted_GetRef();
				Row.Name = FText::FromString(Definition->DisplayName);
				Row.Value =
				    FText::FromString(Definition->ValueKind == FName(TEXT("Percent")) ? TEXT("100%") : TEXT("20"));
				Row.Icon = LoadObject<UTexture2D>(nullptr,
				                                  *FString::Printf(TEXT("/Game/ReEcho/Textures/UI/Attributes/%s.%s"),
				                                                   *Definition->IconName,
				                                                   *Definition->IconName));
				TestNotNull(TEXT("Actual attribute render icon loads"), Row.Icon.Get());
			}
			Attributes->ConfigureRows(Rows);
		}
		TSharedRef<SWidget> Content = Widget->TakeWidget();
		TSharedRef<SWidget> Canvas =
		    SNew(SBorder).BorderImage(&CanvasBrush).Padding(24.0f).HAlign(HAlign_Left).VAlign(VAlign_Top)[Content];
		const FVector2D DrawSize(900.0f, 1100.0f);
		UTextureRenderTarget2D* Target = Renderer.DrawWidget(Canvas, DrawSize);
		if (!TestNotNull(TEXT("Widget render target exists"), Target))
		{
			return false;
		}
		// A second layout pass lets auto-wrapped text settle against the actual width.
		Renderer.DrawWidget(Target, Canvas, DrawSize, 0.0f);
		TArray<FColor> Pixels;
		if (!TestTrue(TEXT("GPU pixels read back"), Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels)))
		{
			return false;
		}
		const int32 CropWidth = FMath::Clamp(FMath::CeilToInt(Widget->GetDesiredSize().X) + 48, 1, 900);
		const int32 CropHeight = FMath::Clamp(FMath::CeilToInt(Widget->GetDesiredSize().Y) + 48, 1, 1100);
		TArray<FColor> CroppedPixels;
		for (int32 Y = 0; Y < CropHeight; ++Y)
		{
			CroppedPixels.Append(Pixels.GetData() + Y * 900, CropWidth);
		}
		TArray64<uint8> Png;
		FImageUtils::PNGCompressImageArray(CropWidth, CropHeight, MakeArrayView(CroppedPixels), Png);
		const FString File = FPaths::ProjectSavedDir() / TEXT("Automation/Plan157") / (FString(Name) + TEXT(".png"));
		TestTrue(TEXT("Rendered preview saved"), FFileHelper::SaveArrayToFile(Png, *File));
		AddInfo(
		    FString::Printf(TEXT("Rendered %s: %s; desired=%s"), Name, *File, *Widget->GetDesiredSize().ToString()));
		if (UVerticalBox* AttributeRows = Cast<UVerticalBox>(Widget->GetWidgetFromName(TEXT("AttributeRows"))))
		{
			for (UWidget* Child : AttributeRows->GetAllChildren())
			{
				UReEchoAttributeRowWidget* Row = Cast<UReEchoAttributeRowWidget>(Child);
				if (!Row || Row->GetVisibility() == ESlateVisibility::Collapsed)
				{
					continue;
				}
				const UImage* Icon = Cast<UImage>(Row->GetWidgetFromName(TEXT("AttributeIcon")));
				if (!TestNotNull(TEXT("Rendered attribute icon exists"), Icon))
				{
					continue;
				}
				const FGeometry& Geometry = Icon->GetCachedGeometry();
				const FVector2D Origin = Geometry.LocalToAbsolute(FVector2D::ZeroVector);
				const FVector2D End = Geometry.LocalToAbsolute(Geometry.GetLocalSize());
				TestTrue(TEXT("Rendered attribute icon has nonzero screen area"), End.X > Origin.X && End.Y > Origin.Y);
				int32 MinChannel = 255;
				int32 MaxChannel = 0;
				for (int32 Y = FMath::Clamp(FMath::CeilToInt(Origin.Y), 0, 1099);
				     Y < FMath::Min(FMath::FloorToInt(End.Y), 1100);
				     ++Y)
				{
					for (int32 X = FMath::Clamp(FMath::CeilToInt(Origin.X), 0, 899);
					     X < FMath::Min(FMath::FloorToInt(End.X), 900);
					     ++X)
					{
						const FColor Pixel = Pixels[Y * 900 + X];
						MinChannel = FMath::Min(MinChannel, static_cast<int32>(FMath::Min3(Pixel.R, Pixel.G, Pixel.B)));
						MaxChannel = FMath::Max(MaxChannel, static_cast<int32>(FMath::Max3(Pixel.R, Pixel.G, Pixel.B)));
					}
				}
				TestTrue(TEXT("Attribute icon pixels contain artwork, not an empty surface"),
				         MaxChannel - MinChannel > 20);
			}
		}
		// Sample outside edges away from text: a valid Brush can still be covered by its child.
		for (const TCHAR* FrameName : {TEXT("TooltipFrame"), TEXT("OutcomeFrame"), TEXT("AttributeTooltipFrame")})
		{
			const UWidget* Frame = Widget->GetWidgetFromName(FrameName);
			if (!Frame)
			{
				continue;
			}
			const FGeometry& Geometry = Frame->GetCachedGeometry();
			const FVector2D Origin = Geometry.GetAbsolutePosition();
			const FVector2D Size = Geometry.GetLocalSize();
			for (const FVector2D Offset : {FVector2D(Size.X * 0.5, 1.0),
			                               FVector2D(Size.X * 0.5, Size.Y - 2.0),
			                               FVector2D(1.0, Size.Y * 0.5),
			                               FVector2D(Size.X - 2.0, Size.Y * 0.5)})
			{
				const int32 X = FMath::Clamp(FMath::RoundToInt(Origin.X + Offset.X), 0, 899);
				const int32 Y = FMath::Clamp(FMath::RoundToInt(Origin.Y + Offset.Y), 0, 1099);
				const FColor Pixel = Pixels[Y * 900 + X];
				TestTrue(
				    *FString::Printf(
				        TEXT("%s.%s perimeter pixel (%d,%d) is visible: %s"), Name, FrameName, X, Y, *Pixel.ToString()),
				    Pixel.R > 190 && Pixel.G > 170 && Pixel.B > 110);
			}
		}
	}
	return true;
}

bool FReEchoShopTooltipBlueprintsTest::RunTest(const FString& Parameters)
{
	UClass* TooltipClass = LoadClass<UReEchoShopTooltipWidget>(
	    nullptr, TEXT("/Game/ReEcho/UI/WBP_ReEchoShopTooltip.WBP_ReEchoShopTooltip_C"));
	UClass* AttributesClass = LoadClass<UReEchoAttributeTooltipWidget>(
	    nullptr, TEXT("/Game/ReEcho/UI/WBP_ReEchoAttributeTooltip.WBP_ReEchoAttributeTooltip_C"));
	if (!TestNotNull(TEXT("Authored item tooltip loads"), TooltipClass) ||
	    !TestNotNull(TEXT("Authored attribute tooltip loads"), AttributesClass))
	{
		return false;
	}
	UReEchoShopTooltipWidget* Tooltip = NewObject<UReEchoShopTooltipWidget>(GetTransientPackage(), TooltipClass);
	Tooltip->Initialize();
	Tooltip->TakeWidget();
	auto TestBackplates = [this](UUserWidget* Widget, const TArray<FName>& Names)
	{
		for (const FName& Name : Names)
		{
			const UBorder* Border = Cast<UBorder>(Widget->GetWidgetFromName(Name));
			if (!TestNotNull(*FString::Printf(TEXT("Backplate %s exists"), *Name.ToString()), Border))
			{
				continue;
			}
			PRAGMA_DISABLE_DEPRECATION_WARNINGS
			TestNotNull(TEXT("Backplate owns a real renderable texture"), Border->Background.GetResourceObject());
			TestTrue(TEXT("Backplate draw mode is enabled"),
			         Border->Background.DrawAs != ESlateBrushDrawType::NoDrawType);
			TestTrue(TEXT("Backplate tint is visible"), Border->Background.TintColor.GetSpecifiedColor().A > 0.0f);
			PRAGMA_ENABLE_DEPRECATION_WARNINGS
			if (Name.ToString().EndsWith(TEXT("Frame")))
			{
				const FMargin Padding = Border->GetPadding();
				TestTrue(TEXT("Frame leaves a visible inset on all four sides"),
				         Padding.Left > 0 && Padding.Top > 0 && Padding.Right > 0 && Padding.Bottom > 0);
				const UBorderSlot* ContentSlot =
				    Border->GetContent() ? Cast<UBorderSlot>(Border->GetContent()->Slot) : nullptr;
				TestTrue(TEXT("Border and content slot agree on inset"),
				         ContentSlot && ContentSlot->GetPadding() == Padding);
			}
		}
	};
	TestBackplates(Tooltip,
	               {TEXT("TooltipFrame"), TEXT("TooltipSurface"), TEXT("OutcomeFrame"), TEXT("OutcomeSurface")});
#if WITH_EDITOR
	UReEchoShopTooltipWidget* Preview = NewObject<UReEchoShopTooltipWidget>(GetTransientPackage(), TooltipClass);
	Preview->SetDesignerFlags(EWidgetDesignFlags::Designing | EWidgetDesignFlags::ExecutePreConstruct);
	Preview->Initialize();
	// UMG caches Slate weakly; keep the preview alive while measuring its layout.
	const TSharedRef<SWidget> PreviewSlate = Preview->TakeWidget();
	Preview->ForceLayoutPrepass();
	const UTextBlock* PreviewBody = Cast<UTextBlock>(Preview->GetWidgetFromName(TEXT("DescriptionText")));
	const UTextBlock* PreviewOutcome = Cast<UTextBlock>(Preview->GetWidgetFromName(TEXT("OutcomeText")));
	TestTrue(TEXT("Designer shows the authored body sample"), PreviewBody && !PreviewBody->GetText().IsEmpty());
	TestTrue(TEXT("Designer shows the additional outcome sample"),
	         PreviewOutcome && !PreviewOutcome->GetText().IsEmpty());
	TestTrue(TEXT("Designer has a nonzero content-sized layout"),
	         Preview->GetDesiredSize().X > 0.0f && Preview->GetDesiredSize().Y > 0.0f);
	TestTrue(TEXT("Standalone preview uses Desired Size"), Preview->DesignSizeMode == EDesignPreviewSizeMode::Desired);
	UReEchoAttributeTooltipWidget* AttributePreview =
	    NewObject<UReEchoAttributeTooltipWidget>(GetTransientPackage(), AttributesClass);
	AttributePreview->SetDesignerFlags(EWidgetDesignFlags::Designing | EWidgetDesignFlags::ExecutePreConstruct);
	AttributePreview->Initialize();
	const TSharedRef<SWidget> AttributePreviewSlate = AttributePreview->TakeWidget();
	AttributePreview->ForceLayoutPrepass();
	TestTrue(TEXT("Attribute Designer has a nonzero content-sized layout"),
	         AttributePreview->GetDesiredSize().X > 0.0f && AttributePreview->GetDesiredSize().Y > 0.0f);
	TestTrue(TEXT("Attribute standalone preview uses Desired Size"),
	         AttributePreview->DesignSizeMode == EDesignPreviewSizeMode::Desired);
#endif
	UTextBlock* Body = Cast<UTextBlock>(Tooltip->GetWidgetFromName(TEXT("DescriptionText")));
	UTextBlock* Outcome = Cast<UTextBlock>(Tooltip->GetWidgetFromName(TEXT("OutcomeText")));
	UWidget* OutcomePanel = Tooltip->GetWidgetFromName(TEXT("OutcomePanel"));
	USizeBox* Root = Cast<USizeBox>(Tooltip->GetWidgetFromName(TEXT("TooltipRootSizeBox")));
	UBorder* Frame = Cast<UBorder>(Tooltip->GetWidgetFromName(TEXT("TooltipFrame")));
	if (!TestNotNull(TEXT("Body is editable"), Body) || !TestNotNull(TEXT("Outcome is editable"), Outcome) ||
	    !TestNotNull(TEXT("Outcome container is authored"), OutcomePanel) ||
	    !TestNotNull(TEXT("Root is authored"), Root) || !TestNotNull(TEXT("Frame is authored"), Frame))
	{
		return false;
	}
	FSlateFontInfo BodyFont = Body->GetFont();
	BodyFont.Size = 27;
	BodyFont.OutlineSettings.OutlineSize = 2;
	Body->SetFont(BodyFont);
	FSlateFontInfo OutcomeFont = Outcome->GetFont();
	OutcomeFont.Size = 25;
	Outcome->SetFont(OutcomeFont);
	Body->SetColorAndOpacity(FLinearColor::Green);
	Root->SetWidthOverride(460.0f);
	Frame->SetPadding(FMargin(9.0f));
	const FText Title = FText::FromString(TEXT("样样都通"));
	const FText Description =
	    FText::FromString(TEXT("生命值、攻击力与多项属性提升。\n长说明必须完整填充，不由样例覆盖。"));
	const FText Result = FText::FromString(TEXT("物理攻击力 +2\n元素攻击力 +2\n移动速度 +5%"));
	Tooltip->Configure(Title, Description, Result);
	Tooltip->TakeWidget();
	TestTrue(TEXT("Body comes from runtime"), Body->GetText().EqualTo(Description));
	TestTrue(TEXT("Multiline outcome remains verbatim"), Outcome->GetText().EqualTo(Result));
	TestEqual(TEXT("Outcome is shown"), OutcomePanel->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
	TestEqual(TEXT("Body font is not overwritten"), Body->GetFont().Size, BodyFont.Size);
	TestEqual(TEXT("Body outline is not overwritten"), Body->GetFont().OutlineSettings.OutlineSize, 2);
	TestEqual(TEXT("Outcome font is not overwritten"), Outcome->GetFont().Size, OutcomeFont.Size);
	TestTrue(TEXT("Body color is not overwritten"), Body->GetColorAndOpacity() == FSlateColor(FLinearColor::Green));
	TestEqual(TEXT("Root width is not overwritten"), Root->GetWidthOverride(), 460.0f);
	TestEqual(TEXT("Frame padding is not overwritten"), Frame->GetPadding(), FMargin(9.0f));
	Tooltip->Configure(Title, Description, FText::GetEmpty());
	TestEqual(TEXT("Empty outcome collapses the complete panel and its spacing"),
	          OutcomePanel->GetVisibility(),
	          ESlateVisibility::Collapsed);
	TestTrue(TEXT("Old outcome text is cleared"), Outcome->GetText().IsEmpty());
	Tooltip->Configure(Title, Description, FText::FromString(TEXT("  \n ")));
	TestEqual(TEXT("Whitespace outcome also collapses"), OutcomePanel->GetVisibility(), ESlateVisibility::Collapsed);
	Tooltip->Configure(Title, Description, Result);
	TestEqual(TEXT("Outcome can reappear"), OutcomePanel->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);

	UReEchoAttributeTooltipWidget* Attributes =
	    NewObject<UReEchoAttributeTooltipWidget>(GetTransientPackage(), AttributesClass);
	Attributes->Initialize();
	Attributes->TakeWidget();
	TestBackplates(Attributes, {TEXT("AttributeTooltipFrame"), TEXT("AttributeTooltipSurface")});
	UVerticalBox* List = Cast<UVerticalBox>(Attributes->GetWidgetFromName(TEXT("AttributeRows")));
	if (!TestNotNull(TEXT("Attribute list is authored"), List) ||
	    !TestTrue(TEXT("Complete preview list exists"), List->GetChildrenCount() >= 8))
	{
		return false;
	}
	UReEchoAttributeRowWidget* First = Cast<UReEchoAttributeRowWidget>(List->GetChildAt(0));
	if (!TestNotNull(TEXT("Attribute row uses the reusable WBP"), First))
	{
		return false;
	}
	First->TakeWidget();
	UTextBlock* Name = Cast<UTextBlock>(First->GetWidgetFromName(TEXT("AttributeNameText")));
	UTextBlock* Value = Cast<UTextBlock>(First->GetWidgetFromName(TEXT("AttributeValueText")));
	if (!TestNotNull(TEXT("Name text is authored"), Name) || !TestNotNull(TEXT("Value text is authored"), Value))
	{
		return false;
	}
	FSlateFontInfo NameFont = Name->GetFont();
	NameFont.Size = 28;
	Name->SetFont(NameFont);
	TArray<FReEchoAttributeRowView> Rows;
	for (int32 Index = 0; Index < 10; ++Index)
	{
		FReEchoAttributeRowView& Row = Rows.AddDefaulted_GetRef();
		Row.Name = FText::FromString(FString::Printf(TEXT("真实属性 %d"), Index));
		Row.Value = FText::FromString(TEXT("123%"));
	}
	Attributes->ConfigureRows(Rows);
	TestEqual(TEXT("Extra catalog rows use the authored row class"), List->GetChildrenCount(), 10);
	TestTrue(TEXT("Existing authored row is reused"), List->GetChildAt(0) == First);
	TestTrue(TEXT("Attribute name is filled"), Name->GetText().EqualTo(Rows[0].Name));
	TestTrue(TEXT("Attribute value is filled"), Value->GetText().EqualTo(Rows[0].Value));
	TestEqual(TEXT("Attribute name font is preserved"), Name->GetFont().Size, NameFont.Size);
	Rows.SetNum(2);
	Attributes->ConfigureRows(Rows);
	TestEqual(TEXT("Repeated refresh does not duplicate rows"), List->GetChildrenCount(), 10);
	TestEqual(TEXT("Unused rows collapse"), List->GetChildAt(2)->GetVisibility(), ESlateVisibility::Collapsed);
	Attributes->ConfigureRows({});
	TestEqual(TEXT("Empty data never leaves example attributes on screen"),
	          First->GetVisibility(),
	          ESlateVisibility::Collapsed);
	TestTrue(TEXT("Old name cleared"), Name->GetText().IsEmpty());

	// Exercise the real shop adapter, not only a synthetic tooltip call.
	UClass* ShopClass = LoadClass<UReEchoInventoryShopWidget>(
	    nullptr, TEXT("/Game/ReEcho/UI/WBP_ReEchoInventoryShopScreen.WBP_ReEchoInventoryShopScreen_C"));
	if (!TestNotNull(TEXT("Shop loads"), ShopClass))
	{
		return false;
	}
	UReEchoInventoryShopWidget* Shop = NewObject<UReEchoInventoryShopWidget>(GetTransientPackage(), ShopClass);
	Shop->Initialize();
	Shop->TakeWidget();
	FReEchoStatBlock Stats;
	Stats.HpMax = 42.0f;
	Stats.MovementSpeed = 1.23f;
	Shop->SetPlayerStats(Stats);
	UWidget* Clock = Shop->GetWidgetFromName(TEXT("DesignerShopClock"));
	UReEchoAttributeTooltipWidget* LiveAttributes =
	    Clock ? Cast<UReEchoAttributeTooltipWidget>(Clock->GetToolTip()) : nullptr;
	if (!TestNotNull(TEXT("Shop clock uses the authored attribute tooltip"), LiveAttributes))
	{
		return false;
	}
	LiveAttributes->TakeWidget();
	UVerticalBox* LiveList = Cast<UVerticalBox>(LiveAttributes->GetWidgetFromName(TEXT("AttributeRows")));
	const auto Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
	if (!TestTrue(TEXT("CSV available"), Snapshot.IsValid()) || !TestNotNull(TEXT("Live rows present"), LiveList))
	{
		return false;
	}
	for (int32 Index = 0; Index < Snapshot->GetAttributeOrder().Num(); ++Index)
	{
		const FReEchoCsvAttributeRow* Definition = Snapshot->FindAttribute(Snapshot->GetAttributeOrder()[Index]);
		UReEchoAttributeRowWidget* Row = Cast<UReEchoAttributeRowWidget>(LiveList->GetChildAt(Index));
		if (!Definition || !Row)
		{
			AddError(TEXT("CSV attribute row missing"));
			continue;
		}
		Row->TakeWidget();
		const UTextBlock* RowName = Cast<UTextBlock>(Row->GetWidgetFromName(TEXT("AttributeNameText")));
		const UTextBlock* RowValue = Cast<UTextBlock>(Row->GetWidgetFromName(TEXT("AttributeValueText")));
		const UImage* RowIcon = Cast<UImage>(Row->GetWidgetFromName(TEXT("AttributeIcon")));
		if (TestNotNull(TEXT("Live attribute icon is bound"), RowIcon))
		{
			TestNotNull(TEXT("Live attribute texture is populated"), RowIcon->GetBrush().GetResourceObject());
			TestTrue(TEXT("Authored attribute image size is nonzero"),
			         RowIcon->GetBrush().ImageSize.X > 0 && RowIcon->GetBrush().ImageSize.Y > 0);
		}
		TestEqual(TEXT("CSV name/order retained"), RowName->GetText().ToString(), Definition->DisplayName);
		if (Definition->Id == FName(TEXT("S_HP")))
		{
			TestEqual(TEXT("Actual HP, not preview HP"), RowValue->GetText().ToString(), FString(TEXT("42")));
		}
		if (Definition->Id == FName(TEXT("S_Movement_Speed")))
		{
			TestEqual(TEXT("Percentage format retained"), RowValue->GetText().ToString(), FString(TEXT("123%")));
		}
	}
	return true;
}

#endif
