#include "UI/ReEchoLoadoutSelectionWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Core/ReEchoBalanceSettings.h"
#include "Data/ReEchoCsvDataRegistry.h"
#include "Engine/Texture2D.h"
#include "ReEcho.h"

namespace
{
const FLinearColor SelectedColor(0.08f, 0.52f, 0.34f, 1.0f);
const FLinearColor UnselectedColor(0.10f, 0.18f, 0.30f, 1.0f);

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

UButton* AddImageOptionButton(
    UWidgetTree* WidgetTree, UHorizontalBox* Row, const FName Name, const FString& Label, const FString& TexturePath)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
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
	FString Token = AppearanceId.ToString();
	Token.RemoveFromStart(TEXT("J_"));
	Token = Token.ToLower();
	if (!Token.IsEmpty())
	{
		Token[0] = FChar::ToUpper(Token[0]);
	}
	return FString::Printf(TEXT("/Game/ReEcho/Textures/Characters/NewCast/Player_%s.Player_%s"), *Token, *Token);
}

FString WeaponTexturePath(const EReEchoWeaponSlot Slot)
{
	switch (Slot)
	{
		case EReEchoWeaponSlot::PhysicalOrb:
			return TEXT("/Game/ReEcho/Textures/Effects/MoonStaff.MoonStaff");
		case EReEchoWeaponSlot::Sword:
			return TEXT("/Game/ReEcho/Textures/Effects/CrescentWeapon.CrescentWeapon");
		case EReEchoWeaponSlot::ElementalOrb:
			return TEXT("/Game/ReEcho/Textures/Effects/StaffLightWave.StaffLightWave");
		default:
			return FString();
	}
}
}

TSharedRef<SWidget> UReEchoLoadoutSelectionWidget::RebuildWidget()
{
	BuildWidgetTree();
	return Super::RebuildWidget();
}

void UReEchoLoadoutSelectionWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	BuildWidgetTree();

	HeartButton->OnClicked.AddUniqueDynamic(this, &UReEchoLoadoutSelectionWidget::HandleHeartClicked);
	SpadeButton->OnClicked.AddUniqueDynamic(this, &UReEchoLoadoutSelectionWidget::HandleSpadeClicked);
	CloverButton->OnClicked.AddUniqueDynamic(this, &UReEchoLoadoutSelectionWidget::HandleCloverClicked);
	DiamondButton->OnClicked.AddUniqueDynamic(this, &UReEchoLoadoutSelectionWidget::HandleDiamondClicked);
	StaffButton->OnClicked.AddUniqueDynamic(this, &UReEchoLoadoutSelectionWidget::HandleStaffClicked);
	SwordButton->OnClicked.AddUniqueDynamic(this, &UReEchoLoadoutSelectionWidget::HandleSwordClicked);
	ElementalButton->OnClicked.AddUniqueDynamic(this, &UReEchoLoadoutSelectionWidget::HandleElementalClicked);
	ConfirmButton->OnClicked.AddUniqueDynamic(this, &UReEchoLoadoutSelectionWidget::HandleConfirmClicked);
	RefreshSelection();
	HeartButton->SetKeyboardFocus();
}

void UReEchoLoadoutSelectionWidget::LoadOptions()
{
	if (!CharacterOptionIds.IsEmpty() || !WeaponOptionIds.IsEmpty())
	{
		return;
	}

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
		    return Left.PromotionPriority == Right.PromotionPriority ? Left.Id.LexicalLess(Right.Id)
		                                                           : Left.PromotionPriority < Right.PromotionPriority;
	    });
	if (Characters.Num() != 4)
	{
		UE_LOG(LogReEcho, Fatal, TEXT("Cannot build loadout UI: expected 4 selectable CSV characters, found %d"), Characters.Num());
	}
	for (const FReEchoCsvCharacterRow& Character : Characters)
	{
		CharacterOptionIds.Add(Character.Id);
		CharacterLabels.Add(Character.Id, Character.DisplayName);
	}

	TArray<FReEchoWeaponConfig> Weapons = GetDefault<UReEchoBalanceSettings>()->Weapons;
	Weapons.Sort(
	    [](const FReEchoWeaponConfig& Left, const FReEchoWeaponConfig& Right)
	    {
		    return static_cast<uint8>(Left.Slot) < static_cast<uint8>(Right.Slot);
	    });
	if (Weapons.Num() != 3)
	{
		UE_LOG(LogReEcho, Fatal, TEXT("Cannot build loadout UI: expected 3 configured weapons, found %d"), Weapons.Num());
	}
	for (const FReEchoWeaponConfig& Weapon : Weapons)
	{
		if (Weapon.WeaponId.IsNone() || WeaponTexturePath(Weapon.Slot).IsEmpty())
		{
			UE_LOG(LogReEcho, Fatal, TEXT("Cannot build loadout UI: weapon configuration is incomplete"));
		}
		WeaponOptionIds.Add(Weapon.WeaponId);
		WeaponLabels.Add(Weapon.WeaponId, Weapon.DisplayName.IsEmpty() ? Weapon.WeaponId.ToString() : Weapon.DisplayName.ToString());
	}
}

