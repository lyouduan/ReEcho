#include "UI/ReEchoLoadoutSelectionWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Data/ReEchoCsvDataRegistry.h"
#include "Engine/Texture2D.h"
#include "ReEcho.h"
#include "UI/ReEchoIndexedButton.h"
#include "UI/ReEchoLoadoutEntryWidget.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
const FLinearColor SelectedColor(0.08f, 0.52f, 0.34f, 1.0f);
const FLinearColor UnselectedColor(0.10f, 0.18f, 0.30f, 1.0f);

int32 CharacterPresentationOrder(const FName CharacterId)
{
	if (CharacterId == TEXT("J_HEART"))
	{
		return 0;
	}
	if (CharacterId == TEXT("J_SPADE"))
	{
		return 1;
	}
	if (CharacterId == TEXT("J_CLOVER"))
	{
		return 2;
	}
	if (CharacterId == TEXT("J_DIAMOND"))
	{
		return 3;
	}
	return 1000;
}

int32 WeaponPresentationOrder(const FName WeaponId)
{
	if (WeaponId == TEXT("W_J_04"))
	{
		return 0;
	}
	if (WeaponId == TEXT("W_J_09"))
	{
		return 1;
	}
	if (WeaponId == TEXT("W_J_01"))
	{
		return 2;
	}
	if (WeaponId == TEXT("W_J_08"))
	{
		return 3;
	}
	return 1000;
}

FString LoadoutTexturePath(const TCHAR* Domain, const FName StableId, const TCHAR* State)
{
	const FString AssetName = FString::Printf(TEXT("T_UI_Loadout_%s_%s_%s"), Domain, *StableId.ToString(), State);
	return FString::Printf(TEXT("/Game/ReEcho/Textures/UI/LoadoutSelection/%s.%s"), *AssetName, *AssetName);
}

UTextBlock* AddSectionLabel(UWidgetTree* WidgetTree, UVerticalBox* Content, const FString& Label)
{
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
	Text->SetText(FText::FromString(Label));
	Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	Text->SetJustification(ETextJustify::Center);
	FSlateFontInfo Font = Text->GetFont();
	Font.Size = 24;
	Text->SetFont(Font);
	UVerticalBoxSlot* Slot = Content->AddChildToVerticalBox(Text);
	Slot->SetHorizontalAlignment(HAlign_Center);
	Slot->SetPadding(FMargin(0.0f, 14.0f, 0.0f, 8.0f));
	return Text;
}

UButton* AddOptionButton(UWidgetTree* WidgetTree, UHorizontalBox* Row, const FName Name, const FString& Label)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
	Button->SetBackgroundColor(UnselectedColor);
	UHorizontalBoxSlot* Slot = Row->AddChildToHorizontalBox(Button);
	Slot->SetPadding(FMargin(5.0f));
	Slot->SetVerticalAlignment(VAlign_Center);

	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
	Text->SetText(FText::FromString(Label));
	Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	Text->SetJustification(ETextJustify::Center);
	Text->SetMargin(FMargin(18.0f, 11.0f));
	FSlateFontInfo Font = Text->GetFont();
	Font.Size = 20;
	Text->SetFont(Font);
	Button->SetContent(Text);
	return Button;
}

UReEchoIndexedButton* AddImageOptionButton(UWidgetTree* WidgetTree,
                                           UHorizontalBox* Row,
                                           const FName Name,
                                           const FString& Label,
                                           const FString& TexturePath,
                                           const int32 OptionIndex)
{
	UReEchoIndexedButton* Button =
	    WidgetTree->ConstructWidget<UReEchoIndexedButton>(UReEchoIndexedButton::StaticClass(), Name);
	Button->SetEntryIndex(OptionIndex);
	Button->SetBackgroundColor(UnselectedColor);
	UHorizontalBoxSlot* ButtonSlot = Row->AddChildToHorizontalBox(Button);
	ButtonSlot->SetPadding(FMargin(7.0f));
	ButtonSlot->SetVerticalAlignment(VAlign_Center);

	UVerticalBox* Card = WidgetTree->ConstructWidget<UVerticalBox>();
	Button->SetContent(Card);

	USizeBox* PortraitSize = WidgetTree->ConstructWidget<USizeBox>();
	PortraitSize->SetWidthOverride(128.0f);
	PortraitSize->SetHeightOverride(128.0f);
	UScaleBox* PortraitScale = WidgetTree->ConstructWidget<UScaleBox>();
	PortraitScale->SetStretch(EStretch::ScaleToFit);
	UImage* Portrait = WidgetTree->ConstructWidget<UImage>();
	if (UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *TexturePath))
	{
		Portrait->SetBrushFromTexture(Texture, true);
	}
	PortraitScale->SetContent(Portrait);
	PortraitSize->SetContent(PortraitScale);
	UVerticalBoxSlot* PortraitSlot = Card->AddChildToVerticalBox(PortraitSize);
	PortraitSlot->SetHorizontalAlignment(HAlign_Center);
	PortraitSlot->SetPadding(FMargin(12.0f, 12.0f, 12.0f, 4.0f));

	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
	Text->SetText(FText::FromString(Label));
	Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	Text->SetJustification(ETextJustify::Center);
	FSlateFontInfo Font = Text->GetFont();
	Font.Size = 20;
	Text->SetFont(Font);
	UVerticalBoxSlot* TextSlot = Card->AddChildToVerticalBox(Text);
	TextSlot->SetHorizontalAlignment(HAlign_Fill);
	TextSlot->SetPadding(FMargin(8.0f, 4.0f, 8.0f, 10.0f));
	return Button;
}

