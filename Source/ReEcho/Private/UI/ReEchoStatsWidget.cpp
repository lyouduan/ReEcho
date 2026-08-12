#include "UI/ReEchoStatsWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
UTextBlock* CreateStatsText(UWidgetTree* WidgetTree, const FName Name, const int32 FontSize)
{
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
	Text->SetColorAndOpacity(FSlateColor(FLinearColor(0.83f, 0.9f, 1.0f, 1.0f)));
	Text->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f));
	Text->SetShadowOffset(FVector2D(2.0f, 2.0f));
	Text->SetAutoWrapText(true);
	FSlateFontInfo Font = Text->GetFont();
	Font.Size = FontSize;
	Text->SetFont(Font);
	return Text;
}
}

UReEchoStatsWidget::UReEchoStatsWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UTexture2D> BackgroundFinder(
	    TEXT("/Game/ReEcho/Textures/UI/StatsBackground.StatsBackground"));
	BackgroundTexture = BackgroundFinder.Object;
}

TSharedRef<SWidget> UReEchoStatsWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		BuildWidgetTree();
	}
	return Super::RebuildWidget();
}

void UReEchoStatsWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildWidgetTree();
	if (CloseButton)
	{
		CloseButton->OnClicked.AddUniqueDynamic(this, &UReEchoStatsWidget::HandleCloseClicked);
	}
	Refresh();
}

void UReEchoStatsWidget::InitializeStats(const FReEchoStatBlock& PlayerStats,
                                         const float PlayerCurrentHealth,
                                         const FReEchoStatBlock& EchoStats,
                                         const float EchoCurrentHealth,
                                         const bool bHasEcho)
{
	CurrentPlayerStats = PlayerStats;
	CurrentPlayerHealth = PlayerCurrentHealth;
	CurrentEchoStats = EchoStats;
	CurrentEchoHealth = EchoCurrentHealth;
	bCurrentHasEcho = bHasEcho;
	Refresh();
}

void UReEchoStatsWidget::BuildWidgetTree()
{
	if (!WidgetTree || (WidgetTree->RootWidget && BackgroundImage && PlayerStatsText && EchoStatsText && CloseButton))
	{
		return;
	}

	UCanvasPanel* RootCanvas =
	    WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("StatsRoot"));
	WidgetTree->RootWidget = RootCanvas;

	BackgroundImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("StatsBackground"));
	BackgroundImage->SetBrushFromTexture(BackgroundTexture, true);
	UCanvasPanelSlot* BackgroundSlot = RootCanvas->AddChildToCanvas(BackgroundImage);
	BackgroundSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	BackgroundSlot->SetOffsets(FMargin(0.0f));

	UTextBlock* Title = CreateStatsText(WidgetTree, TEXT("StatsTitle"), 38);
	Title->SetText(NSLOCTEXT("ReEcho", "StatsPanelTitle", "时间映像 · 属性"));
	Title->SetJustification(ETextJustify::Center);
	UCanvasPanelSlot* TitleSlot = RootCanvas->AddChildToCanvas(Title);
	TitleSlot->SetAnchors(FAnchors(0.25f, 0.06f, 0.75f, 0.15f));
	TitleSlot->SetOffsets(FMargin(0.0f));

	UHorizontalBox* Columns =
	    WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("StatsColumns"));
	UCanvasPanelSlot* ColumnsSlot = RootCanvas->AddChildToCanvas(Columns);
	ColumnsSlot->SetAnchors(FAnchors(0.12f, 0.2f, 0.88f, 0.83f));
	ColumnsSlot->SetOffsets(FMargin(0.0f));

	auto AddColumn = [&](const FName BorderName, const FText& Heading, UTextBlock*& OutStatsText)
	{
		UBorder* Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), BorderName);
		Border->SetBrushColor(FLinearColor(0.015f, 0.025f, 0.06f, 0.72f));
		Border->SetPadding(FMargin(36.0f, 26.0f));
		UHorizontalBoxSlot* BorderSlot = Columns->AddChildToHorizontalBox(Border);
		BorderSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		BorderSlot->SetPadding(FMargin(18.0f));

		UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(
		    UVerticalBox::StaticClass(), *FString(BorderName.ToString() + TEXT("Content")));
		Border->SetContent(Content);

		UTextBlock* HeadingText = CreateStatsText(WidgetTree, *FString(BorderName.ToString() + TEXT("Heading")), 30);
		HeadingText->SetText(Heading);
		HeadingText->SetJustification(ETextJustify::Center);
		UVerticalBoxSlot* HeadingSlot = Content->AddChildToVerticalBox(HeadingText);
		HeadingSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 24.0f));

		OutStatsText = CreateStatsText(WidgetTree, *FString(BorderName.ToString() + TEXT("Stats")), 22);
		UVerticalBoxSlot* StatsSlot = Content->AddChildToVerticalBox(OutStatsText);
		StatsSlot->SetPadding(FMargin(18.0f));
	};

	UTextBlock* PlayerStatsRaw = nullptr;
	AddColumn(TEXT("PlayerColumn"), NSLOCTEXT("ReEcho", "PlayerStatsHeading", "当前玩家"), PlayerStatsRaw);
	PlayerStatsText = PlayerStatsRaw;
	UTextBlock* EchoStatsRaw = nullptr;
	AddColumn(TEXT("EchoColumn"), NSLOCTEXT("ReEcho", "EchoStatsHeading", "当前回响"), EchoStatsRaw);
	EchoStatsText = EchoStatsRaw;

	CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("StatsClose"));
	CloseButton->SetBackgroundColor(FLinearColor(0.04f, 0.08f, 0.15f, 0.9f));
	UCanvasPanelSlot* CloseSlot = RootCanvas->AddChildToCanvas(CloseButton);
	CloseSlot->SetAnchors(FAnchors(0.42f, 0.87f, 0.58f, 0.95f));
	CloseSlot->SetOffsets(FMargin(0.0f));
	UTextBlock* CloseText = CreateStatsText(WidgetTree, TEXT("StatsCloseText"), 22);
	CloseText->SetText(NSLOCTEXT("ReEcho", "StatsCloseLabel", "Tab / 返回"));
	CloseText->SetJustification(ETextJustify::Center);
	CloseButton->SetContent(CloseText);
}