void UReEchoLoadoutSelectionWidget::BuildWidgetTree()
{
	if (ConfirmButton || !WidgetTree)
	{
		return;
	}
	LoadOptions();
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();

	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LoadoutBackground"));
	Background->SetBrushColor(FLinearColor(0.015f, 0.02f, 0.04f, 0.96f));
	Background->SetPadding(FMargin(80.0f));
	Background->SetHorizontalAlignment(HAlign_Center);
	Background->SetVerticalAlignment(VAlign_Center);
	WidgetTree->RootWidget = Background;

	UVerticalBox* Content =
	    WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LoadoutContent"));
	Background->SetContent(Content);

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>();
	Title->SetText(FText::FromString(TEXT("选择角色与武器")));
	Title->SetColorAndOpacity(FSlateColor(FLinearColor(0.4f, 0.85f, 1.0f)));
	Title->SetJustification(ETextJustify::Center);
	FSlateFontInfo TitleFont = Title->GetFont();
	TitleFont.Size = 42;
	Title->SetFont(TitleFont);
	UVerticalBoxSlot* TitleSlot = Content->AddChildToVerticalBox(Title);
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
	UHorizontalBox* CharacterRow =
	    WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("CharacterRow"));
	UVerticalBoxSlot* CharacterSlot = Content->AddChildToVerticalBox(CharacterRow);
	CharacterSlot->SetHorizontalAlignment(HAlign_Center);
	HeartButton = AddImageOptionButton(WidgetTree,
	                                   CharacterRow,
	                                   TEXT("HeartButton"),
	                                   CharacterLabels.FindRef(CharacterOptionIds[0]),
	                                   CharacterTexturePath(Snapshot->FindCharacter(CharacterOptionIds[0])->AppearanceId));
	SpadeButton = AddImageOptionButton(WidgetTree,
	                                   CharacterRow,
	                                   TEXT("SpadeButton"),
	                                   CharacterLabels.FindRef(CharacterOptionIds[1]),
	                                   CharacterTexturePath(Snapshot->FindCharacter(CharacterOptionIds[1])->AppearanceId));
	CloverButton = AddImageOptionButton(WidgetTree,
	                                    CharacterRow,
	                                    TEXT("CloverButton"),
	                                    CharacterLabels.FindRef(CharacterOptionIds[2]),
	                                    CharacterTexturePath(Snapshot->FindCharacter(CharacterOptionIds[2])->AppearanceId));
	DiamondButton =
	    AddImageOptionButton(WidgetTree,
	                         CharacterRow,
	                         TEXT("DiamondButton"),
	                         CharacterLabels.FindRef(CharacterOptionIds[3]),
	                         CharacterTexturePath(Snapshot->FindCharacter(CharacterOptionIds[3])->AppearanceId));

	AddSectionLabel(WidgetTree, Content, TEXT("武器"));
	UHorizontalBox* WeaponRow =
	    WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("WeaponRow"));
	UVerticalBoxSlot* WeaponSlot = Content->AddChildToVerticalBox(WeaponRow);
	WeaponSlot->SetHorizontalAlignment(HAlign_Center);
	StaffButton = AddImageOptionButton(WidgetTree,
	                                   WeaponRow,
	                                   TEXT("StaffButton"),
	                                   WeaponLabels.FindRef(WeaponOptionIds[0]),
	                                   WeaponTexturePath(EReEchoWeaponSlot::PhysicalOrb));
	SwordButton = AddImageOptionButton(WidgetTree,
	                                   WeaponRow,
	                                   TEXT("SwordButton"),
	                                   WeaponLabels.FindRef(WeaponOptionIds[1]),
	                                   WeaponTexturePath(EReEchoWeaponSlot::Sword));
	ElementalButton = AddImageOptionButton(WidgetTree,
	                                       WeaponRow,
	                                       TEXT("ElementalButton"),
	                                       WeaponLabels.FindRef(WeaponOptionIds[2]),
	                                       WeaponTexturePath(EReEchoWeaponSlot::ElementalOrb));

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

