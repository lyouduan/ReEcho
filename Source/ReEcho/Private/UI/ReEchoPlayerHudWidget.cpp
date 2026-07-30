#include "UI/ReEchoPlayerHudWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Combat/ReEchoCombatantComponent.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"

TSharedRef<SWidget> UReEchoPlayerHudWidget::RebuildWidget()
{
	BuildWidgetTree();
	return Super::RebuildWidget();
}

void UReEchoPlayerHudWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildWidgetTree();
	Refresh();
}

void UReEchoPlayerHudWidget::InitializePlayerHud(UReEchoCombatantComponent* InCombatant, UTexture2D* InPortraitTexture)
{
	Combatant = InCombatant;
	PortraitTexture = InPortraitTexture;
	if (PortraitImage && PortraitTexture)
	{
		PortraitImage->SetBrushFromTexture(PortraitTexture, true);
	}
	Refresh();
}

void UReEchoPlayerHudWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Refresh();
}

void UReEchoPlayerHudWidget::BuildWidgetTree()
{
	if (HealthProgressBar || !WidgetTree)
	{
		return;
	}

	UCanvasPanel* RootCanvas =
	    WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("PlayerHudRoot"));
	RootCanvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	WidgetTree->RootWidget = RootCanvas;

	UBorder* PanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PlayerHudPanelBorder"));
	PanelBorder->SetBrushColor(FLinearColor(0.035f, 0.025f, 0.018f, 0.92f));
	PanelBorder->SetPadding(FMargin(7.0f));
	PanelBorder->SetVisibility(ESlateVisibility::HitTestInvisible);
	UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(PanelBorder);
	PanelSlot->SetAnchors(FAnchors(0.0f, 0.0f));
	PanelSlot->SetAlignment(FVector2D::ZeroVector);
	PanelSlot->SetPosition(FVector2D(24.0f, 24.0f));
	PanelSlot->SetAutoSize(true);

	UHorizontalBox* MainRow =
	    WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("PlayerHudMainRow"));
	PanelBorder->SetContent(MainRow);

	USizeBox* PortraitSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PlayerPortraitSize"));
	PortraitSize->SetWidthOverride(78.0f);
	PortraitSize->SetHeightOverride(78.0f);
	UHorizontalBoxSlot* PortraitSlot = MainRow->AddChildToHorizontalBox(PortraitSize);
	PortraitSlot->SetVerticalAlignment(VAlign_Center);

	UBorder* PortraitFrame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PlayerPortraitFrame"));
	PortraitFrame->SetBrushColor(FLinearColor(0.26f, 0.16f, 0.08f, 1.0f));
	PortraitFrame->SetPadding(FMargin(4.0f));
	PortraitSize->SetContent(PortraitFrame);

	PortraitImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("PlayerPortrait"));
	PortraitImage->SetColorAndOpacity(FLinearColor::White);
	PortraitFrame->SetContent(PortraitImage);
	if (PortraitTexture)
	{
		PortraitImage->SetBrushFromTexture(PortraitTexture, true);
	}

	UVerticalBox* Vitals = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PlayerVitals"));
	UHorizontalBoxSlot* VitalsSlot = MainRow->AddChildToHorizontalBox(Vitals);
	VitalsSlot->SetPadding(FMargin(8.0f, 0.0f, 0.0f, 0.0f));
	VitalsSlot->SetVerticalAlignment(VAlign_Center);

	UTextBlock* PlayerLabel =
	    WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PlayerHudLabel"));
	PlayerLabel->SetText(NSLOCTEXT("ReEcho", "PlayerHudLabel", "当前玩家"));
	PlayerLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.88f, 0.78f, 0.58f, 1.0f)));
	FSlateFontInfo LabelFont = PlayerLabel->GetFont();
	LabelFont.Size = 16;
	PlayerLabel->SetFont(LabelFont);
	UVerticalBoxSlot* LabelSlot = Vitals->AddChildToVerticalBox(PlayerLabel);
	LabelSlot->SetPadding(FMargin(3.0f, 0.0f, 0.0f, 4.0f));

	USizeBox* HealthSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PlayerHealthSize"));
	HealthSize->SetWidthOverride(300.0f);
	HealthSize->SetHeightOverride(36.0f);
	Vitals->AddChildToVerticalBox(HealthSize);

	UBorder* HealthFrame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PlayerHealthFrame"));
	HealthFrame->SetBrushColor(FLinearColor(0.31f, 0.19f, 0.08f, 1.0f));
	HealthFrame->SetPadding(FMargin(4.0f));
	HealthSize->SetContent(HealthFrame);

	UOverlay* HealthOverlay =
	    WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("PlayerHealthOverlay"));
	HealthFrame->SetContent(HealthOverlay);

	HealthProgressBar =
	    WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("PlayerHealthProgress"));
	HealthProgressBar->SetBarFillType(EProgressBarFillType::LeftToRight);
	HealthProgressBar->SetFillColorAndOpacity(FLinearColor(0.72f, 0.08f, 0.065f, 1.0f));
	HealthProgressBar->SetVisibility(ESlateVisibility::HitTestInvisible);
	UOverlaySlot* ProgressSlot = HealthOverlay->AddChildToOverlay(HealthProgressBar);
	ProgressSlot->SetHorizontalAlignment(HAlign_Fill);
	ProgressSlot->SetVerticalAlignment(VAlign_Fill);

	HealthText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PlayerHealthText"));
	HealthText->SetJustification(ETextJustify::Center);
	HealthText->SetColorAndOpacity(FSlateColor(FLinearColor(0.96f, 0.90f, 0.78f, 1.0f)));
	HealthText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f));
	HealthText->SetShadowOffset(FVector2D(1.0f, 1.0f));
	FSlateFontInfo HealthFont = HealthText->GetFont();
	HealthFont.Size = 22;
	HealthText->SetFont(HealthFont);
	UOverlaySlot* TextSlot = HealthOverlay->AddChildToOverlay(HealthText);
	TextSlot->SetHorizontalAlignment(HAlign_Fill);
	TextSlot->SetVerticalAlignment(VAlign_Center);

	Refresh();
}

void UReEchoPlayerHudWidget::Refresh()
{
	if (!HealthProgressBar || !HealthText || !Combatant.IsValid())
	{
		return;
	}

	const float MaximumHealth = FMath::Max(1.0f, Combatant->Stats.HpMax);
	const float CurrentHealth = FMath::Clamp(Combatant->CurrentHealth, 0.0f, MaximumHealth);
	HealthProgressBar->SetPercent(CurrentHealth / MaximumHealth);
	HealthText->SetText(FText::Format(NSLOCTEXT("ReEcho", "PlayerHudHealth", "{0} / {1}"),
	                                  FText::AsNumber(FMath::CeilToInt(CurrentHealth)),
	                                  FText::AsNumber(FMath::CeilToInt(MaximumHealth))));
}