void SetSelected(UButton* Button, const bool bSelected)
{
	if (Button)
	{
		Button->SetBackgroundColor(bSelected ? SelectedColor : UnselectedColor);
	}
}

FString CharacterTexturePath(const FName AppearanceId)
{
	if (AppearanceId == TEXT("J_SPADE"))
	{
		return TEXT("/Game/ReEcho/Textures/Characters/NewCast/Player_Cat.Player_Cat");
	}

	FString Token = AppearanceId.ToString();
	Token.RemoveFromStart(TEXT("J_"));
	Token = Token.ToLower();
	if (!Token.IsEmpty())
	{
		Token[0] = FChar::ToUpper(Token[0]);
	}
	return FString::Printf(TEXT("/Game/ReEcho/Textures/Characters/NewCast/Player_%s.Player_%s"), *Token, *Token);
}

FString WeaponTexturePath(const FName VisualKey)
{
	if (VisualKey == TEXT("MoonStaff"))
	{
		return TEXT("/Game/ReEcho/Textures/Effects/MoonStaff.MoonStaff");
	}
	if (VisualKey == TEXT("CrescentBlade"))
	{
		return TEXT("/Game/ReEcho/Textures/Effects/CrescentWeapon.CrescentWeapon");
	}
	if (VisualKey == TEXT("ElementalOrb"))
	{
		return TEXT("/Game/ReEcho/Textures/Effects/StaffLightWave.StaffLightWave");
	}
	if (VisualKey == TEXT("Scythe"))
	{
		return TEXT("/Game/ReEcho/Textures/Effects/Scythe.Scythe");
	}
	if (VisualKey == TEXT("Bow"))
	{
		return TEXT("/Game/ReEcho/Textures/Effects/Bow.Bow");
	}
	if (VisualKey == TEXT("Gun"))
	{
		return TEXT("/Game/ReEcho/Textures/Effects/Gun.Gun");
	}
	return FString();
}
}

UReEchoLoadoutSelectionWidget::UReEchoLoadoutSelectionWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
	static ConstructorHelpers::FClassFinder<UReEchoLoadoutEntryWidget> EntryClassFinder(
	    TEXT("/Game/ReEcho/UI/WBP_ReEchoLoadoutEntry"));
	EntryWidgetClass = EntryClassFinder.Class;
}

TSharedRef<SWidget> UReEchoLoadoutSelectionWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		BuildWidgetTree();
	}
	return Super::RebuildWidget();
}

void UReEchoLoadoutSelectionWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	BuildWidgetTree();
	SelectionStage = ESelectionStage::Character;
	SelectedCharacterId = NAME_None;
	SelectedWeaponId = NAME_None;
	bFinalConfirmationBroadcast = false;
	bShowAnchoredDescription = false;
	LoadOptions();
	BuildOptionEntries();

	for (UReEchoIndexedButton* Button : CharacterButtons)
	{
		Button->OnIndexedClicked.AddUniqueDynamic(this, &UReEchoLoadoutSelectionWidget::HandleCharacterClicked);
	}
	for (UReEchoIndexedButton* Button : WeaponButtons)
	{
		Button->OnIndexedClicked.AddUniqueDynamic(this, &UReEchoLoadoutSelectionWidget::HandleWeaponClicked);
	}
	for (UReEchoLoadoutEntryWidget* Entry : CharacterEntries)
	{
		Entry->OnEntrySelected.AddUniqueDynamic(this, &UReEchoLoadoutSelectionWidget::HandleCharacterClicked);
		Entry->OnEntryPreviewed.AddUniqueDynamic(this, &UReEchoLoadoutSelectionWidget::HandleCharacterPreviewed);
		Entry->OnEntryHovered.AddUniqueDynamic(this, &UReEchoLoadoutSelectionWidget::HandleCharacterHovered);
	}
	for (UReEchoLoadoutEntryWidget* Entry : WeaponEntries)
	{
		Entry->OnEntrySelected.AddUniqueDynamic(this, &UReEchoLoadoutSelectionWidget::HandleWeaponClicked);
		Entry->OnEntryPreviewed.AddUniqueDynamic(this, &UReEchoLoadoutSelectionWidget::HandleWeaponPreviewed);
		Entry->OnEntryHovered.AddUniqueDynamic(this, &UReEchoLoadoutSelectionWidget::HandleWeaponHovered);
	}
	if (ConfirmButton)
	{
		ConfirmButton->OnClicked.AddUniqueDynamic(this, &UReEchoLoadoutSelectionWidget::HandleConfirmClicked);
	}
	RefreshSelection();
	SetKeyboardFocus();
}

void UReEchoLoadoutSelectionWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
#if WITH_EDITOR
	if (!IsDesignTime())
	{
		return;
	}
	const bool bCharacterStage = DesignerPreviewStage == EReEchoLoadoutDesignerPreviewStage::Character;
	if (CharacterStagePanel)
	{
		CharacterStagePanel->SetVisibility(bCharacterStage ? ESlateVisibility::SelfHitTestInvisible
		                                                   : ESlateVisibility::Collapsed);
	}
	if (WeaponStagePanel)
	{
		WeaponStagePanel->SetVisibility(bCharacterStage ? ESlateVisibility::Collapsed
		                                                : ESlateVisibility::SelfHitTestInvisible);
	}
	if (TitleText)
	{
		TitleText->SetText(FText::FromString(bCharacterStage ? TEXT("选择角色") : TEXT("选择武器")));
	}
	UHorizontalBox* PreviewRow = bCharacterStage ? CharacterRow : WeaponRow;
	if (PreviewRow)
	{
		for (int32 ChildIndex = 0; ChildIndex < PreviewRow->GetChildrenCount(); ++ChildIndex)
		{
			if (UReEchoLoadoutEntryWidget* Entry = Cast<UReEchoLoadoutEntryWidget>(PreviewRow->GetChildAt(ChildIndex)))
			{
				Entry->ApplyDesignerPreviewState(DesignerPreviewIndex < 0 || DesignerPreviewIndex == ChildIndex);
			}
		}
	}
	const bool bHasPreview = DesignerPreviewIndex >= 0;
	ApplySelectionArrowVisibility(bCharacterStage, bHasPreview ? DesignerPreviewIndex : INDEX_NONE);
	if (ConfirmButton)
	{
		ConfirmButton->SetVisibility(bHasPreview ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (ConfirmButtonLabel)
	{
		ConfirmButtonLabel->SetVisibility(bHasPreview ? ESlateVisibility::HitTestInvisible
		                                              : ESlateVisibility::Collapsed);
	}
#endif
}

void UReEchoLoadoutSelectionWidget::LoadOptions()
{
	CharacterOptionIds.Reset();
	WeaponOptionIds.Reset();
	CharacterLabels.Reset();
	WeaponLabels.Reset();
	CharacterDescriptions.Reset();
	WeaponDescriptions.Reset();

	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
	if (!Snapshot.IsValid())
	{
		UE_LOG(LogReEcho, Fatal, TEXT("Cannot build loadout UI: CSV snapshot is unavailable"));
	}
	TArray<FReEchoCsvCharacterRow> Characters;
	for (const TPair<FName, FReEchoCsvCharacterRow>& Pair : Snapshot->Characters)
	{
		const FReEchoCsvCharacterRow& Character = Pair.Value;
		if (Character.bEnabled && !Character.SourceWorkbookId.ToString().StartsWith(TEXT("RuntimeOnly.")))
		{
			Characters.Add(Character);
		}
	}
	Characters.Sort(
	    [](const FReEchoCsvCharacterRow& Left, const FReEchoCsvCharacterRow& Right)
	    {
		    const int32 LeftOrder = CharacterPresentationOrder(Left.Id);
		    const int32 RightOrder = CharacterPresentationOrder(Right.Id);
		    if (LeftOrder != RightOrder)
		    {
			    return LeftOrder < RightOrder;
		    }
		    return Left.PromotionPriority == Right.PromotionPriority ? Left.Id.LexicalLess(Right.Id)
		                                                             : Left.PromotionPriority < Right.PromotionPriority;
	    });
	if (Characters.Num() != 4)
	{
		UE_LOG(LogReEcho,
		       Fatal,
		       TEXT("Cannot build loadout UI: expected 4 selectable CSV characters, found %d"),
		       Characters.Num());
	}
	for (const FReEchoCsvCharacterRow& Character : Characters)
	{
		CharacterOptionIds.Add(Character.Id);
		CharacterLabels.Add(Character.Id, Character.DisplayName);
		CharacterDescriptions.Add(Character.Id, Character.Description);
	}

	TArray<FReEchoCsvWeaponRow> Weapons = Snapshot->GetStartSelectableWeapons();
	if (Weapons.Num() < 1)
	{
		UE_LOG(LogReEcho,
		       Fatal,
		       TEXT("Cannot build loadout UI: expected at least 1 StartSelectable CSV weapon, found %d"),
		       Weapons.Num());
	}
	Weapons.Sort(
	    [](const FReEchoCsvWeaponRow& Left, const FReEchoCsvWeaponRow& Right)
	    {
		    const int32 LeftOrder = WeaponPresentationOrder(Left.Id);
		    const int32 RightOrder = WeaponPresentationOrder(Right.Id);
		    if (LeftOrder != RightOrder)
		    {
			    return LeftOrder < RightOrder;
		    }
		    return Left.LoadoutOrder == Right.LoadoutOrder ? Left.Id.LexicalLess(Right.Id)
		                                                   : Left.LoadoutOrder < Right.LoadoutOrder;
	    });
	for (const FReEchoCsvWeaponRow& Weapon : Weapons)
	{
		if (Weapon.Id.IsNone() || WeaponTexturePath(Weapon.VisualKey).IsEmpty())
		{
			UE_LOG(LogReEcho, Fatal, TEXT("Cannot build loadout UI: weapon configuration is incomplete"));
		}
		WeaponOptionIds.Add(Weapon.Id);
		WeaponLabels.Add(Weapon.Id,
		                 Weapon.Id == TEXT("W_J_01")
		                     ? TEXT("剑")
		                     : (Weapon.DisplayName.IsEmpty() ? Weapon.Id.ToString() : Weapon.DisplayName));
		const FReEchoCsvWeaponTypeRow* WeaponType = Snapshot->FindWeaponType(Weapon.WeaponTypeId);
		WeaponDescriptions.Add(Weapon.Id, WeaponType ? WeaponType->Description : FString());
	}
}

void UReEchoLoadoutSelectionWidget::BuildWidgetTree()
{
	if (ConfirmButton || !WidgetTree)
	{
		return;
	}
	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LoadoutBackground"));
	Background->SetBrushColor(FLinearColor(0.015f, 0.02f, 0.04f, 0.96f));
	Background->SetPadding(FMargin(80.0f));
	Background->SetHorizontalAlignment(HAlign_Center);
	Background->SetVerticalAlignment(VAlign_Center);
	WidgetTree->RootWidget = Background;

	UVerticalBox* Content =
	    WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LoadoutContent"));
	Background->SetContent(Content);

	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
	TitleText->SetText(FText::FromString(TEXT("选择角色")));
	TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.4f, 0.85f, 1.0f)));
	TitleText->SetJustification(ETextJustify::Center);
	FSlateFontInfo TitleFont = TitleText->GetFont();
	TitleFont.Size = 42;
	TitleText->SetFont(TitleFont);
	UVerticalBoxSlot* TitleSlot = Content->AddChildToVerticalBox(TitleText);
	TitleSlot->SetHorizontalAlignment(HAlign_Center);
	TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

	UTextBlock* Hint = WidgetTree->ConstructWidget<UTextBlock>();
	Hint->SetText(FText::FromString(TEXT("选择本局初始武器，局内仍可切换")));
	Hint->SetColorAndOpacity(FSlateColor(FLinearColor(0.82f, 0.86f, 0.92f)));
	Hint->SetJustification(ETextJustify::Center);
	FSlateFontInfo HintFont = Hint->GetFont();
	HintFont.Size = 19;
	Hint->SetFont(HintFont);
	UVerticalBoxSlot* HintSlot = Content->AddChildToVerticalBox(Hint);
	HintSlot->SetHorizontalAlignment(HAlign_Center);

	AddSectionLabel(WidgetTree, Content, TEXT("角色"));
	CharacterRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("CharacterRow"));
	UVerticalBoxSlot* CharacterSlot = Content->AddChildToVerticalBox(CharacterRow);
	CharacterSlot->SetHorizontalAlignment(HAlign_Center);

	AddSectionLabel(WidgetTree, Content, TEXT("武器"));
	WeaponRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("WeaponRow"));
	UVerticalBoxSlot* WeaponSlot = Content->AddChildToVerticalBox(WeaponRow);
	WeaponSlot->SetHorizontalAlignment(HAlign_Center);

	DescriptionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DescriptionText"));
	DescriptionText->SetAutoWrapText(true);
	DescriptionText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	DescriptionText->SetJustification(ETextJustify::Left);
	FSlateFontInfo DescriptionFont = DescriptionText->GetFont();
	DescriptionFont.Size = 16;
	DescriptionText->SetFont(DescriptionFont);
	UVerticalBoxSlot* DescriptionSlot = Content->AddChildToVerticalBox(DescriptionText);
	DescriptionSlot->SetHorizontalAlignment(HAlign_Center);
	DescriptionSlot->SetPadding(FMargin(0.0f, 12.0f));

	StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LoadoutStatus"));
	StatusText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	StatusText->SetJustification(ETextJustify::Center);
	FSlateFontInfo StatusFont = StatusText->GetFont();
	StatusFont.Size = 19;
	StatusText->SetFont(StatusFont);
	UVerticalBoxSlot* StatusSlot = Content->AddChildToVerticalBox(StatusText);
	StatusSlot->SetHorizontalAlignment(HAlign_Center);
	StatusSlot->SetPadding(FMargin(0.0f, 20.0f, 0.0f, 8.0f));

	UHorizontalBox* ConfirmRow =
	    WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ConfirmRow"));
	ConfirmButton = AddOptionButton(WidgetTree, ConfirmRow, TEXT("ConfirmButton"), TEXT("确认并进入第 1 关"));
	UVerticalBoxSlot* ConfirmSlot = Content->AddChildToVerticalBox(ConfirmRow);
	ConfirmSlot->SetHorizontalAlignment(HAlign_Center);
	ConfirmSlot->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 0.0f));
	RefreshSelection();
}