void UReEchoLoadoutSelectionWidget::RefreshSelection()
{
	SetSelected(HeartButton, CharacterOptionIds.IsValidIndex(0) && SelectedCharacterId == CharacterOptionIds[0]);
	SetSelected(SpadeButton, CharacterOptionIds.IsValidIndex(1) && SelectedCharacterId == CharacterOptionIds[1]);
	SetSelected(CloverButton, CharacterOptionIds.IsValidIndex(2) && SelectedCharacterId == CharacterOptionIds[2]);
	SetSelected(DiamondButton, CharacterOptionIds.IsValidIndex(3) && SelectedCharacterId == CharacterOptionIds[3]);
	SetSelected(StaffButton, WeaponOptionIds.IsValidIndex(0) && SelectedWeaponId == WeaponOptionIds[0]);
	SetSelected(SwordButton, WeaponOptionIds.IsValidIndex(1) && SelectedWeaponId == WeaponOptionIds[1]);
	SetSelected(ElementalButton, WeaponOptionIds.IsValidIndex(2) && SelectedWeaponId == WeaponOptionIds[2]);
	if (StatusText)
	{
		const FString CharacterLabel = SelectedCharacterId.IsNone() ? TEXT("未选择") : CharacterLabels.FindRef(SelectedCharacterId);
		const FString WeaponLabel = SelectedWeaponId.IsNone() ? TEXT("未选择") : WeaponLabels.FindRef(SelectedWeaponId);
		StatusText->SetText(FText::FromString(
		    FString::Printf(TEXT("角色：%s    武器：%s"), *CharacterLabel, *WeaponLabel)));
	}
	if (ConfirmButton)
	{
		ConfirmButton->SetIsEnabled(!SelectedCharacterId.IsNone() && !SelectedWeaponId.IsNone());
		ConfirmButton->SetBackgroundColor(ConfirmButton->GetIsEnabled() ? FLinearColor(0.12f, 0.48f, 0.28f, 1.0f)
		                                                                : FLinearColor(0.16f, 0.16f, 0.16f, 1.0f));
	}
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

void UReEchoLoadoutSelectionWidget::HandleHeartClicked()
{
	SelectCharacter(CharacterOptionIds[0]);
}

void UReEchoLoadoutSelectionWidget::HandleSpadeClicked()
{
	SelectCharacter(CharacterOptionIds[1]);
}

void UReEchoLoadoutSelectionWidget::HandleCloverClicked()
{
	SelectCharacter(CharacterOptionIds[2]);
}

void UReEchoLoadoutSelectionWidget::HandleDiamondClicked()
{
	SelectCharacter(CharacterOptionIds[3]);
}

void UReEchoLoadoutSelectionWidget::HandleStaffClicked()
{
	ChooseWeapon(WeaponOptionIds[0]);
}

void UReEchoLoadoutSelectionWidget::HandleSwordClicked()
{
	ChooseWeapon(WeaponOptionIds[1]);
}

void UReEchoLoadoutSelectionWidget::HandleElementalClicked()
{
	ChooseWeapon(WeaponOptionIds[2]);
}

void UReEchoLoadoutSelectionWidget::HandleConfirmClicked()
{
	if (!SelectedCharacterId.IsNone() && !SelectedWeaponId.IsNone())
	{
		OnLoadoutConfirmed.Broadcast(SelectedCharacterId, SelectedWeaponId);
	}
}
