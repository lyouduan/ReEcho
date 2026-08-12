#include "UI/ReEchoSettingsWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/ReEchoIndexedButton.h"
#include "UI/ReEchoMenuWidgetHelpers.h"

TSharedRef<SWidget> UReEchoSettingsWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		BuildWidgetTree();
	}
	return Super::RebuildWidget();
}

void UReEchoSettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	BuildWidgetTree();
	CategoryButtons = {GraphicsSettingsButton, AudioSettingsButton, ControlsSettingsButton};
	CategoryButtons.Remove(nullptr);
	for (int32 CategoryIndex = 0; CategoryIndex < CategoryButtons.Num(); ++CategoryIndex)
	{
		CategoryButtons[CategoryIndex]->SetEntryIndex(CategoryIndex);
		CategoryButtons[CategoryIndex]->OnIndexedClicked.AddUniqueDynamic(
		    this, &UReEchoSettingsWidget::HandleCategoryClicked);
	}
	if (RestoreDefaultsButton)
	{
		RestoreDefaultsButton->OnClicked.AddUniqueDynamic(this, &UReEchoSettingsWidget::HandleRestoreDefaultsClicked);
	}
	if (ApplyAndReturnButton)
	{
		ApplyAndReturnButton->OnClicked.AddUniqueDynamic(this, &UReEchoSettingsWidget::HandleApplyAndReturnClicked);
	}
	RefreshCategory();
	if (!CategoryButtons.IsEmpty())
	{
		CategoryButtons[0]->SetKeyboardFocus();
	}
}

void UReEchoSettingsWidget::BuildWidgetTree()
{
	if (DetailText || !WidgetTree)
	{
		return;
	}

	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SettingsBackground"));
	Background->SetBrushColor(FLinearColor(0.015f, 0.02f, 0.035f, 0.96f));
	Background->SetPadding(FMargin(120.0f));
	Background->SetHorizontalAlignment(HAlign_Center);
	Background->SetVerticalAlignment(VAlign_Center);
	WidgetTree->RootWidget = Background;

	UVerticalBox* Content =
	    WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SettingsContent"));
	Background->SetContent(Content);

	UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SettingsTitle"));
	TitleText->SetText(FText::FromString(TEXT("游戏设置")));
	TitleText->SetJustification(ETextJustify::Center);
	TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.4f, 0.85f, 1.0f)));
	FSlateFontInfo TitleFont = TitleText->GetFont();
	TitleFont.Size = 44;
	TitleText->SetFont(TitleFont);
	UVerticalBoxSlot* TitleSlot = Content->AddChildToVerticalBox(TitleText);
	TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 18.0f));

	UHorizontalBox* SettingsColumns =
	    WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SettingsColumns"));
	UVerticalBoxSlot* ColumnsSlot = Content->AddChildToVerticalBox(SettingsColumns);
	ColumnsSlot->SetHorizontalAlignment(HAlign_Fill);
	ColumnsSlot->SetVerticalAlignment(VAlign_Fill);

	UBorder* NavigationBorder =
	    WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SettingsNavigationBorder"));
	NavigationBorder->SetBrushColor(FLinearColor(0.025f, 0.04f, 0.065f, 0.98f));
	NavigationBorder->SetPadding(FMargin(18.0f));
	UHorizontalBoxSlot* NavigationSlot = SettingsColumns->AddChildToHorizontalBox(NavigationBorder);
	NavigationSlot->SetPadding(FMargin(0.0f, 0.0f, 18.0f, 0.0f));
	NavigationSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

	UVerticalBox* NavigationPanel =
	    WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SettingsNavigation"));
	NavigationBorder->SetContent(NavigationPanel);
	CategoryButtons.Reset();
	const ReEcho::UI::FMenuButtonStyle NavigationButtonStyle{
	    FLinearColor(0.12f, 0.32f, 0.48f, 1.0f), FMargin(0.0f, 5.0f), FMargin(42.0f, 12.0f), 24};
	static const TPair<FName, FString> CategoryDefinitions[] = {{TEXT("GraphicsSettingsButton"), TEXT("画面")},
	                                                            {TEXT("AudioSettingsButton"), TEXT("声音")},
	                                                            {TEXT("ControlsSettingsButton"), TEXT("操作（键位）")}};
	for (int32 CategoryIndex = 0; CategoryIndex < UE_ARRAY_COUNT(CategoryDefinitions); ++CategoryIndex)
	{
		const TPair<FName, FString>& Definition = CategoryDefinitions[CategoryIndex];
		UReEchoIndexedButton* CategoryButton = ReEcho::UI::AddIndexedMenuButton(*WidgetTree,
		                                                                        *NavigationPanel,
		                                                                        Definition.Key,
		                                                                        FText::FromString(Definition.Value),
		                                                                        CategoryIndex,
		                                                                        NavigationButtonStyle);
		CategoryButtons.Add(CategoryButton);
	}
	GraphicsSettingsButton = CategoryButtons[0];
	AudioSettingsButton = CategoryButtons[1];
	ControlsSettingsButton = CategoryButtons[2];

	UBorder* DetailBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SettingsDetailBorder"));
	DetailBorder->SetBrushColor(FLinearColor(0.035f, 0.05f, 0.075f, 0.98f));
	DetailBorder->SetPadding(FMargin(32.0f));
	UHorizontalBoxSlot* DetailColumnSlot = SettingsColumns->AddChildToHorizontalBox(DetailBorder);
	DetailColumnSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	UVerticalBox* DetailContent =
	    WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SettingsDetailContent"));
	DetailBorder->SetContent(DetailContent);

	CategoryTitleText =
	    WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SettingsCategoryTitle"));
	CategoryTitleText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	FSlateFontInfo CategoryTitleFont = CategoryTitleText->GetFont();
	CategoryTitleFont.Size = 30;
	CategoryTitleText->SetFont(CategoryTitleFont);
	UVerticalBoxSlot* CategoryTitleSlot = DetailContent->AddChildToVerticalBox(CategoryTitleText);
	CategoryTitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 20.0f));

	DetailText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SettingsDetail"));
	DetailText->SetJustification(ETextJustify::Left);
	DetailText->SetColorAndOpacity(FSlateColor(FLinearColor(0.82f, 0.86f, 0.92f)));
	FSlateFontInfo DetailFont = DetailText->GetFont();
	DetailFont.Size = 20;
	DetailText->SetFont(DetailFont);
	UVerticalBoxSlot* DetailSlot = DetailContent->AddChildToVerticalBox(DetailText);
	DetailSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	DetailSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 24.0f));

	ReEcho::UI::FMenuButtonStyle ActionButtonStyle{
	    FLinearColor(0.24f, 0.24f, 0.28f, 1.0f), FMargin(0.0f, 5.0f), FMargin(42.0f, 12.0f), 24};
	RestoreDefaultsButton = ReEcho::UI::AddMenuButton(*WidgetTree,
	                                                  *DetailContent,
	                                                  TEXT("RestoreDefaultsButton"),
	                                                  FText::FromString(TEXT("恢复默认")),
	                                                  ActionButtonStyle);
	ActionButtonStyle.Color = FLinearColor(0.08f, 0.42f, 0.32f, 1.0f);
	ApplyAndReturnButton = ReEcho::UI::AddMenuButton(*WidgetTree,
	                                                 *DetailContent,
	                                                 TEXT("ApplyAndReturnButton"),
	                                                 FText::FromString(TEXT("应用并返回")),
	                                                 ActionButtonStyle);
}

