#if WITH_DEV_AUTOMATION_TESTS

#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/SizeBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Engine/Texture2D.h"
#include "Misc/AutomationTest.h"
#include "UI/ReEchoIndexedButton.h"
#include "UI/ReEchoInventoryShopWidget.h"
#include "UI/ReEchoTraitCardChoiceWidget.h"
#include "UI/ReEchoTraitCardEntryWidget.h"
#include "Weapons/ReEchoWeaponVisualCatalog.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoShopLogicBlocksTest,
                                 "ReEcho.UI.Shop.LogicBlocks",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoShopLogicBlocksTest::RunTest(const FString& Parameters)
{
	UReEchoInventoryShopWidget* Widget = NewObject<UReEchoInventoryShopWidget>(GetTransientPackage());
	TestNotNull(TEXT("Shop widget can be created"), Widget);
	if (!Widget)
	{
		return false;
	}
	TestTrue(TEXT("Shop widget initializes its widget tree"), Widget->Initialize());
	Widget->TakeWidget();
	UScaleBox* ResponsiveScale = Cast<UScaleBox>(Widget->GetWidgetFromName(TEXT("ResponsiveContentScale")));
	USizeBox* ResponsiveSize = Cast<USizeBox>(Widget->GetWidgetFromName(TEXT("ResponsiveContentSize")));
	UCanvasPanel* ResponsiveCanvas = Cast<UCanvasPanel>(Widget->GetWidgetFromName(TEXT("ResponsiveContentCanvas")));
	TestNotNull(TEXT("Shop owns a responsive design-surface scale box"), ResponsiveScale);
	TestNotNull(TEXT("Shop owns a fixed design-size host"), ResponsiveSize);
	TestNotNull(TEXT("Shop owns a responsive content canvas"), ResponsiveCanvas);
	if (ResponsiveScale)
	{
		TestEqual(
		    TEXT("Shop design surface preserves aspect ratio"), ResponsiveScale->GetStretch(), EStretch::ScaleToFit);
		TestEqual(TEXT("Shop design surface scales up and down"),
		          ResponsiveScale->GetStretchDirection(),
		          EStretchDirection::Both);
		const UCanvasPanelSlot* ScaleSlot = Cast<UCanvasPanelSlot>(ResponsiveScale->Slot);
		TestTrue(TEXT("Responsive design surface fills the viewport"),
		         ScaleSlot && ScaleSlot->GetAnchors().Minimum == FVector2D::ZeroVector &&
		             ScaleSlot->GetAnchors().Maximum == FVector2D(1.0f, 1.0f));
	}
	if (ResponsiveSize)
	{
		TestEqual(TEXT("Shop design width remains 1920"), ResponsiveSize->GetWidthOverride(), 1920.0f);
		TestEqual(TEXT("Shop design height remains 1080"), ResponsiveSize->GetHeightOverride(), 1080.0f);
	}
	TestTrue(TEXT("Fallback shop controls live on the scaled design surface"),
	         ResponsiveCanvas && Widget->GetWidgetFromName(TEXT("CloseButton"))->GetParent() == ResponsiveCanvas);

	FReEchoShopCardPackOffer TierOnePack;
	TierOnePack.Tier = 1;
	TierOnePack.DisplayName = FText::FromString(TEXT("一级"));
	TierOnePack.Status = EReEchoShopCardPackStatus::Available;
	TierOnePack.StatusText = FText::FromString(TEXT("选择"));
	FReEchoShopCardChoiceOffer TierOneChoice;
	TierOneChoice.CardId = TEXT("TEST_BUILD_CARD");
	TierOneChoice.ItemId = TEXT("TEST_CARD_CHOICE");
	TierOneChoice.Tier = 1;
	TierOneChoice.Price = 10;
	TierOnePack.Choices.Add(TierOneChoice);
	FReEchoShopCardPackOffer TierTwoPack;
	TierTwoPack.Tier = 2;
	TierTwoPack.DisplayName = FText::FromString(TEXT("二级"));
	TierTwoPack.Status = EReEchoShopCardPackStatus::NotOffered;
	TierTwoPack.StatusText = FText::FromString(TEXT("未投放"));
	FReEchoShopCardPackOffer TierThreePack = TierTwoPack;
	TierThreePack.Tier = 3;
	TierThreePack.DisplayName = FText::FromString(TEXT("三级"));

	FReEchoShopOffer WeaponPart;
	WeaponPart.ItemId = TEXT("TEST_WEAPON_PART_ITEM");
	WeaponPart.DisplayName = FText::FromString(TEXT("Weapon part"));
	WeaponPart.EffectText = FText::FromString(TEXT("Part effect"));
	WeaponPart.Price = 20;
	WeaponPart.Type = EReEchoShopOfferType::WeaponPart;
	WeaponPart.ContentId = TEXT("TEST_WEAPON_PART");
	WeaponPart.SlotTypeId = TEXT("Core");

	FReEchoWeaponPartShopView View;
	View.WeaponId = TEXT("TEST_WEAPON");
	View.WeaponDisplayName = FText::FromString(TEXT("Test weapon"));
	View.Offers = {WeaponPart};
	View.CardPackOffers = {TierOnePack, TierTwoPack, TierThreePack};
	View.WeaponRuneRefreshesRemaining = 2;
	View.WeaponRuneRefreshCost = 5;
	View.bWeaponRuneRefreshAllowed = true;
	FReEchoWeaponSlotShopView Slot;
	Slot.SlotTypeId = TEXT("Core");
	Slot.DisplayName = FText::FromString(TEXT("Core"));
	Slot.Capacity = 1;
	View.Slots.Add(Slot);

	Widget->SetWeaponPartShopView(View);
	Widget->ShowShop(100, {});
	const UTextBlock* RefreshLimitText = Cast<UTextBlock>(Widget->GetWidgetFromName(TEXT("TargetRefreshLimitText")));
	TestTrue(TEXT("Main shop shows the weapon/rune refresh budget and configured cost"),
	         RefreshLimitText && RefreshLimitText->GetText().ToString().Contains(TEXT("2")) &&
	             RefreshLimitText->GetText().ToString().Contains(TEXT("5")));

	TestNotNull(TEXT("Run item offers have an independent block"),
	            Cast<UVerticalBox>(Widget->GetWidgetFromName(TEXT("RunItemOfferPanel"))));
	TestNotNull(TEXT("Shop controls have an independent block"),
	            Cast<UVerticalBox>(Widget->GetWidgetFromName(TEXT("ShopControlPanel"))));
	TestNotNull(TEXT("Weapon part offers have an independent block"),
	            Cast<UVerticalBox>(Widget->GetWidgetFromName(TEXT("WeaponPartOfferPanel"))));
	TestNotNull(TEXT("Weapon loadout has an independent block"),
	            Cast<UVerticalBox>(Widget->GetWidgetFromName(TEXT("WeaponLoadoutPanel"))));
	TestNotNull(TEXT("Echo management has an independent block"),
	            Cast<UVerticalBox>(Widget->GetWidgetFromName(TEXT("EchoPanel"))));
	UScrollBox* ShopScrollBox = Cast<UScrollBox>(Widget->GetWidgetFromName(TEXT("ShopLogicScrollBox")));
	UVerticalBox* ShopLogicPanel = Cast<UVerticalBox>(Widget->GetWidgetFromName(TEXT("ShopLogicPanel")));
	UVerticalBox* RunItemPanel = Cast<UVerticalBox>(Widget->GetWidgetFromName(TEXT("RunItemOfferPanel")));
	UVerticalBox* WeaponPartPanel = Cast<UVerticalBox>(Widget->GetWidgetFromName(TEXT("WeaponPartOfferPanel")));
	UVerticalBox* WeaponLoadoutPanel = Cast<UVerticalBox>(Widget->GetWidgetFromName(TEXT("WeaponLoadoutPanel")));
	TestNotNull(TEXT("All shop logic blocks have a scrollable host"), ShopScrollBox);
	TestNotNull(TEXT("The scroll box owns a common logic panel"), ShopLogicPanel);
	if (ShopLogicPanel)
	{
		TestTrue(TEXT("Weapon parts are the first visible shop block"),
		         ShopLogicPanel->GetChildrenCount() > 0 && ShopLogicPanel->GetChildAt(0) == WeaponPartPanel);
		TestTrue(TEXT("Run item block is inside the scrollable host"),
		         RunItemPanel && RunItemPanel->GetParent() == ShopLogicPanel);
		TestTrue(TEXT("Weapon part block is inside the scrollable host"),
		         WeaponPartPanel && WeaponPartPanel->GetParent() == ShopLogicPanel);
		TestTrue(TEXT("Weapon loadout block is inside the scrollable host"),
		         WeaponLoadoutPanel && WeaponLoadoutPanel->GetParent() == ShopLogicPanel);
	}

	UScaleBox* EchoPanelScale = Cast<UScaleBox>(Widget->GetWidgetFromName(TEXT("EchoPanelScale")));
	TestNotNull(TEXT("Echo management has an independent popup host"), EchoPanelScale);
	if (EchoPanelScale)
	{
		const UCanvasPanelSlot* EchoCanvasSlot = Cast<UCanvasPanelSlot>(EchoPanelScale->Slot);
		TestNotNull(TEXT("Echo popup is independently anchored on the root canvas"), EchoCanvasSlot);
		if (EchoCanvasSlot)
		{
			TestEqual(TEXT("Echo popup renders over authored shop art"), EchoCanvasSlot->GetZOrder(), 40);
			TestTrue(TEXT("Echo popup is centered instead of occupying the bottom band"),
			         EchoCanvasSlot->GetAnchors().Minimum.Y < 0.30f && EchoCanvasSlot->GetAnchors().Maximum.Y > 0.70f);
		}
		TestEqual(TEXT("Echo popup starts hidden"), EchoPanelScale->GetVisibility(), ESlateVisibility::Collapsed);
	}

	TArray<FName> PurchaseRequests;
	Widget->OnPurchaseRequested.AddLambda(
	    [&PurchaseRequests](const FName ItemId)
	    {
		    PurchaseRequests.Add(ItemId);
	    });
	TArray<int32> CardPackRequests;
	Widget->OnCardPackRequested.AddLambda(
	    [&CardPackRequests](const int32 Tier)
	    {
		    CardPackRequests.Add(Tier);
	    });
	UReEchoIndexedButton* RunItemButton =
	    Cast<UReEchoIndexedButton>(Widget->GetWidgetFromName(TEXT("TargetCardPackButton0")));
	UReEchoIndexedButton* EmptyTierTwoButton =
	    Cast<UReEchoIndexedButton>(Widget->GetWidgetFromName(TEXT("TargetCardPackButton1")));
	UReEchoIndexedButton* EmptyTierThreeButton =
	    Cast<UReEchoIndexedButton>(Widget->GetWidgetFromName(TEXT("TargetCardPackButton2")));
	UReEchoIndexedButton* WeaponPartButton =
	    Cast<UReEchoIndexedButton>(Widget->GetWidgetFromName(TEXT("TargetPartBuy0")));
	TestNotNull(TEXT("Tier-one pack button exists"), RunItemButton);
	TestNotNull(TEXT("Empty tier-two slot button exists"), EmptyTierTwoButton);
	TestNotNull(TEXT("Empty tier-three slot button exists"), EmptyTierThreeButton);
	TestNotNull(TEXT("Weapon part button exists"), WeaponPartButton);
	if (!RunItemButton || !EmptyTierTwoButton || !EmptyTierThreeButton || !WeaponPartButton)
	{
		return false;
	}
	TestFalse(TEXT("Empty tier-two slot cannot be purchased"), EmptyTierTwoButton->GetIsEnabled());
	TestFalse(TEXT("Empty tier-three slot cannot be purchased"), EmptyTierThreeButton->GetIsEnabled());
	RunItemButton->OnClicked.Broadcast();
	EmptyTierTwoButton->OnClicked.Broadcast();
	EmptyTierThreeButton->OnClicked.Broadcast();
	WeaponPartButton->OnClicked.Broadcast();
	TestEqual(TEXT("Only the weapon/rune slot emits a direct purchase command"), PurchaseRequests.Num(), 1);
	TestEqual(TEXT("Only the available card pack emits an open-pack command"), CardPackRequests.Num(), 1);
	if (CardPackRequests.Num() == 1)
	{
		TestEqual(TEXT("Pack click maps to its fixed tier"), CardPackRequests[0], 1);
	}
	if (PurchaseRequests.Num() == 1)
	{
		TestEqual(TEXT("Weapon part click maps to the weapon part id"), PurchaseRequests[0], WeaponPart.ItemId);
	}
	TestNull(TEXT("Tier pack entrance deliberately has no concrete card icon"),
	         Widget->GetWidgetFromName(TEXT("TargetCardPackIcon0")));

	// Plan 67 removed the draft/save-loadout flow (purchase equals equip). An owned
	// part is no longer edited into a draft nor saved via a loadout button.
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoShopCardPackChoicePresentationTest,
                                 "ReEcho.UI.Shop.CardPackChoicePresentation",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoShopCardPackChoicePresentationTest::RunTest(const FString& Parameters)
{
	UReEchoTraitCardChoiceWidget* Widget = NewObject<UReEchoTraitCardChoiceWidget>(GetTransientPackage());
	if (!TestNotNull(TEXT("Card-pack choice widget can be created"), Widget))
	{
		return false;
	}
	TestTrue(TEXT("Card-pack choice widget initializes"), Widget->Initialize());
	Widget->TakeWidget();
	FReEchoShopCardChoiceOffer First;
	First.CardId = TEXT("G_2_01");
	First.ItemId = TEXT("SHOP_CARD_TEST_1");
	First.DisplayName = FText::FromString(TEXT("候选一"));
	First.EffectText = FText::FromString(TEXT("效果一"));
	First.Tier = 2;
	First.Price = 35;
	First.SlotIndex = 0;
	First.RemainingRefreshes = 1;
	First.RefreshCost = 5;
	First.bCanRefresh = true;
	FReEchoShopCardChoiceOffer Second = First;
	Second.CardId = TEXT("G_2_02");
	Second.ItemId = TEXT("SHOP_CARD_TEST_2");
	Second.DisplayName = FText::FromString(TEXT("候选二"));
	Second.Price = 47;
	Second.SlotIndex = 1;
	Widget->InitializeShopOffers({First, Second}, 40, 2);

	UButton* CancelButton = Cast<UButton>(Widget->GetWidgetFromName(TEXT("ShopCardChoiceCancelButton")));
	UTextBlock* Title = Cast<UTextBlock>(Widget->GetWidgetFromName(TEXT("TraitDrawTitle")));
	USizeBox* FirstCard = Cast<USizeBox>(Widget->GetWidgetFromName(TEXT("TraitCardSize0")));
	USizeBox* SecondCard = Cast<USizeBox>(Widget->GetWidgetFromName(TEXT("TraitCardSize1")));
	USizeBox* ThirdCard = Cast<USizeBox>(Widget->GetWidgetFromName(TEXT("TraitCardSize2")));
	TestTrue(TEXT("Paid pack mode exposes a visible return-to-shop button"),
	         CancelButton && CancelButton->GetVisibility() == ESlateVisibility::Visible);
	TestTrue(TEXT("Paid pack title identifies the selected tier"),
	         Title && Title->GetText().ToString().Contains(TEXT("二级")));
	TestTrue(TEXT("The two actual candidates are visible"),
	         FirstCard && SecondCard && FirstCard->GetVisibility() == ESlateVisibility::Visible &&
	             SecondCard->GetVisibility() == ESlateVisibility::Visible);
	TestTrue(TEXT("A missing third candidate stays collapsed rather than being backfilled"),
	         ThirdCard && ThirdCard->GetVisibility() == ESlateVisibility::Collapsed);
	TestNotNull(TEXT("The first paid candidate uses the authored card entry presentation"),
	            Widget->GetWidgetFromName(TEXT("TraitCardEntry0")));
	UButton* FirstRefresh = Cast<UButton>(Widget->GetWidgetFromName(TEXT("ShopCardRefreshButton0")));
	UTextBlock* FirstRefreshText = Cast<UTextBlock>(Widget->GetWidgetFromName(TEXT("ShopCardRefreshText0")));
	UButton* ThirdRefresh = Cast<UButton>(Widget->GetWidgetFromName(TEXT("ShopCardRefreshButton2")));
	TestTrue(TEXT("Each actual paid card exposes its own refresh button"),
	         FirstRefresh && FirstRefresh->GetVisibility() == ESlateVisibility::Visible);
	TestTrue(TEXT("Card refresh button displays its independent remaining use and price"),
	         FirstRefreshText && FirstRefreshText->GetText().ToString().Contains(TEXT("1")) &&
	             FirstRefreshText->GetText().ToString().Contains(TEXT("5")));
	TestTrue(TEXT("A missing candidate also hides its refresh button"),
	         ThirdRefresh && ThirdRefresh->GetVisibility() == ESlateVisibility::Collapsed);
	FReEchoShopCardChoiceOffer Third = First;
	Third.CardId = TEXT("G_2_03");
	Third.ItemId = TEXT("SHOP_CARD_TEST_3");
	Third.DisplayName = FText::FromString(TEXT("候选三"));
	Third.SlotIndex = 2;
	Widget->InitializeShopOffers({First, Second, Third}, 40, 2);
	Widget->AdvanceRevealAnimationForTesting(10.0f);
	TestTrue(TEXT("The initial pack reveal finishes all three cards"),
	         FirstCard && SecondCard && ThirdCard && FMath::IsNearlyEqual(FirstCard->GetRenderOpacity(), 1.0f) &&
	             FMath::IsNearlyEqual(SecondCard->GetRenderOpacity(), 1.0f) &&
	             FMath::IsNearlyEqual(ThirdCard->GetRenderOpacity(), 1.0f));

	FReEchoShopCardChoiceOffer RefreshedSecond = Second;
	RefreshedSecond.CardId = TEXT("G_2_04");
	RefreshedSecond.ItemId = TEXT("SHOP_CARD_TEST_2_REFRESHED");
	RefreshedSecond.DisplayName = FText::FromString(TEXT("刷新候选二"));
	Widget->InitializeShopOffers({First, RefreshedSecond, Third}, 35, 2, Second.SlotIndex);
	TestTrue(TEXT("A successful slot refresh hides only the replaced card"),
	         FMath::IsNearlyEqual(FirstCard->GetRenderOpacity(), 1.0f) &&
	             FMath::IsNearlyEqual(SecondCard->GetRenderOpacity(), 0.0f) &&
	             FMath::IsNearlyEqual(ThirdCard->GetRenderOpacity(), 1.0f));
	TestTrue(TEXT("Unchanged cards keep their completed reveal transforms"),
	         FirstCard->GetRenderTransform().Scale.Equals(FVector2D(1.0f)) &&
	             FirstCard->GetRenderTransform().Translation.IsNearlyZero() &&
	             ThirdCard->GetRenderTransform().Scale.Equals(FVector2D(1.0f)) &&
	             ThirdCard->GetRenderTransform().Translation.IsNearlyZero());
	Widget->AdvanceRevealAnimationForTesting(0.17f);
	TestTrue(TEXT("Only the replaced card advances through the slot reveal"),
	         SecondCard->GetRenderOpacity() > 0.0f && SecondCard->GetRenderOpacity() < 1.0f &&
	             FMath::IsNearlyEqual(FirstCard->GetRenderOpacity(), 1.0f) &&
	             FMath::IsNearlyEqual(ThirdCard->GetRenderOpacity(), 1.0f));
	Widget->AdvanceRevealAnimationForTesting(1.0f);
	TestTrue(TEXT("The refreshed card finishes visible without replaying its neighbors"),
	         FMath::IsNearlyEqual(FirstCard->GetRenderOpacity(), 1.0f) &&
	             FMath::IsNearlyEqual(SecondCard->GetRenderOpacity(), 1.0f) &&
	             FMath::IsNearlyEqual(ThirdCard->GetRenderOpacity(), 1.0f));

	FReEchoTraitCardOffer FreeChoice;
	FreeChoice.CardId = TEXT("G_2_03");
	FreeChoice.DisplayName = FText::FromString(TEXT("免费候选"));
	FreeChoice.Description = FText::FromString(TEXT("免费投放效果"));
	FreeChoice.Tier = 2;
	FreeChoice.SlotIndex = 0;
	FreeChoice.RemainingRefreshes = 1;
	FreeChoice.RefreshCost = 5;
	FreeChoice.bCanRefresh = true;
	Widget->InitializeOffers({FreeChoice}, 20);
	TestTrue(TEXT("Post-encounter free choice also exposes its per-card refresh button"),
	         FirstRefresh && FirstRefresh->GetVisibility() == ESlateVisibility::Visible);
	TestTrue(TEXT("Free-choice refresh uses the same remaining-count and cost projection"),
	         FirstRefreshText && FirstRefreshText->GetText().ToString().Contains(TEXT("1")) &&
	             FirstRefreshText->GetText().ToString().Contains(TEXT("5")));
	TestTrue(TEXT("Free choice keeps the shop-only cancel action hidden"),
	         CancelButton && CancelButton->GetVisibility() == ESlateVisibility::Collapsed);
	Widget->AdvanceRevealAnimationForTesting(10.0f);
	FReEchoTraitCardOffer RefreshedFreeChoice = FreeChoice;
	RefreshedFreeChoice.CardId = TEXT("G_2_04");
	RefreshedFreeChoice.DisplayName = FText::FromString(TEXT("刷新免费候选"));
	Widget->InitializeOffers({RefreshedFreeChoice}, 15, FreeChoice.SlotIndex);
	TestTrue(TEXT("Post-encounter free refresh uses the same isolated slot reveal"),
	         FMath::IsNearlyEqual(FirstCard->GetRenderOpacity(), 0.0f));
	Widget->AdvanceRevealAnimationForTesting(1.0f);
	TestTrue(TEXT("Post-encounter refreshed card becomes visible again"),
	         FMath::IsNearlyEqual(FirstCard->GetRenderOpacity(), 1.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoTraitCardAuthoredPresentationTest,
                                 "ReEcho.UI.TraitCard.AuthoredPresentation",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoTraitCardAuthoredPresentationTest::RunTest(const FString& Parameters)
{
	UClass* EntryWidgetClass = LoadClass<UReEchoTraitCardEntryWidget>(
	    nullptr, TEXT("/Game/ReEcho/UI/WBP_ReEchoTraitCardEntry.WBP_ReEchoTraitCardEntry_C"));
	if (!TestNotNull(TEXT("Authored Trait Card entry class loads"), EntryWidgetClass))
	{
		return false;
	}
	UReEchoTraitCardEntryWidget* Entry =
	    NewObject<UReEchoTraitCardEntryWidget>(GetTransientPackage(), EntryWidgetClass);
	if (!TestTrue(TEXT("Authored Trait Card entry initializes"), Entry && Entry->Initialize()))
	{
		return false;
	}
	Entry->TakeWidget();
	for (const FName ObsoleteWidgetName : {FName(TEXT("ArtCardFrame")),
	                                       FName(TEXT("ArtCardImage")),
	                                       FName(TEXT("ArtTagPrimary")),
	                                       FName(TEXT("ArtTagSecondary")),
	                                       FName(TEXT("PrimaryTagText")),
	                                       FName(TEXT("SecondaryTagText")),
	                                       FName(TEXT("CardContent")),
	                                       FName(TEXT("KickerText")),
	                                       FName(TEXT("SelectHintText"))})
	{
		TestNull(*FString::Printf(TEXT("Obsolete Trait Card widget %s is absent"), *ObsoleteWidgetName.ToString()),
		         Entry->GetWidgetFromName(ObsoleteWidgetName));
	}
	TestNotNull(TEXT("Production card art binding remains present"), Entry->GetWidgetFromName(TEXT("ArtImage")));
	const UScaleBox* CardRootScaleBox = Cast<UScaleBox>(Entry->GetWidgetFromName(TEXT("CardRootScaleBox")));
	const USizeBox* CardRootSizeBox = Cast<USizeBox>(Entry->GetWidgetFromName(TEXT("CardRootSizeBox")));
	const UButton* EntrySelectButton = Cast<UButton>(Entry->GetWidgetFromName(TEXT("SelectButton")));
	const UOverlay* EntryOverlay = Cast<UOverlay>(Entry->GetWidgetFromName(TEXT("Overlay_0")));
	TestTrue(TEXT("Designer card entry preserves the imported card aspect ratio"),
	         CardRootScaleBox && CardRootScaleBox->GetParent() == nullptr &&
	             CardRootScaleBox->GetStretch() == EStretch::ScaleToFit &&
	             CardRootScaleBox->GetStretchDirection() == EStretchDirection::Both);
	TestTrue(TEXT("Designer card entry uses the imported 420x593 art size"),
	         CardRootSizeBox && CardRootSizeBox->GetParent() == CardRootScaleBox &&
	             CardRootSizeBox->IsWidthOverride() && CardRootSizeBox->IsHeightOverride() &&
	             CardRootSizeBox->GetWidthOverride() == 420.0f &&
	             CardRootSizeBox->GetHeightOverride() == 593.0f);
	const UScaleBoxSlot* CardDesignSurfaceSlot =
	    CardRootSizeBox ? Cast<UScaleBoxSlot>(CardRootSizeBox->Slot) : nullptr;
	TestTrue(TEXT("Card design surface remains centered instead of stretching in the ScaleBox"),
	         CardDesignSurfaceSlot && CardDesignSurfaceSlot->GetHorizontalAlignment() == HAlign_Center &&
	             CardDesignSurfaceSlot->GetVerticalAlignment() == VAlign_Center);
	TestTrue(TEXT("Card selection button fills the fixed root"),
	         EntrySelectButton && EntrySelectButton->GetParent() == CardRootSizeBox &&
	             Cast<USizeBoxSlot>(EntrySelectButton->Slot));
	const UButtonSlot* EntryOverlaySlot = EntryOverlay ? Cast<UButtonSlot>(EntryOverlay->Slot) : nullptr;
	TestTrue(TEXT("Card presentation overlay fills the fixed selection button"),
	         EntryOverlaySlot && EntryOverlaySlot->GetHorizontalAlignment() == HAlign_Fill &&
	             EntryOverlaySlot->GetVerticalAlignment() == VAlign_Fill);
	const UTextBlock* SampleName = Cast<UTextBlock>(Entry->GetWidgetFromName(TEXT("NameText")));
	const UTextBlock* SampleDescription = Cast<UTextBlock>(Entry->GetWidgetFromName(TEXT("DescriptionText")));
	TestTrue(TEXT("Designer card entry has representative sample copy"),
	         SampleName && !SampleName->GetText().IsEmpty() && SampleDescription &&
	             !SampleDescription->GetText().IsEmpty());
	TestTrue(TEXT("Designer card name is freely draggable on the card canvas"),
	         SampleName && SampleName->GetParent() &&
	             SampleName->GetParent()->GetName() == TEXT("CardDesignerCanvas") &&
	             Cast<UCanvasPanelSlot>(SampleName->Slot));
	TestTrue(TEXT("Designer card description is freely draggable on the card canvas"),
	         SampleDescription && SampleDescription->GetParent() &&
	             SampleDescription->GetParent()->GetName() == TEXT("CardDesignerCanvas") &&
	             Cast<UCanvasPanelSlot>(SampleDescription->Slot));

	UClass* ChoiceWidgetClass = LoadClass<UReEchoTraitCardChoiceWidget>(
	    nullptr, TEXT("/Game/ReEcho/UI/WBP_ReEchoTraitCardChoice.WBP_ReEchoTraitCardChoice_C"));
	if (!TestNotNull(TEXT("Authored Trait Card choice class loads"), ChoiceWidgetClass))
	{
		return false;
	}
	UReEchoTraitCardChoiceWidget* Choice =
	    NewObject<UReEchoTraitCardChoiceWidget>(GetTransientPackage(), ChoiceWidgetClass);
	if (!TestTrue(TEXT("Authored Trait Card choice initializes"), Choice && Choice->Initialize()))
	{
		return false;
	}
	for (int32 SlotIndex = 0; SlotIndex < 3; ++SlotIndex)
	{
		TestNotNull(*FString::Printf(TEXT("Designer slot %d contains a sample card"), SlotIndex),
		            Choice->GetWidgetFromName(*FString::Printf(TEXT("DesignerTraitCardSample%d"), SlotIndex)));
	}
	Choice->TakeWidget();
	for (const FName ObsoleteWidgetName :
	     {FName(TEXT("SubtitleText")), FName(TEXT("CurrencyText")), FName(TEXT("NeedleWidget"))})
	{
		TestNull(*FString::Printf(TEXT("Obsolete choice widget %s is absent"), *ObsoleteWidgetName.ToString()),
		         Choice->GetWidgetFromName(ObsoleteWidgetName));
	}
	const UButton* Confirm = Cast<UButton>(Choice->GetWidgetFromName(TEXT("ConfirmButton")));
	const UTextBlock* ConfirmLabel = Cast<UTextBlock>(Choice->GetWidgetFromName(TEXT("ConfirmButtonLabel")));
	const UTextBlock* ChoiceTitle = Cast<UTextBlock>(Choice->GetWidgetFromName(TEXT("TitleText")));
	const UTexture2D* ExpectedButtonTexture =
	    LoadObject<UTexture2D>(nullptr,
	                           TEXT("/Game/ReEcho/Textures/UI/InteractionPlaceholder/PauseAndCombat/"
	                                "T_UI_Pause_ButtonLight.T_UI_Pause_ButtonLight"));
	TestTrue(TEXT("Confirm button uses the delivered light pause-button art for normal and disabled states"),
	         Confirm && ExpectedButtonTexture &&
	             Confirm->GetStyle().Normal.GetResourceObject() == ExpectedButtonTexture &&
	             Confirm->GetStyle().Disabled.GetResourceObject() == ExpectedButtonTexture);
	TestTrue(TEXT("Confirm label remains the visible interaction copy"),
	         ConfirmLabel && ConfirmLabel->GetText().ToString() == TEXT("确定"));
	TestTrue(TEXT("Choice title is freely draggable on the root canvas"),
	         ChoiceTitle && Cast<UCanvasPanelSlot>(ChoiceTitle->Slot));
	TestTrue(TEXT("Confirm label is independent from the button and freely draggable"),
	         ConfirmLabel && ConfirmLabel->GetParent() && ConfirmLabel->GetParent()->GetName() == TEXT("RootPanel") &&
	             Cast<UCanvasPanelSlot>(ConfirmLabel->Slot));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoAuthoredShopLayoutHostTest,
                                 "ReEcho.UI.Shop.AuthoredLayoutHosts",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoAuthoredShopLayoutHostTest::RunTest(const FString& Parameters)
{
	UClass* ShopWidgetClass = LoadClass<UReEchoInventoryShopWidget>(
	    nullptr, TEXT("/Game/ReEcho/UI/WBP_ReEchoInventoryShopScreen.WBP_ReEchoInventoryShopScreen_C"));
	TestNotNull(TEXT("Authored inventory/shop widget class loads"), ShopWidgetClass);
	if (!ShopWidgetClass)
	{
		return false;
	}

	UReEchoInventoryShopWidget* Widget = NewObject<UReEchoInventoryShopWidget>(GetTransientPackage(), ShopWidgetClass);
	TestNotNull(TEXT("Authored inventory/shop widget can be instantiated"), Widget);
	if (!Widget)
	{
		return false;
	}
	TestTrue(TEXT("Authored inventory/shop widget initializes"), Widget->Initialize());
	Widget->TakeWidget();
	UScaleBox* ResponsiveScale = Cast<UScaleBox>(Widget->GetWidgetFromName(TEXT("ResponsiveContentScale")));
	UCanvasPanel* ResponsiveCanvas = Cast<UCanvasPanel>(Widget->GetWidgetFromName(TEXT("ResponsiveContentCanvas")));
	UImage* BackgroundImage = Cast<UImage>(Widget->GetWidgetFromName(TEXT("BackgroundImage")));
	TestNotNull(TEXT("Authored shop uses the responsive design surface"), ResponsiveScale);
	TestNotNull(TEXT("Authored shop exposes the responsive content canvas"), ResponsiveCanvas);
	TestTrue(TEXT("Full-screen background remains outside the aspect-preserving surface"),
	         BackgroundImage && BackgroundImage->GetParent() != ResponsiveCanvas);
	TestTrue(TEXT("Authored shop controls move together on the responsive surface"),
	         ResponsiveCanvas && Widget->GetWidgetFromName(TEXT("ShopPanel"))->GetParent() == ResponsiveCanvas &&
	             Widget->GetWidgetFromName(TEXT("Overlay_0"))->GetParent() == ResponsiveCanvas &&
	             Widget->GetWidgetFromName(TEXT("CloseButton"))->GetParent() == ResponsiveCanvas);
	FReEchoShopOffer WeaponPart;
	WeaponPart.ItemId = TEXT("TEST_AUTHORED_WEAPON_PART_ITEM");
	WeaponPart.ContentId = TEXT("P_CORE_TIDE");
	WeaponPart.DisplayName = FText::FromString(TEXT("Authored weapon part"));
	WeaponPart.EffectText = FText::FromString(TEXT("Authored part effect"));
	WeaponPart.Price = 10;
	WeaponPart.Type = EReEchoShopOfferType::WeaponPart;
	WeaponPart.SlotTypeId = TEXT("Core");
	FReEchoWeaponPartShopView PartShopView;
	PartShopView.WeaponId = TEXT("TEST_AUTHORED_WEAPON");
	PartShopView.WeaponDisplayName = FText::FromString(TEXT("Authored weapon"));
	PartShopView.WeaponIconTexturePath = FReEchoWeaponVisualCatalog::ResolveHeldTexturePath(TEXT("Bow"));
	FReEchoShopOffer CurrentWeapon;
	CurrentWeapon.ItemId = TEXT("TEST_AUTHORED_WEAPON");
	CurrentWeapon.ContentId = CurrentWeapon.ItemId;
	CurrentWeapon.DisplayName = PartShopView.WeaponDisplayName;
	CurrentWeapon.EffectText = FText::FromString(TEXT("Current weapon"));
	CurrentWeapon.Type = EReEchoShopOfferType::Weapon;
	CurrentWeapon.IconTexturePath = PartShopView.WeaponIconTexturePath;
	FReEchoShopOffer AlternateWeapon = CurrentWeapon;
	AlternateWeapon.ItemId = TEXT("TEST_ALTERNATE_WEAPON");
	AlternateWeapon.ContentId = AlternateWeapon.ItemId;
	AlternateWeapon.DisplayName = FText::FromString(TEXT("Alternate weapon"));
	AlternateWeapon.EffectText = FText::FromString(TEXT("Alternate weapon effect"));
	PartShopView.OwnedWeapons = {CurrentWeapon.ContentId, AlternateWeapon.ContentId};
	PartShopView.OwnedWeaponOffers = {CurrentWeapon, AlternateWeapon};
	PartShopView.Offers.Add(WeaponPart);
	PartShopView.Offers.Add(CurrentWeapon);
	FReEchoWeaponSlotShopView PartSlot;
	PartSlot.SlotTypeId = TEXT("Core");
	PartSlot.DisplayName = FText::FromString(TEXT("Core"));
	PartSlot.Capacity = 1;
	PartShopView.Slots.Add(PartSlot);
	PartShopView.OwnedParts.Add(WeaponPart);
	FReEchoShopOffer BackpackPart = WeaponPart;
	BackpackPart.ItemId = TEXT("P_CORE_PRIMORDIAL");
	BackpackPart.ContentId = BackpackPart.ItemId;
	BackpackPart.DisplayName = FText::FromString(TEXT("Backpack rune"));
	PartShopView.OwnedParts.Add(BackpackPart);
	FReEchoEquippedPartSnapshot EquippedPart;
	EquippedPart.PartId = WeaponPart.ContentId;
	EquippedPart.SlotTypeId = WeaponPart.SlotTypeId;
	PartShopView.EquippedParts.Add(EquippedPart);
	FReEchoShopOffer StorageCard;
	StorageCard.ItemId = TEXT("SHOP_CARD_G_3_02");
	StorageCard.ContentId = FName(ReEchoEchoStorage::StorageUnlockCardId);
	StorageCard.DisplayName = FText::FromString(TEXT("时空锚点"));
	StorageCard.EffectText = FText::FromString(TEXT("开启回响存储"));
	StorageCard.OutcomeText = FText::FromString(TEXT("实际效果测试"));
	StorageCard.Type = EReEchoShopOfferType::BuildCard;
	StorageCard.Tier = 3;
	StorageCard.IconTexturePath = TEXT("/Game/ReEcho/Textures/UI/Cards/Icon/T_UI_CardIcon_G_3_02.T_UI_CardIcon_G_3_02");
	PartShopView.OwnedCards.Add(StorageCard);
	FReEchoShopOffer PurchasedCard;
	PurchasedCard.ItemId = TEXT("SHOP_CARD_0_G_1_01");
	PurchasedCard.ContentId = TEXT("G_1_01");
	PurchasedCard.DisplayName = FText::FromString(TEXT("生命强化"));
	PurchasedCard.EffectText = FText::FromString(TEXT("提高最大生命"));
	PurchasedCard.Type = EReEchoShopOfferType::BuildCard;
	PurchasedCard.Tier = 1;
	PurchasedCard.IconTexturePath =
	    TEXT("/Game/ReEcho/Textures/UI/Cards/Icon/T_UI_CardIcon_G_1_01.T_UI_CardIcon_G_1_01");
	PartShopView.Offers.Add(PurchasedCard);
	Widget->SetWeaponPartShopView(PartShopView);
	UImage* DesignerClock = Cast<UImage>(Widget->GetWidgetFromName(TEXT("DesignerShopClock")));
	UCanvasPanelSlot* DesignerClockSlot = DesignerClock ? Cast<UCanvasPanelSlot>(DesignerClock->Slot) : nullptr;
	TestNotNull(TEXT("Shop clock is authored as a direct Canvas child"), DesignerClockSlot);
	const FVector2D DesignerClockTestPosition(431.0f, 397.0f);
	if (DesignerClockSlot)
	{
		DesignerClockSlot->SetPosition(DesignerClockTestPosition);
	}
	FReEchoEchoStorageSummary EchoSummary;
	EchoSummary.StorageCapacity = 3;
	EchoSummary.bHasPendingRecording = true;
	Widget->ShowPostTraitIntermission(100, {}, EchoSummary);

	UScrollBox* ShopScrollBox = Cast<UScrollBox>(Widget->GetWidgetFromName(TEXT("ShopLogicScrollBox")));
	UScaleBox* EchoPanelScale = Cast<UScaleBox>(Widget->GetWidgetFromName(TEXT("EchoPanelScale")));
	UVerticalBox* EchoPanel = Cast<UVerticalBox>(Widget->GetWidgetFromName(TEXT("EchoPanel")));
	UVerticalBox* WeaponPartPanel = Cast<UVerticalBox>(Widget->GetWidgetFromName(TEXT("WeaponPartOfferPanel")));
	TestNotNull(TEXT("Authored shop keeps the legacy scroll host as a hidden compatibility host"), ShopScrollBox);
	TestNotNull(TEXT("Authored shop creates the target presentation layer"),
	            Cast<UCanvasPanel>(Widget->GetWidgetFromName(TEXT("ShopPresentationLayer"))));
	TestNotNull(TEXT("Authored shop creates the three-part target row"),
	            Cast<UHorizontalBox>(Widget->GetWidgetFromName(TEXT("TargetPartOfferRow"))));
	TestNotNull(TEXT("Authored shop creates a purchasable target weapon-part entry"),
	            Widget->GetWidgetFromName(TEXT("TargetPartBuy0")));
	UTexture2D* ExpectedPartIcon = LoadObject<UTexture2D>(
	    nullptr, TEXT("/Game/ReEcho/Textures/UI/WeaponParts/Icons/T_UI_Part_P_CORE_TIDE.T_UI_Part_P_CORE_TIDE"));
	UImage* OfferPartIcon = Cast<UImage>(Widget->GetWidgetFromName(TEXT("TargetCardIcon1_0")));
	TestNotNull(TEXT("Mapped weapon-part icon asset loads"), ExpectedPartIcon);
	TestTrue(TEXT("Weapon-part offer uses its PartId icon instead of the attachment placeholder"),
	         OfferPartIcon && OfferPartIcon->GetBrush().GetResourceObject() == ExpectedPartIcon);
	UReEchoIndexedButton* OwnedWeaponBuy =
	    Cast<UReEchoIndexedButton>(Widget->GetWidgetFromName(TEXT("TargetPartBuy1")));
	UTextBlock* OwnedWeaponBuyText = Cast<UTextBlock>(Widget->GetWidgetFromName(TEXT("TargetBuyText1_1")));
	TestTrue(TEXT("An owned weapon retained on the stable page is projected as unavailable"),
	         OwnedWeaponBuy && !OwnedWeaponBuy->GetIsEnabled());
	TestTrue(TEXT("An owned weapon retained on the stable page is labelled as already obtained"),
	         OwnedWeaponBuyText && OwnedWeaponBuyText->GetText().EqualTo(FText::FromString(TEXT("已获得"))));
	TestTrue(TEXT("Legacy weapon-part block is hidden behind the target composition"),
	         WeaponPartPanel && WeaponPartPanel->GetVisibility() == ESlateVisibility::Collapsed);
	TestNotNull(TEXT("Authored shop receives the independent echo popup"), EchoPanelScale);
	TestTrue(TEXT("Post-trait echo management starts hidden until the storage-card slot is clicked"),
	         EchoPanel && EchoPanel->GetVisibility() == ESlateVisibility::Collapsed);
	TestTrue(TEXT("Runtime refresh preserves the clock's Blueprint-authored position"),
	         DesignerClockSlot && DesignerClockSlot->GetPosition() == DesignerClockTestPosition);
	UImage* DesignerWeaponPanel = Cast<UImage>(Widget->GetWidgetFromName(TEXT("DesignerWeaponPanel")));
	TestNotNull(TEXT("Weapon panel is authored as a direct Canvas child"),
	            DesignerWeaponPanel ? Cast<UCanvasPanelSlot>(DesignerWeaponPanel->Slot) : nullptr);
	UButton* EquippedWeaponButton = Cast<UButton>(Widget->GetWidgetFromName(TEXT("DesignerEquippedWeaponButton")));
	UImage* EquippedWeaponArt = Cast<UImage>(Widget->GetWidgetFromName(TEXT("DesignerEquippedWeaponArt")));
	UTexture2D* ExpectedWeaponTexture = LoadObject<UTexture2D>(nullptr, *PartShopView.WeaponIconTexturePath);
	TestNotNull(TEXT("Current weapon has a clickable overlay in the authored weapon panel"), EquippedWeaponButton);
	TestTrue(TEXT("Current weapon overlay renders the equipped weapon texture"),
	         EquippedWeaponArt && ExpectedWeaponTexture &&
	             EquippedWeaponArt->GetBrush().GetResourceObject() == ExpectedWeaponTexture);
	TestTrue(TEXT("Current weapon art uses the source texture dimensions instead of the default 32px brush"),
	         EquippedWeaponArt && ExpectedWeaponTexture &&
	             EquippedWeaponArt->GetBrush().ImageSize ==
	                 FVector2D(ExpectedWeaponTexture->GetSizeX(), ExpectedWeaponTexture->GetSizeY()));
	const UButtonSlot* EquippedWeaponContentSlot = EquippedWeaponButton && EquippedWeaponButton->GetContent()
	                                                   ? Cast<UButtonSlot>(EquippedWeaponButton->GetContent()->Slot)
	                                                   : nullptr;
	TestTrue(TEXT("Current weapon art fills the authored weapon button"),
	         EquippedWeaponContentSlot && EquippedWeaponContentSlot->GetHorizontalAlignment() == HAlign_Fill &&
	             EquippedWeaponContentSlot->GetVerticalAlignment() == VAlign_Fill);
	TArray<FName> WeaponEquipRequests;
	Widget->OnWeaponEquipRequested.AddLambda(
	    [&WeaponEquipRequests](const FName WeaponId)
	    {
		    WeaponEquipRequests.Add(WeaponId);
	    });
	if (EquippedWeaponButton)
	{
		EquippedWeaponButton->OnClicked.Broadcast();
	}
	UCanvasPanel* WeaponBackpack = Cast<UCanvasPanel>(Widget->GetWidgetFromName(TEXT("WeaponBackpackPopupPanel")));
	UCanvasPanel* BackpackPopupLayer = Cast<UCanvasPanel>(Widget->GetWidgetFromName(TEXT("BackpackPopupLayer")));
	UReEchoIndexedButton* CurrentWeaponButton =
	    Cast<UReEchoIndexedButton>(Widget->GetWidgetFromName(TEXT("WeaponBackpackItem0")));
	UReEchoIndexedButton* AlternateWeaponButton =
	    Cast<UReEchoIndexedButton>(Widget->GetWidgetFromName(TEXT("WeaponBackpackItem1")));
	TestTrue(TEXT("Clicking the weapon art opens the weapon backpack"),
	         WeaponBackpack && WeaponBackpack->GetVisibility() == ESlateVisibility::Visible);
	const UCanvasPanelSlot* BackpackLayerSlot =
	    BackpackPopupLayer ? Cast<UCanvasPanelSlot>(BackpackPopupLayer->Slot) : nullptr;
	TestTrue(TEXT("Weapon backpack is parented to the root-level popup layer"),
	         WeaponBackpack && WeaponBackpack->GetParent() == BackpackPopupLayer);
	TestTrue(TEXT("Backpack popup layer renders above the shop and echo presentation"),
	         BackpackLayerSlot && BackpackLayerSlot->GetZOrder() == 100);
	TestTrue(TEXT("Current weapon is listed and cannot be equipped twice"),
	         CurrentWeaponButton && !CurrentWeaponButton->GetIsEnabled());
	TestTrue(TEXT("Another owned weapon is selectable"),
	         AlternateWeaponButton && AlternateWeaponButton->GetIsEnabled());
	if (AlternateWeaponButton)
	{
		AlternateWeaponButton->OnIndexedClicked.Broadcast(1);
	}
	TestEqual(TEXT("Weapon backpack emits one independent equip request"), WeaponEquipRequests.Num(), 1);
	if (WeaponEquipRequests.Num() == 1)
	{
		TestEqual(TEXT("Weapon backpack request carries the selected owned weapon"),
		          WeaponEquipRequests[0],
		          AlternateWeapon.ContentId);
	}
	UButton* AttachmentHoverSlot = Cast<UButton>(Widget->GetWidgetFromName(TEXT("DesignerAttachmentSlot0")));
	TestNotNull(TEXT("Equipped attachment has a hover target below the weapon"), AttachmentHoverSlot);
	TestTrue(TEXT("Attachment hover uses a custom cursor-following tooltip"),
	         AttachmentHoverSlot && Cast<UVerticalBox>(AttachmentHoverSlot->GetToolTip()) != nullptr);
	if (AttachmentHoverSlot)
	{
		const UVerticalBox* TooltipStack = Cast<UVerticalBox>(AttachmentHoverSlot->GetToolTip());
		const USizeBox* TooltipSize = TooltipStack && TooltipStack->GetChildrenCount() == 1
		                                  ? Cast<USizeBox>(TooltipStack->GetChildAt(0))
		                                  : nullptr;
		const UBorder* TooltipFrame = TooltipSize ? Cast<UBorder>(TooltipSize->GetContent()) : nullptr;
		const UBorder* TooltipSurface = TooltipFrame ? Cast<UBorder>(TooltipFrame->GetContent()) : nullptr;
		const UVerticalBox* TooltipContent =
		    TooltipSurface ? Cast<UVerticalBox>(TooltipSurface->GetContent()) : nullptr;
		const UTextBlock* TooltipEffect = TooltipContent && TooltipContent->GetChildrenCount() > 1
		                                      ? Cast<UTextBlock>(TooltipContent->GetChildAt(1))
		                                      : nullptr;
		TestTrue(TEXT("Attachment tooltip includes its effect explanation"),
		         TooltipEffect && TooltipEffect->GetText().EqualTo(WeaponPart.EffectText));
		AttachmentHoverSlot->OnClicked.Broadcast();
	}
	UCanvasPanel* RuneBackpack = Cast<UCanvasPanel>(Widget->GetWidgetFromName(TEXT("BackpackPopupPanel")));
	UScrollBox* RuneBackpackScroll = Cast<UScrollBox>(Widget->GetWidgetFromName(TEXT("BackpackPopupScroll")));
	UBorder* RuneBackpackSurface = Cast<UBorder>(Widget->GetWidgetFromName(TEXT("BackpackPopupSurface")));
	UTextBlock* RuneBackpackTitle = Cast<UTextBlock>(Widget->GetWidgetFromName(TEXT("BackpackPopupTitle")));
	UTextBlock* RuneBackpackItemName = Cast<UTextBlock>(Widget->GetWidgetFromName(TEXT("BackpackItemName0")));
	TestTrue(TEXT("Rune backpack is parented to the same root-level popup layer"),
	         RuneBackpack && RuneBackpack->GetParent() == BackpackPopupLayer);
	TestTrue(TEXT("Rune backpack uses the same framed scroll layout as the weapon backpack"),
	         RuneBackpackSurface && RuneBackpackScroll && RuneBackpackTitle && RuneBackpackItemName &&
	             RuneBackpackTitle->GetFont().Size == 20 && RuneBackpackItemName->GetFont().Size == 17);
	TestNull(TEXT("Hover detail does not add another fixed panel over the authored board"),
	         Widget->GetWidgetFromName(TEXT("SlotDetailPanel")));
	UImage* LoadoutStatsBoard = Cast<UImage>(Widget->GetWidgetFromName(TEXT("ArtLoadoutStats")));
	TestNotNull(TEXT("Authored loadout stats board still exists for layout compatibility"), LoadoutStatsBoard);
	TestTrue(TEXT("Large black loadout stats board is hidden behind the card slots"),
	         LoadoutStatsBoard && LoadoutStatsBoard->GetVisibility() == ESlateVisibility::Collapsed);
	UButton* StorageCardSlot = Cast<UButton>(Widget->GetWidgetFromName(TEXT("DesignerCardSlot0")));
	TestNotNull(TEXT("G_3_02 owns a clickable card slot"), StorageCardSlot);
	if (StorageCardSlot)
	{
		const UVerticalBox* CardTooltipStack = Cast<UVerticalBox>(StorageCardSlot->GetToolTip());
		TestNotNull(TEXT("Owned card hover uses the same custom cursor-following tooltip"), CardTooltipStack);
		TestEqual(TEXT("Owned card with a resolved result has two tooltip panels"),
		          CardTooltipStack ? CardTooltipStack->GetChildrenCount() : 0,
		          2);
		const USizeBox* OutcomeSize = CardTooltipStack && CardTooltipStack->GetChildrenCount() > 1
		                                  ? Cast<USizeBox>(CardTooltipStack->GetChildAt(1))
		                                  : nullptr;
		const UBorder* OutcomeFrame = OutcomeSize ? Cast<UBorder>(OutcomeSize->GetContent()) : nullptr;
		const UBorder* OutcomeSurface = OutcomeFrame ? Cast<UBorder>(OutcomeFrame->GetContent()) : nullptr;
		const UVerticalBox* OutcomeContent =
		    OutcomeSurface ? Cast<UVerticalBox>(OutcomeSurface->GetContent()) : nullptr;
		const UTextBlock* OutcomeBody = OutcomeContent && OutcomeContent->GetChildrenCount() > 1
		                                    ? Cast<UTextBlock>(OutcomeContent->GetChildAt(1))
		                                    : nullptr;
		TestTrue(TEXT("Resolved-result panel displays the projected outcome text"),
		         OutcomeBody && OutcomeBody->GetText().EqualTo(StorageCard.OutcomeText));
		UImage* CardImage = Cast<UImage>(Widget->GetWidgetFromName(TEXT("DesignerCardSlotArt0")));
		const UCanvasPanelSlot* DesignerCardSlot = Cast<UCanvasPanelSlot>(StorageCardSlot->Slot);
		TestNotNull(TEXT("Owned card slot is directly editable on the designer canvas"), DesignerCardSlot);
		TestNotNull(TEXT("Owned card art remains the authored button content"), CardImage);
		if (DesignerCardSlot)
		{
			TestEqual(TEXT("Owned card slot remains 60 by 60"), DesignerCardSlot->GetSize(), FVector2D(60.0f, 60.0f));
		}
		StorageCardSlot->OnClicked.Broadcast();
	}
	TestTrue(TEXT("Clicking the G_3_02 slot opens echo storage"),
	         EchoPanel && EchoPanel->GetVisibility() == ESlateVisibility::Visible && EchoPanelScale &&
	             EchoPanelScale->GetVisibility() == ESlateVisibility::SelfHitTestInvisible);
	UButton* PurchasedCardSlot = Cast<UButton>(Widget->GetWidgetFromName(TEXT("DesignerCardSlot1")));
	TestNotNull(TEXT("The next authored card slot exists"), PurchasedCardSlot);
	TestNull(TEXT("An unowned card is absent from the loadout before purchase"),
	         PurchasedCardSlot ? PurchasedCardSlot->GetToolTip() : nullptr);
	Widget->MarkItemPurchased(PurchasedCard.ItemId);
	const UVerticalBox* PurchasedCardTooltip =
	    PurchasedCardSlot ? Cast<UVerticalBox>(PurchasedCardSlot->GetToolTip()) : nullptr;
	TestNotNull(TEXT("A purchased build card appears in the loadout immediately"), PurchasedCardTooltip);
	TestEqual(TEXT("A card without a resolved result retains one tooltip panel"),
	          PurchasedCardTooltip ? PurchasedCardTooltip->GetChildrenCount() : 0,
	          1);
	UImage* PurchasedCardArt = Cast<UImage>(Widget->GetWidgetFromName(TEXT("DesignerCardSlotArt1")));
	UTexture2D* ExpectedPurchasedCardIcon = LoadObject<UTexture2D>(nullptr, *PurchasedCard.IconTexturePath);
	TestNotNull(TEXT("The purchased card icon asset loads"), ExpectedPurchasedCardIcon);
	TestTrue(TEXT("The purchased card slot shows the purchased card icon"),
	         PurchasedCardArt && PurchasedCardArt->GetBrush().GetResourceObject() == ExpectedPurchasedCardIcon);
	if (ShopScrollBox)
	{
		TestEqual(TEXT("Legacy shop scroll host does not intercept target buttons"),
		          ShopScrollBox->GetVisibility(),
		          ESlateVisibility::Collapsed);
	}
	if (EchoPanelScale)
	{
		const UCanvasPanelSlot* EchoCanvasSlot = Cast<UCanvasPanelSlot>(EchoPanelScale->Slot);
		TestNotNull(TEXT("Authored echo popup is attached to a canvas"), EchoCanvasSlot);
		if (EchoCanvasSlot)
		{
			TestEqual(TEXT("Authored echo popup renders above shop art"), EchoCanvasSlot->GetZOrder(), 40);
			TestTrue(TEXT("Authored echo popup occupies a centered modal region"),
			         EchoCanvasSlot->GetAnchors().Minimum.Y < 0.30f && EchoCanvasSlot->GetAnchors().Maximum.Y > 0.70f);
		}
	}
	for (int32 Index = 0; Index < 3; ++Index)
	{
		UButton* AttachmentSlotButton =
		    Cast<UButton>(Widget->GetWidgetFromName(*FString::Printf(TEXT("DesignerAttachmentSlot%d"), Index)));
		UImage* Attachment =
		    Cast<UImage>(Widget->GetWidgetFromName(*FString::Printf(TEXT("DesignerAttachmentSlotArt%d"), Index)));
		const UCanvasPanelSlot* AttachmentSlot =
		    AttachmentSlotButton ? Cast<UCanvasPanelSlot>(AttachmentSlotButton->Slot) : nullptr;
		TestNotNull(*FString::Printf(TEXT("Attachment slot %d exists in the authored loadout"), Index), AttachmentSlot);
		if (AttachmentSlot)
		{
			TestEqual(*FString::Printf(TEXT("Attachment slot %d keeps its authored size"), Index),
			          AttachmentSlot->GetSize(),
			          FVector2D(93.0f, 93.0f));
		}
		if (Index == 0)
		{
			TestTrue(TEXT("Equipped attachment uses the same mapped PartId icon"),
			         Attachment && Attachment->GetBrush().GetResourceObject() == ExpectedPartIcon);
		}
	}
	return true;
}

#endif