void UReEchoLoadoutSelectionWidget::BuildOptionEntries()
{
	if (!WidgetTree || !CharacterRow || !WeaponRow)
	{
		return;
	}

	CharacterButtons.Reset();
	WeaponButtons.Reset();
	CharacterEntries.Reset();
	WeaponEntries.Reset();

	for (int32 ChildIndex = 0; ChildIndex < CharacterRow->GetChildrenCount(); ++ChildIndex)
	{
		if (UReEchoLoadoutEntryWidget* Entry = Cast<UReEchoLoadoutEntryWidget>(CharacterRow->GetChildAt(ChildIndex)))
		{
			CharacterEntries.Add(Entry);
		}
	}
	for (int32 ChildIndex = 0; ChildIndex < WeaponRow->GetChildrenCount(); ++ChildIndex)
	{
		if (UReEchoLoadoutEntryWidget* Entry = Cast<UReEchoLoadoutEntryWidget>(WeaponRow->GetChildAt(ChildIndex)))
		{
			WeaponEntries.Add(Entry);
		}
	}
	const bool bReuseAuthoredCharacterEntries = CharacterEntries.Num() == CharacterOptionIds.Num();
	const bool bReuseAuthoredWeaponEntries = WeaponEntries.Num() == WeaponOptionIds.Num();
	if (!bReuseAuthoredCharacterEntries)
	{
		CharacterEntries.Reset();
		CharacterRow->ClearChildren();
	}
	if (!bReuseAuthoredWeaponEntries)
	{
		WeaponEntries.Reset();
		WeaponRow->ClearChildren();
	}

	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
	if (!Snapshot.IsValid())
	{
		return;
	}

	for (int32 OptionIndex = 0; OptionIndex < CharacterOptionIds.Num(); ++OptionIndex)
	{
		const FName CharacterId = CharacterOptionIds[OptionIndex];
		if (EntryWidgetClass)
		{
			UReEchoLoadoutEntryWidget* Entry =
			    bReuseAuthoredCharacterEntries
			        ? CharacterEntries[OptionIndex].Get()
			        : WidgetTree->ConstructWidget<UReEchoLoadoutEntryWidget>(
			              EntryWidgetClass, *FString::Printf(TEXT("CharacterEntry%d"), OptionIndex));
			if (!bReuseAuthoredCharacterEntries)
			{
				CharacterRow->AddChildToHorizontalBox(Entry)->SetPadding(FMargin(7.0f));
				CharacterEntries.Add(Entry);
			}
			Entry->Configure(OptionIndex,
			                 FText::FromString(CharacterLabels.FindRef(CharacterId)),
			                 FText::FromString(CharacterDescriptions.FindRef(CharacterId)),
			                 LoadoutTexturePath(TEXT("Character"), CharacterId, TEXT("Selected")),
			                 LoadoutTexturePath(TEXT("Character"), CharacterId, TEXT("Unselected")),
			                 CharacterTexturePath(Snapshot->FindCharacter(CharacterId)->AppearanceId),
			                 390.0f,
			                 560.0f);
		}
		else
		{
			CharacterButtons.Add(
			    AddImageOptionButton(WidgetTree,
			                         CharacterRow,
			                         *FString::Printf(TEXT("CharacterButton%d"), OptionIndex),
			                         CharacterLabels.FindRef(CharacterId),
			                         CharacterTexturePath(Snapshot->FindCharacter(CharacterId)->AppearanceId),
			                         OptionIndex));
		}
	}
	for (int32 OptionIndex = 0; OptionIndex < WeaponOptionIds.Num(); ++OptionIndex)
	{
		const FName WeaponId = WeaponOptionIds[OptionIndex];
		if (EntryWidgetClass)
		{
			UReEchoLoadoutEntryWidget* Entry =
			    bReuseAuthoredWeaponEntries
			        ? WeaponEntries[OptionIndex].Get()
			        : WidgetTree->ConstructWidget<UReEchoLoadoutEntryWidget>(
			              EntryWidgetClass, *FString::Printf(TEXT("WeaponEntry%d"), OptionIndex));
			if (!bReuseAuthoredWeaponEntries)
			{
				WeaponRow->AddChildToHorizontalBox(Entry)->SetPadding(FMargin(7.0f));
				WeaponEntries.Add(Entry);
			}
			Entry->Configure(OptionIndex,
			                 FText::FromString(WeaponLabels.FindRef(WeaponId)),
			                 FText::FromString(WeaponDescriptions.FindRef(WeaponId)),
			                 LoadoutTexturePath(TEXT("Weapon"), WeaponId, TEXT("Selected")),
			                 LoadoutTexturePath(TEXT("Weapon"), WeaponId, TEXT("Unselected")),
			                 WeaponTexturePath(Snapshot->FindWeapon(WeaponId)->VisualKey),
			                 280.0f,
			                 560.0f);
		}
		else
		{
			WeaponButtons.Add(AddImageOptionButton(WidgetTree,
			                                       WeaponRow,
			                                       *FString::Printf(TEXT("WeaponButton%d"), OptionIndex),
			                                       WeaponLabels.FindRef(WeaponId),
			                                       WeaponTexturePath(Snapshot->FindWeapon(WeaponId)->VisualKey),
			                                       OptionIndex));
		}
	}
}

