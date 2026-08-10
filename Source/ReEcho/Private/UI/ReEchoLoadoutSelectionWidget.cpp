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
#include "Engine/Texture2D.h"

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
    UWidgetTree* WidgetTree, UHorizontalBox* Row, const FName Name, const FString& Label, const TCHAR* TexturePath)
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
	if (UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, TexturePath))
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

FString CharacterLabel(const FName CharacterId)
{
	if (CharacterId == TEXT("J_HEART"))
	{
		return TEXT("红心");
	}
	if (CharacterId == TEXT("J_SPADE"))
	{
		return TEXT("黑桃");
	}
	if (CharacterId == TEXT("J_CLOVER"))
	{
		return TEXT("梅花");
	}
	if (CharacterId == TEXT("J_DIAMOND"))
	{
		return TEXT("方片");
	}
	return TEXT("未选择");
}

FString WeaponLabel(const FName WeaponId)
{
	if (WeaponId == TEXT("W_J_02"))
	{
		return TEXT("月光法杖");
	}
	if (WeaponId == TEXT("W_J_01"))
	{
		return TEXT("新月刃");
	}
	if (WeaponId == TEXT("W_J_03"))
	{
		return TEXT("元素回响");
	}
	return TEXT("未选择");
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
	Hint->SetText(FText::FromString(TEXT("本局确认后不可更换武器")));
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
	                                   TEXT("红心"),
	                                   TEXT("/Game/ReEcho/Textures/Characters/NewCast/Player_Heart.Player_Heart"));
	SpadeButton = AddImageOptionButton(WidgetTree,
	                                   CharacterRow,
	                                   TEXT("SpadeButton"),
	                                   TEXT("黑桃"),
	                                   TEXT("/Game/ReEcho/Textures/Characters/NewCast/Player_Spade.Player_Spade"));
	CloverButton = AddImageOptionButton(WidgetTree,
	                                    CharacterRow,
	                                    TEXT("CloverButton"),
	                                    TEXT("梅花"),
	                                    TEXT("/Game/ReEcho/Textures/Characters/NewCast/Player_Clover.Player_Clover"));
	DiamondButton =
	    AddImageOptionButton(WidgetTree,
	                         CharacterRow,
	                         TEXT("DiamondButton"),
	                         TEXT("方片"),
	                         TEXT("/Game/ReEcho/Textures/Characters/NewCast/Player_Diamond.Player_Diamond"));

	AddSectionLabel(WidgetTree, Content, TEXT("武器"));
	UHorizontalBox* WeaponRow =
	    WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("WeaponRow"));
	UVerticalBoxSlot* WeaponSlot = Content->AddChildToVerticalBox(WeaponRow);
	WeaponSlot->SetHorizontalAlignment(HAlign_Center);
	StaffButton = AddImageOptionButton(WidgetTree,
	                                   WeaponRow,
	                                   TEXT("StaffButton"),
	                                   TEXT("月光法杖"),
	                                   TEXT("/Game/ReEcho/Textures/Effects/MoonStaff.MoonStaff"));
	SwordButton = AddImageOptionButton(WidgetTree,
	                                   WeaponRow,
	                                   TEXT("SwordButton"),
	                                   TEXT("新月刃"),
	                                   TEXT("/Game/ReEcho/Textures/Effects/CrescentWeapon.CrescentWeapon"));
	ElementalButton = AddImageOptionButton(WidgetTree,
	                                       WeaponRow,
	                                       TEXT("ElementalButton"),
	                                       TEXT("元素回响"),
	                                       TEXT("/Game/ReEcho/Textures/Effects/StaffLightWave.StaffLightWave"));

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
	SetSelected(HeartButton, SelectedCharacterId == TEXT("J_HEART"));
	SetSelected(SpadeButton, SelectedCharacterId == TEXT("J_SPADE"));
	SetSelected(CloverButton, SelectedCharacterId == TEXT("J_CLOVER"));
	SetSelected(DiamondButton, SelectedCharacterId == TEXT("J_DIAMOND"));
	SetSelected(StaffButton, SelectedWeaponId == TEXT("W_J_02"));
	SetSelected(SwordButton, SelectedWeaponId == TEXT("W_J_01"));
	SetSelected(ElementalButton, SelectedWeaponId == TEXT("W_J_03"));
	if (StatusText)
	{
		StatusText->SetText(FText::FromString(FString::Printf(
		    TEXT("角色：%s    武器：%s"), *CharacterLabel(SelectedCharacterId), *WeaponLabel(SelectedWeaponId))));
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
	SelectCharacter(TEXT("J_HEART"));
}

void UReEchoLoadoutSelectionWidget::HandleSpadeClicked()
{
	SelectCharacter(TEXT("J_SPADE"));
}

void UReEchoLoadoutSelectionWidget::HandleCloverClicked()
{
	SelectCharacter(TEXT("J_CLOVER"));
}

void UReEchoLoadoutSelectionWidget::HandleDiamondClicked()
{
	SelectCharacter(TEXT("J_DIAMOND"));
}

void UReEchoLoadoutSelectionWidget::HandleStaffClicked()
{
	ChooseWeapon(TEXT("W_J_02"));
}

void UReEchoLoadoutSelectionWidget::HandleSwordClicked()
{
	ChooseWeapon(TEXT("W_J_01"));
}

void UReEchoLoadoutSelectionWidget::HandleElementalClicked()
{
	ChooseWeapon(TEXT("W_J_03"));
}

void UReEchoLoadoutSelectionWidget::HandleConfirmClicked()
{
	if (!SelectedCharacterId.IsNone() && !SelectedWeaponId.IsNone())
	{
		OnLoadoutConfirmed.Broadcast(SelectedCharacterId, SelectedWeaponId);
	}
}
