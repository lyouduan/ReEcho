#include "Misc/AutomationTest.h"

#include "Components/Button.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "UI/ReEchoIndexedButton.h"
#include "UI/ReEchoTraitCardChoiceWidget.h"
#include "UI/ReEchoTraitCardEntryWidget.h"
#include "UObject/UnrealType.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoTraitChoiceAuthoringTest,
                                 "ReEcho.UI.TraitChoice.AuthoredPresentation",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoTraitChoiceAuthoringTest::RunTest(const FString& Parameters)
{
	UClass* ChoiceClass = LoadClass<UReEchoTraitCardChoiceWidget>(
	    nullptr, TEXT("/Game/ReEcho/UI/WBP_ReEchoTraitCardChoice.WBP_ReEchoTraitCardChoice_C"));
	if (!TestNotNull(TEXT("Formal choice Blueprint loads"), ChoiceClass))
	{
		return false;
	}
	UReEchoTraitCardChoiceWidget* Choice = NewObject<UReEchoTraitCardChoiceWidget>(GetTransientPackage(), ChoiceClass);
	Choice->Initialize();
	UTextBlock* Title = Cast<UTextBlock>(Choice->GetWidgetFromName(TEXT("TitleText")));
	UTextBlock* Label = Cast<UTextBlock>(Choice->GetWidgetFromName(TEXT("ShopCardRefreshText0")));
	UReEchoIndexedButton* Refresh =
	    Cast<UReEchoIndexedButton>(Choice->GetWidgetFromName(TEXT("ShopCardRefreshButton0")));
	UButton* Confirm = Cast<UButton>(Choice->GetWidgetFromName(TEXT("ConfirmButton")));
	if (!TestTrue(TEXT("Authored bindings exist"), Title && Label && Refresh && Confirm))
	{
		return false;
	}
	UCanvasPanelSlot* TitleSlot = Cast<UCanvasPanelSlot>(Title->Slot);
	UCanvasPanelSlot* RefreshSlot = Cast<UCanvasPanelSlot>(Refresh->Slot);
	if (!TestTrue(TEXT("Authored positions remain in Canvas slots"), TitleSlot && RefreshSlot))
	{
		return false;
	}
	// Deliberately non-default author values, on a transient instance only.
	Title->SetText(FText::FromString(TEXT("挑选12张奖励")));
	Label->SetText(FText::FromString(TEXT("刷新(剩余52次)\n876 时间碎片")));
	UTextBlock* StaticLabel = Cast<UTextBlock>(Choice->GetWidgetFromName(TEXT("ShopCardRefreshText1")));
	UTextBlock* EmptyLabel = Cast<UTextBlock>(Choice->GetWidgetFromName(TEXT("ShopCardRefreshText2")));
	if (!TestTrue(TEXT("All authored refresh labels exist"), StaticLabel && EmptyLabel))
	{
		return false;
	}
	StaticLabel->SetText(FText::FromString(TEXT("重新选择")));
	EmptyLabel->SetText(FText::GetEmpty());
	TitleSlot->SetPosition(FVector2D(37.0f, 93.0f));
	TitleSlot->SetSize(FVector2D(420.0f, 82.0f));
	RefreshSlot->SetPosition(FVector2D(-231.0f, 632.0f));
	RefreshSlot->SetSize(FVector2D(254.0f, 88.0f));
	FSlateFontInfo TitleFont = Title->GetFont();
	TitleFont.Size = 43;
	Title->SetFont(TitleFont);
	FSlateFontInfo LabelFont = Label->GetFont();
	LabelFont.Size = 27;
	Label->SetFont(LabelFont);
	Label->SetJustification(ETextJustify::Right);
	Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.2f, 0.7f, 0.4f, 0.8f)));
	FButtonStyle Style = Refresh->GetStyle();
	Style.Normal.TintColor = FSlateColor(FLinearColor::Red);
	Style.Hovered.TintColor = FSlateColor(FLinearColor::Green);
	Style.Pressed.TintColor = FSlateColor(FLinearColor::Blue);
	Style.Disabled.TintColor = FSlateColor(FLinearColor(0.3f, 0.2f, 0.5f, 0.6f));
	Style.NormalPadding = FMargin(3.0f, 8.0f, 4.0f, 9.0f);
	Style.PressedPadding = FMargin(11.0f, 2.0f, 5.0f, 6.0f);
	Refresh->SetStyle(Style);
	UWidget* ButtonContent = Refresh->GetContent();
	auto VerifyAuthorValues =
	    [this, Title, Label, Refresh, TitleSlot, RefreshSlot, TitleFont, LabelFont, Style, ButtonContent]()
	{
		TestEqual(TEXT("Title author position survives"), TitleSlot->GetPosition(), FVector2D(37.0f, 93.0f));
		TestEqual(TEXT("Title author size survives"), TitleSlot->GetSize(), FVector2D(420.0f, 82.0f));
		TestEqual(TEXT("Refresh author position survives"), RefreshSlot->GetPosition(), FVector2D(-231.0f, 632.0f));
		TestEqual(TEXT("Refresh author size survives"), RefreshSlot->GetSize(), FVector2D(254.0f, 88.0f));
		TestTrue(TEXT("Both author fonts survive"), Title->GetFont() == TitleFont && Label->GetFont() == LabelFont);
		const FByteProperty* Justification =
		    FindFProperty<FByteProperty>(UTextBlock::StaticClass(), TEXT("Justification"));
		TestTrue(TEXT("Author text alignment survives"),
		         Justification && Justification->GetPropertyValue_InContainer(Label) == ETextJustify::Right);
		TestTrue(TEXT("Author text color survives"),
		         Label->GetColorAndOpacity() == FSlateColor(FLinearColor(0.2f, 0.7f, 0.4f, 0.8f)));
		const FButtonStyle& Current = Refresh->GetStyle();
		TestTrue(TEXT("All authored button brushes survive"),
		         Current.Normal == Style.Normal && Current.Hovered == Style.Hovered &&
		             Current.Pressed == Style.Pressed && Current.Disabled == Style.Disabled);
		TestTrue(TEXT("Authored normal/pressed padding survives"),
		         Current.NormalPadding == Style.NormalPadding && Current.PressedPadding == Style.PressedPadding);
		TestTrue(TEXT("Authored button content is not replaced"), Refresh->GetContent() == ButtonContent);
	};
	TArray<FReEchoTraitCardOffer> Offers;
	for (int32 Index = 0; Index < 3; ++Index)
	{
		FReEchoTraitCardOffer Offer;
		Offer.CardId = FName(*FString::Printf(TEXT("G_2_0%d"), Index + 1));
		Offer.DisplayName = FText::FromString(TEXT("测试候选"));
		Offer.Description = FText::FromString(TEXT("测试说明"));
		Offer.Tier = 2;
		Offer.PresentationTier = 2;
		Offer.SlotIndex = Index;
		Offer.RemainingRefreshes = 2;
		Offer.RefreshCost = 5;
		Offer.bCanRefresh = true;
		Offers.Add(Offer);
	}
	// Data may arrive before NativeConstruct. Capture the title before that first write.
	Choice->InitializeOffers(Offers, 20);
	VerifyAuthorValues();
	Choice->TakeWidget();
	VerifyAuthorValues();
	TestEqual(
	    TEXT("Authored title copy changes only its count"), Title->GetText().ToString(), FString(TEXT("挑选1张奖励")));
	TestEqual(TEXT("Refresh keeps units, punctuation and newline"),
	          Label->GetText().ToString(),
	          FString(TEXT("刷新(剩余2次)\n5 时间碎片")));
	TestEqual(
	    TEXT("Non-numeric author copy stays static"), StaticLabel->GetText().ToString(), FString(TEXT("重新选择")));
	TestTrue(TEXT("Intentionally empty author text stays empty"), EmptyLabel->GetText().IsEmpty());
	Choice->AdvanceRevealAnimationForTesting(10.0f);
	TestTrue(TEXT("Affordable refresh is enabled after reveal"), Refresh->GetIsEnabled());
	Offers[0].RemainingRefreshes = 1;
	Offers[0].RefreshCost = 15;
	Choice->InitializeOffers(Offers, 10, 0);
	USizeBox* FirstCard = Cast<USizeBox>(Choice->GetWidgetFromName(TEXT("TraitCardSlot0")));
	USizeBox* SecondCard = Cast<USizeBox>(Choice->GetWidgetFromName(TEXT("TraitCardSlot1")));
	TestTrue(TEXT("Refresh still reveals only the changed slot"),
	         FirstCard && SecondCard && FirstCard->GetRenderOpacity() == 0.0f &&
	             SecondCard->GetRenderOpacity() == 1.0f);
	Choice->AdvanceRevealAnimationForTesting(10.0f);
	TestEqual(TEXT("Cached template updates multi-digit cost"),
	          Label->GetText().ToString(),
	          FString(TEXT("刷新(剩余1次)\n15 时间碎片")));
	TestFalse(TEXT("Insufficient balance still disables refresh"), Refresh->GetIsEnabled());
	Choice->RestoreChoiceFailure(20);
	TestTrue(TEXT("Failure recovery respects new balance"), Refresh->GetIsEnabled());
	VerifyAuthorValues();
	Choice->SetSelectableCount(2);
	Choice->InitializeOffers(Offers, 20);
	Choice->AdvanceRevealAnimationForTesting(10.0f);
	TestEqual(TEXT("Double pick keeps author copy"), Title->GetText().ToString(), FString(TEXT("挑选2张奖励")));
	UReEchoTraitCardEntryWidget* FirstEntry =
	    Cast<UReEchoTraitCardEntryWidget>(Choice->GetWidgetFromName(TEXT("TraitCardEntry0")));
	UReEchoTraitCardEntryWidget* SecondEntry =
	    Cast<UReEchoTraitCardEntryWidget>(Choice->GetWidgetFromName(TEXT("TraitCardEntry1")));
	if (TestTrue(TEXT("Runtime card entries exist"), FirstEntry && SecondEntry))
	{
		FirstEntry->OnEntrySelected.Broadcast(0);
		TestFalse(TEXT("Double pick cannot confirm one selection"), Confirm->GetIsEnabled());
		SecondEntry->OnEntrySelected.Broadcast(1);
		TestTrue(TEXT("Double pick can confirm exactly two selections"), Confirm->GetIsEnabled());
	}
	Choice->ReleaseSlateResources(true);
	Choice->TakeWidget();
	Choice->SetSelectableCount(1);
	Offers[0].RemainingRefreshes = 0;
	Offers[0].RefreshCost = 0;
	Offers[0].bCanRefresh = false;
	Choice->InitializeOffers(Offers, 20);
	Choice->AdvanceRevealAnimationForTesting(10.0f);
	TestEqual(TEXT("Reconstruction does not recapture live count"),
	          Title->GetText().ToString(),
	          FString(TEXT("挑选1张奖励")));
	TestEqual(TEXT("Zero values retain authored units and newline"),
	          Label->GetText().ToString(),
	          FString(TEXT("刷新(剩余0次)\n0 时间碎片")));
	TestFalse(TEXT("Exhausted slot stays disabled"), Refresh->GetIsEnabled());
	VerifyAuthorValues();

	// Explicit named placeholders permit arbitrary order and unrelated numeric copy.
	UReEchoTraitCardChoiceWidget* Explicit =
	    NewObject<UReEchoTraitCardChoiceWidget>(GetTransientPackage(), ChoiceClass);
	Explicit->Initialize();
	UTextBlock* ExplicitTitle = Cast<UTextBlock>(Explicit->GetWidgetFromName(TEXT("TitleText")));
	UTextBlock* ExplicitLabel = Cast<UTextBlock>(Explicit->GetWidgetFromName(TEXT("ShopCardRefreshText0")));
	ExplicitTitle->SetText(FText::FromString(TEXT("第2026期：选{Count}张{Tier}卡牌")));
	ExplicitLabel->SetText(FText::FromString(TEXT("费用 {Cost}\n余下 {Remaining} 次")));
	Explicit->TakeWidget();
	FReEchoShopCardChoiceOffer ShopOffer;
	ShopOffer.CardId = TEXT("G_2_01");
	ShopOffer.ItemId = TEXT("SHOP_TEST_1");
	ShopOffer.DisplayName = FText::FromString(TEXT("商店候选"));
	ShopOffer.Tier = 2;
	ShopOffer.SlotIndex = 0;
	ShopOffer.RemainingRefreshes = 3;
	ShopOffer.RefreshCost = 12;
	ShopOffer.bCanRefresh = true;
	Explicit->InitializeShopOffers({ShopOffer}, 20, 2);
	TestEqual(TEXT("Shop title fills named count/tier without touching unrelated numbers"),
	          ExplicitTitle->GetText().ToString(),
	          FString(TEXT("第2026期：选1张二级卡牌")));
	TestEqual(TEXT("Refresh supports explicit reversed argument order"),
	          ExplicitLabel->GetText().ToString(),
	          FString(TEXT("费用 12\n余下 3 次")));
	return true;
}

#endif