void UReEchoSettingsWidget::RefreshCategory()
{
	if (!DetailText || !CategoryTitleText)
	{
		return;
	}

	const FString CategoryTitle = SelectedCategory == 1   ? TEXT("声音设置")
	                              : SelectedCategory == 2 ? TEXT("操作（键位）")
	                                                      : TEXT("画面设置");
	const FString Detail = SelectedCategory == 1   ? TEXT("音量等具体选项暂未接入")
	                       : SelectedCategory == 2 ? TEXT("按键重绑定等具体选项暂未接入")
	                                               : TEXT("分辨率、显示模式、画质等具体选项暂未接入");
	CategoryTitleText->SetText(FText::FromString(CategoryTitle));
	DetailText->SetText(FText::FromString(Detail));

	const FLinearColor SettingsSelectedColor(0.08f, 0.42f, 0.62f, 1.0f);
	const FLinearColor SettingsNormalColor(0.12f, 0.22f, 0.32f, 1.0f);
	for (int32 CategoryIndex = 0; CategoryIndex < CategoryButtons.Num(); ++CategoryIndex)
	{
		CategoryButtons[CategoryIndex]->SetBackgroundColor(SelectedCategory == CategoryIndex ? SettingsSelectedColor
		                                                                                     : SettingsNormalColor);
	}
}

void UReEchoSettingsWidget::HandleCategoryClicked(const int32 CategoryIndex)
{
	if (!CategoryButtons.IsValidIndex(CategoryIndex))
	{
		return;
	}

	SelectedCategory = CategoryIndex;
	RefreshCategory();
}

void UReEchoSettingsWidget::HandleRestoreDefaultsClicked()
{
	RefreshCategory();
}

void UReEchoSettingsWidget::HandleApplyAndReturnClicked()
{
	OnClosed.Broadcast();
}
