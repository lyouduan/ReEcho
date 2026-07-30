#include "UI/ReEchoPlayerHudWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Combat/ReEchoCombatantComponent.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Styling/SlateBrush.h"

namespace
{
FSlateBrush MakeRoundedBrush(const FLinearColor& FillColor,
                             const float Radius,
                             const FLinearColor& OutlineColor,
                             const float OutlineWidth)
{
	FSlateBrush Brush;
	Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
	Brush.TintColor = FSlateColor(FillColor);
	Brush.OutlineSettings = FSlateBrushOutlineSettings(Radius, FSlateColor(OutlineColor), OutlineWidth);
	return Brush;
}
} // namespace

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

	USizeBox* HudSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PlayerHudSize"));
	HudSize->SetWidthOverride(404.0f);
	HudSize->SetHeightOverride(94.0f);
	HudSize->SetVisibility(ESlateVisibility::HitTestInvisible);
	UCanvasPanelSlot* HudSlot = RootCanvas->AddChildToCanvas(HudSize);
	HudSlot->SetAnchors(FAnchors(0.0f, 0.0f));
	HudSlot->SetAlignment(FVector2D::ZeroVector);
	HudSlot->SetPosition(FVector2D(20.0f, 18.0f));
	HudSlot->SetAutoSize(true);

	UCanvasPanel* HudCanvas =
	    WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("PlayerHudCanvas"));
	HudCanvas->SetVisibility(ESlateVisibility::HitTestInvisible);
	HudSize->SetContent(HudCanvas);

	USizeBox* HealthSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PlayerHealthSize"));
	HealthSize->SetWidthOverride(334.0f);
	HealthSize->SetHeightOverride(50.0f);
	UCanvasPanelSlot* HealthSlot = HudCanvas->AddChildToCanvas(HealthSize);
	HealthSlot->SetPosition(FVector2D(67.0f, 13.0f));
	HealthSlot->SetSize(FVector2D(334.0f, 50.0f));

	UBorder* HealthFrame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PlayerHealthFrame"));
	HealthFrame->SetBrush(MakeRoundedBrush(
	    FLinearColor(0.055f, 0.035f, 0.025f, 0.94f), 12.0f, FLinearColor(0.34f, 0.22f, 0.10f, 1.0f), 3.0f));
	HealthFrame->SetPadding(FMargin(5.0f));
	HealthSize->SetContent(HealthFrame);

	UOverlay* HealthOverlay =
	    WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("PlayerHealthOverlay"));
	HealthFrame->SetContent(HealthOverlay);

	HealthProgressBar =
	    WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("PlayerHealthProgress"));
	HealthProgressBar->SetBarFillType(EProgressBarFillType::LeftToRight);
	HealthProgressBar->SetFillColorAndOpacity(FLinearColor(0.76f, 0.075f, 0.055f, 1.0f));
	HealthProgressBar->SetVisibility(ESlateVisibility::HitTestInvisible);
	UOverlaySlot* ProgressSlot = HealthOverlay->AddChildToOverlay(HealthProgressBar);
	ProgressSlot->SetHorizontalAlignment(HAlign_Fill);
	ProgressSlot->SetVerticalAlignment(VAlign_Fill);

	HealthText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PlayerHealthText"));
	HealthText->SetJustification(ETextJustify::Center);
	HealthText->SetColorAndOpacity(FSlateColor(FLinearColor(0.96f, 0.87f, 0.72f, 1.0f)));
	HealthText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f));
	HealthText->SetShadowOffset(FVector2D(2.0f, 2.0f));
	FSlateFontInfo HealthFont = HealthText->GetFont();
	HealthFont.Size = 27;
	HealthText->SetFont(HealthFont);
	UOverlaySlot* TextSlot = HealthOverlay->AddChildToOverlay(HealthText);
	TextSlot->SetHorizontalAlignment(HAlign_Fill);
	TextSlot->SetVerticalAlignment(VAlign_Center);

	USizeBox* PortraitSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PlayerPortraitSize"));
	PortraitSize->SetWidthOverride(90.0f);
	PortraitSize->SetHeightOverride(90.0f);
	UCanvasPanelSlot* PortraitSlot = HudCanvas->AddChildToCanvas(PortraitSize);
	PortraitSlot->SetPosition(FVector2D::ZeroVector);
	PortraitSlot->SetSize(FVector2D(90.0f, 90.0f));
	PortraitSlot->SetZOrder(2);

	UBorder* PortraitFrame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PlayerPortraitFrame"));
	PortraitFrame->SetBrush(MakeRoundedBrush(
	    FLinearColor(0.025f, 0.020f, 0.018f, 0.98f), 45.0f, FLinearColor(0.40f, 0.27f, 0.12f, 1.0f), 4.0f));
	PortraitFrame->SetPadding(FMargin(6.0f));
	PortraitSize->SetContent(PortraitFrame);

	PortraitImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("PlayerPortrait"));
	PortraitImage->SetColorAndOpacity(FLinearColor::White);
	PortraitFrame->SetContent(PortraitImage);
	if (PortraitTexture)
	{
		PortraitImage->SetBrushFromTexture(PortraitTexture, true);
	}

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
	HealthText->SetText(FText::Format(NSLOCTEXT("ReEcho", "PlayerHudHealth", "{0}/{1}"),
	                                  FText::AsNumber(FMath::CeilToInt(CurrentHealth)),
	                                  FText::AsNumber(FMath::CeilToInt(MaximumHealth))));
}