FText UReEchoStatsWidget::FormatStats(const FReEchoStatBlock& Stats, const float CurrentHealth) const
{
	return FText::Format(NSLOCTEXT("ReEcho",
	                               "StatsFormat",
	                               "生命        {0} / {1}\n\n物理攻击    {2}\n元素攻击    {3}\n格挡次数    "
	                               "{4}\n攻击速度    {5}%\n移动速度    {6}%\n暴击率      {7}%\n回响效率    {8}%"),
	                     FText::AsNumber(FMath::CeilToInt(CurrentHealth)),
	                     FText::AsNumber(FMath::CeilToInt(Stats.HpMax)),
	                     FText::AsNumber(FMath::RoundToInt(Stats.PhysicalAttack)),
	                     FText::AsNumber(FMath::RoundToInt(Stats.ElementalAttack)),
	                     FText::AsNumber(Stats.Block),
	                     FText::AsNumber(FMath::RoundToInt(Stats.AttackSpeed * 100.0f)),
	                     FText::AsNumber(FMath::RoundToInt(Stats.MovementSpeed * 100.0f)),
	                     FText::AsNumber(FMath::RoundToInt(Stats.CriticalRate * 100.0f)),
	                     FText::AsNumber(FMath::RoundToInt(Stats.EchoEfficiency * 100.0f)));
}

void UReEchoStatsWidget::Refresh()
{
	if (!PlayerStatsText || !EchoStatsText)
	{
		return;
	}

	PlayerStatsText->SetText(FormatStats(CurrentPlayerStats, CurrentPlayerHealth));
	EchoStatsText->SetText(bCurrentHasEcho
	                           ? FormatStats(CurrentEchoStats, CurrentEchoHealth)
	                           : NSLOCTEXT("ReEcho",
	                                       "NoActiveEchoStats",
	                                       "尚未生成回响。\n\n完成第一场遭遇后，\n这里会显示上一时间线的实际属性。"));
}

void UReEchoStatsWidget::HandleCloseClicked()
{
	OnClosed.Broadcast();
}