void UReEchoLoadoutSelectionWidget::RefreshSelection()
{
	const bool bCharacterStage = SelectionStage == ESelectionStage::Character;
	const bool bHasCharacterPreview = !SelectedCharacterId.IsNone();
	const bool bHasWeaponPreview = !SelectedWeaponId.IsNone();
	const bool bHasCurrentPreview = bCharacterStage ? bHasCharacterPreview : bHasWeaponPreview;
	if (CharacterStagePanel)
	{
		CharacterStagePanel->SetVisibility(bCharacterStage ? ESlateVisibility::SelfHitTestInvisible
		                                                   : ESlateVisibility::Collapsed);
	}
	if (WeaponStagePanel)
	{
		WeaponStagePanel->SetVisibility(bCharacterStage ? ESlateVisibility::Collapsed
		                                                : ESlateVisibility::SelfHitTestInvisible);
	}
	if (CharacterRow)
	{
		CharacterRow->SetVisibility(bCharacterStage ? ESlateVisibility::SelfHitTestInvisible
		                                            : ESlateVisibility::Collapsed);
	}
	if (WeaponRow)
	{
		WeaponRow->SetVisibility(bCharacterStage ? ESlateVisibility::Collapsed
		                                         : ESlateVisibility::SelfHitTestInvisible);
	}
	if (TitleText)
	{
		TitleText->SetText(FText::FromString(bCharacterStage ? TEXT("选择角色") : TEXT("选择武器")));
	}
	for (int32 OptionIndex = 0; OptionIndex < CharacterButtons.Num(); ++OptionIndex)
	{
		SetSelected(CharacterButtons[OptionIndex],
		            CharacterOptionIds.IsValidIndex(OptionIndex) &&
		                SelectedCharacterId == CharacterOptionIds[OptionIndex]);
	}
	for (int32 OptionIndex = 0; OptionIndex < WeaponButtons.Num(); ++OptionIndex)
	{
		SetSelected(WeaponButtons[OptionIndex],
		            WeaponOptionIds.IsValidIndex(OptionIndex) && SelectedWeaponId == WeaponOptionIds[OptionIndex]);
	}
	for (int32 OptionIndex = 0; OptionIndex < CharacterEntries.Num(); ++OptionIndex)
	{
		CharacterEntries[OptionIndex]->SetPresentationState(bHasCharacterPreview,
		                                                    CharacterOptionIds.IsValidIndex(OptionIndex) &&
		                                                        SelectedCharacterId == CharacterOptionIds[OptionIndex]);
	}
	for (int32 OptionIndex = 0; OptionIndex < WeaponEntries.Num(); ++OptionIndex)
	{
		WeaponEntries[OptionIndex]->SetPresentationState(bHasWeaponPreview,
		                                                 WeaponOptionIds.IsValidIndex(OptionIndex) &&
		                                                     SelectedWeaponId == WeaponOptionIds[OptionIndex]);
	}
	if (StatusText)
	{
		StatusText->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (DescriptionText)
	{
		DescriptionText->SetText(FText::FromString(bCharacterStage ? CharacterDescriptions.FindRef(SelectedCharacterId)
		                                                           : WeaponDescriptions.FindRef(SelectedWeaponId)));
		DescriptionText->SetVisibility(bHasCurrentPreview && bShowAnchoredDescription
		                                   ? ESlateVisibility::HitTestInvisible
		                                   : ESlateVisibility::Collapsed);
	}
	if (DescriptionPanel)
	{
		DescriptionPanel->SetVisibility(bHasCurrentPreview && bShowAnchoredDescription
		                                    ? ESlateVisibility::HitTestInvisible
		                                    : ESlateVisibility::Collapsed);
	}
	RefreshSelectionArrow();
	if (ConfirmButton)
	{
		ConfirmButton->SetIsEnabled(bHasCurrentPreview);
		ConfirmButton->SetVisibility(bHasCurrentPreview ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (ConfirmButtonLabel)
	{
		ConfirmButtonLabel->SetVisibility(bHasCurrentPreview ? ESlateVisibility::HitTestInvisible
		                                                     : ESlateVisibility::Collapsed);
	}
}

void UReEchoLoadoutSelectionWidget::RefreshSelectionArrow()
{
	static const float CharacterDescriptionX[] = {526.0f, 930.0f, 526.0f, 930.0f};
	static const float WeaponDescriptionX[] = {666.0f, 960.0f, 524.0f, 818.0f};
	const bool bCharacterStage = SelectionStage == ESelectionStage::Character;
	const int32 PreviewIndex = bCharacterStage ? CharacterOptionIds.IndexOfByKey(SelectedCharacterId)
	                                           : WeaponOptionIds.IndexOfByKey(SelectedWeaponId);
	ApplySelectionArrowVisibility(bCharacterStage, PreviewIndex);
	if (PreviewIndex < 0 || PreviewIndex >= 4)
	{
		return;
	}

	if (UCanvasPanelSlot* DescriptionPanelSlot =
	        DescriptionPanel ? Cast<UCanvasPanelSlot>(DescriptionPanel->Slot) : nullptr)
	{
		DescriptionPanelSlot->SetPosition(FVector2D(
		    bCharacterStage ? CharacterDescriptionX[PreviewIndex] : WeaponDescriptionX[PreviewIndex], 330.0f));
	}
	if (UCanvasPanelSlot* DescriptionTextSlot =
	        DescriptionTextScale ? Cast<UCanvasPanelSlot>(DescriptionTextScale->Slot) : nullptr)
	{
		DescriptionTextSlot->SetPosition(FVector2D(
		    (bCharacterStage ? CharacterDescriptionX[PreviewIndex] : WeaponDescriptionX[PreviewIndex]) + 20.0f,
		    350.0f));
	}
}

void UReEchoLoadoutSelectionWidget::ApplySelectionArrowVisibility(const bool bCharacterStage, const int32 PreviewIndex)
{
	UImage* AllArrows[] = {CharacterSelectionArrow0.Get(),
	                       CharacterSelectionArrow1.Get(),
	                       CharacterSelectionArrow2.Get(),
	                       SelectionArrow.Get(),
	                       WeaponSelectionArrow0.Get(),
	                       WeaponSelectionArrow1.Get(),
	                       WeaponSelectionArrow2.Get(),
	                       WeaponSelectionArrow3.Get()};
	for (UImage* Arrow : AllArrows)
	{
		if (Arrow)
		{
			Arrow->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	if (UImage* ActiveArrow = GetSelectionArrow(bCharacterStage, PreviewIndex))
	{
		ActiveArrow->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

UImage* UReEchoLoadoutSelectionWidget::GetSelectionArrow(const bool bCharacterStage, const int32 PreviewIndex) const
{
	if (PreviewIndex < 0 || PreviewIndex >= 4)
	{
		return nullptr;
	}
	if (bCharacterStage)
	{
		switch (PreviewIndex)
		{
			case 0:
				return CharacterSelectionArrow0.Get();
			case 1:
				return CharacterSelectionArrow1.Get();
			case 2:
				return CharacterSelectionArrow2.Get();
			default:
				return SelectionArrow.Get();
		}
	}
	switch (PreviewIndex)
	{
		case 0:
			return WeaponSelectionArrow0.Get();
		case 1:
			return WeaponSelectionArrow1.Get();
		case 2:
			return WeaponSelectionArrow2.Get();
		default:
			return WeaponSelectionArrow3.Get();
	}
}

void UReEchoLoadoutSelectionWidget::SetSelectionStage(const ESelectionStage NewStage)
{
	SelectionStage = NewStage;
	if (SelectionStage == ESelectionStage::Weapon)
	{
		SelectedWeaponId = NAME_None;
	}
	bShowAnchoredDescription = false;
	RefreshSelection();
	SetKeyboardFocus();
}

void UReEchoLoadoutSelectionWidget::SelectCharacter(const FName CharacterId)
{
	SelectedCharacterId = CharacterId;
	RefreshSelection();
}

void UReEchoLoadoutSelectionWidget::ChooseWeapon(const FName WeaponId)
{
	SelectedWeaponId = WeaponId;
	RefreshSelection();
}

void UReEchoLoadoutSelectionWidget::HandleCharacterClicked(const int32 OptionIndex)
{
	bShowAnchoredDescription = false;
	if (SelectionStage == ESelectionStage::Character && CharacterOptionIds.IsValidIndex(OptionIndex))
	{
		SelectCharacter(CharacterOptionIds[OptionIndex]);
	}
}

void UReEchoLoadoutSelectionWidget::HandleWeaponClicked(const int32 OptionIndex)
{
	bShowAnchoredDescription = false;
	if (SelectionStage == ESelectionStage::Weapon && WeaponOptionIds.IsValidIndex(OptionIndex))
	{
		ChooseWeapon(WeaponOptionIds[OptionIndex]);
	}
}

void UReEchoLoadoutSelectionWidget::HandleCharacterPreviewed(const int32 OptionIndex)
{
	bShowAnchoredDescription = true;
	if (SelectionStage == ESelectionStage::Character && CharacterOptionIds.IsValidIndex(OptionIndex))
	{
		SelectCharacter(CharacterOptionIds[OptionIndex]);
	}
}

void UReEchoLoadoutSelectionWidget::HandleWeaponPreviewed(const int32 OptionIndex)
{
	bShowAnchoredDescription = true;
	if (SelectionStage == ESelectionStage::Weapon && WeaponOptionIds.IsValidIndex(OptionIndex))
	{
		ChooseWeapon(WeaponOptionIds[OptionIndex]);
	}
}

void UReEchoLoadoutSelectionWidget::HandleCharacterHovered(const int32 OptionIndex)
{
	bShowAnchoredDescription = false;
	if (SelectionStage == ESelectionStage::Character && CharacterOptionIds.IsValidIndex(OptionIndex))
	{
		SelectCharacter(CharacterOptionIds[OptionIndex]);
	}
}

void UReEchoLoadoutSelectionWidget::HandleWeaponHovered(const int32 OptionIndex)
{
	bShowAnchoredDescription = false;
	if (SelectionStage == ESelectionStage::Weapon && WeaponOptionIds.IsValidIndex(OptionIndex))
	{
		ChooseWeapon(WeaponOptionIds[OptionIndex]);
	}
}

void UReEchoLoadoutSelectionWidget::HandleConfirmClicked()
{
	if (SelectionStage == ESelectionStage::Character)
	{
		if (!SelectedCharacterId.IsNone())
		{
			SetSelectionStage(ESelectionStage::Weapon);
		}
		return;
	}
	if (!bFinalConfirmationBroadcast && !SelectedCharacterId.IsNone() && !SelectedWeaponId.IsNone())
	{
		bFinalConfirmationBroadcast = true;
		if (ConfirmButton)
		{
			ConfirmButton->SetIsEnabled(false);
		}
		OnLoadoutConfirmed.Broadcast(SelectedCharacterId, SelectedWeaponId);
	}
}
