#include "UI/ReEchoPlayerHudWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Combat/ReEchoCombatContracts.h"
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
#include "UI/ReEchoPlayerScreenFeedbackWidget.h"
#include "UObject/ConstructorHelpers.h"

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

UReEchoPlayerHudWidget::UReEchoPlayerHudWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	static ConstructorHelpers::FClassFinder<UReEchoPlayerScreenFeedbackWidget> FeedbackWidgetFinder(
	    TEXT("/Game/ReEcho/UI/WBP_ReEchoPlayerScreenFeedback"));
	PlayerScreenFeedbackClass = FeedbackWidgetFinder.Class;
	if (!PlayerScreenFeedbackClass)
	{
		PlayerScreenFeedbackClass = UReEchoPlayerScreenFeedbackWidget::StaticClass();
	}
}

TSharedRef<SWidget> UReEchoPlayerHudWidget::RebuildWidget()
{
	if (GetClass() == StaticClass())
	{
		BuildWidgetTree();
	}
	return Super::RebuildWidget();
}

void UReEchoPlayerHudWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildWidgetTree();
	EnsureScreenFeedbackWidget();
	Refresh();
}

void UReEchoPlayerHudWidget::InitializePlayerHud(UReEchoCombatantComponent* InCombatant,
                                                 UReEchoCombatEventsComponent* InCombatEvents,
                                                 UTexture2D* InPortraitTexture)
{
	BindCombatant(InCombatant);
	BindCombatEvents(InCombatEvents);
	EnsureScreenFeedbackWidget();
	PortraitTexture = InPortraitTexture;
	if (PlayerPortrait && PortraitTexture)
	{
		PlayerPortrait->SetBrushFromTexture(PortraitTexture, true);
	}
	Refresh();
}

void UReEchoPlayerHudWidget::SetTimeShards(const int32 InTimeShards)
{
	CurrentTimeShards = InTimeShards;
	if (TimeShardText)
	{
		TimeShardText->SetText(FText::AsNumber(CurrentTimeShards));
	}
}

void UReEchoPlayerHudWidget::NativeDestruct()
{
	BindCombatant(nullptr);
	BindCombatEvents(nullptr);
	Super::NativeDestruct();
}

void UReEchoPlayerHudWidget::BuildWidgetTree()
{
	if (PlayerHealthProgress || !WidgetTree)
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

	PlayerHealthProgress =
	    WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("PlayerHealthProgress"));
	PlayerHealthProgress->SetBarFillType(EProgressBarFillType::LeftToRight);
	PlayerHealthProgress->SetFillColorAndOpacity(FLinearColor(0.76f, 0.075f, 0.055f, 1.0f));
	PlayerHealthProgress->SetVisibility(ESlateVisibility::HitTestInvisible);
	UOverlaySlot* ProgressSlot = HealthOverlay->AddChildToOverlay(PlayerHealthProgress);
	ProgressSlot->SetHorizontalAlignment(HAlign_Fill);
	ProgressSlot->SetVerticalAlignment(VAlign_Fill);

	PlayerHealthText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PlayerHealthText"));
	PlayerHealthText->SetJustification(ETextJustify::Center);
	PlayerHealthText->SetColorAndOpacity(FSlateColor(FLinearColor(0.96f, 0.87f, 0.72f, 1.0f)));
	PlayerHealthText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f));
	PlayerHealthText->SetShadowOffset(FVector2D(2.0f, 2.0f));
	FSlateFontInfo HealthFont = PlayerHealthText->GetFont();
	HealthFont.Size = 27;
	PlayerHealthText->SetFont(HealthFont);
	UOverlaySlot* TextSlot = HealthOverlay->AddChildToOverlay(PlayerHealthText);
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

	PlayerPortrait = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("PlayerPortrait"));
	PlayerPortrait->SetColorAndOpacity(FLinearColor::White);
	PortraitFrame->SetContent(PlayerPortrait);
	if (PortraitTexture)
	{
		PlayerPortrait->SetBrushFromTexture(PortraitTexture, true);
	}

	Refresh();
}

void UReEchoPlayerHudWidget::EnsureScreenFeedbackWidget()
{
	if (PlayerScreenFeedback || !WidgetTree)
	{
		return;
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		return;
	}

	TSubclassOf<UReEchoPlayerScreenFeedbackWidget> FeedbackClass = PlayerScreenFeedbackClass;
	if (!FeedbackClass)
	{
		FeedbackClass = UReEchoPlayerScreenFeedbackWidget::StaticClass();
	}
	PlayerScreenFeedback =
	    WidgetTree->ConstructWidget<UReEchoPlayerScreenFeedbackWidget>(FeedbackClass, TEXT("PlayerScreenFeedback"));
	PlayerScreenFeedback->SetVisibility(ESlateVisibility::HitTestInvisible);
	UCanvasPanelSlot* FeedbackSlot = RootCanvas->AddChildToCanvas(PlayerScreenFeedback);
	FeedbackSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	FeedbackSlot->SetOffsets(FMargin(0.0f));
	FeedbackSlot->SetZOrder(-100);
}

void UReEchoPlayerHudWidget::Refresh()
{
	SetTimeShards(CurrentTimeShards);
	if (!Combatant.IsValid())
	{
		return;
	}

	HandleHealthChanged(Combatant->CurrentHealth, Combatant->Stats.HpMax);
}

void UReEchoPlayerHudWidget::BindCombatant(UReEchoCombatantComponent* InCombatant)
{
	if (Combatant.IsValid())
	{
		Combatant->OnHealthChanged.RemoveDynamic(this, &UReEchoPlayerHudWidget::HandleHealthChanged);
	}
	Combatant = InCombatant;
	if (Combatant.IsValid())
	{
		Combatant->OnHealthChanged.AddUniqueDynamic(this, &UReEchoPlayerHudWidget::HandleHealthChanged);
	}
}

void UReEchoPlayerHudWidget::BindCombatEvents(UReEchoCombatEventsComponent* InCombatEvents)
{
	if (CombatEvents.IsValid())
	{
		CombatEvents->OnHurt.RemoveDynamic(this, &UReEchoPlayerHudWidget::HandlePlayerHurt);
	}
	CombatEvents = InCombatEvents;
	if (CombatEvents.IsValid())
	{
		CombatEvents->OnHurt.AddUniqueDynamic(this, &UReEchoPlayerHudWidget::HandlePlayerHurt);
	}
}

void UReEchoPlayerHudWidget::HandleHealthChanged(const float CurrentHealth, const float MaximumHealth)
{
	const float SafeMaximumHealth = FMath::Max(1.0f, MaximumHealth);
	const float ClampedHealth = FMath::Clamp(CurrentHealth, 0.0f, SafeMaximumHealth);
	if (PlayerScreenFeedback)
	{
		PlayerScreenFeedback->SetHealth(ClampedHealth, SafeMaximumHealth);
	}

	const float HealthRatio = ClampedHealth / SafeMaximumHealth;
	if (PlayerHealthProgress)
	{
		PlayerHealthProgress->SetPercent(HealthRatio);
	}
	if (PlayerHealthFill)
	{
		PlayerHealthFill->SetRenderScale(FVector2D(HealthRatio, 1.0f));
	}
	if (PlayerHealthText)
	{
		PlayerHealthText->SetText(FText::Format(NSLOCTEXT("ReEcho", "PlayerHudHealth", "{0}/{1}"),
		                                        FText::AsNumber(FMath::CeilToInt(ClampedHealth)),
		                                        FText::AsNumber(FMath::CeilToInt(SafeMaximumHealth))));
	}
}

void UReEchoPlayerHudWidget::HandlePlayerHurt(const FReEchoDamageEvent& Event)
{
	if (!PlayerScreenFeedback || !Combatant.IsValid() || Combatant->CurrentHealth <= 0.0f ||
	    !UReEchoPlayerScreenFeedbackWidget::ShouldPlayHurtFeedback(Event.AppliedDamage, Event.bBlocked))
	{
		return;
	}

	PlayerScreenFeedback->PlayHurtFeedback(Event.AppliedDamage, Combatant->Stats.HpMax);
}

#if WITH_DEV_AUTOMATION_TESTS
bool UReEchoPlayerHudWidget::IsBoundToCombatEventsForTests(const UReEchoCombatEventsComponent* InCombatEvents) const
{
	return InCombatEvents && InCombatEvents->OnHurt.IsAlreadyBound(this, &UReEchoPlayerHudWidget::HandlePlayerHurt);
}

UClass* UReEchoPlayerHudWidget::GetScreenFeedbackClassForTests() const
{
	return PlayerScreenFeedbackClass.Get();
}

int32 UReEchoPlayerHudWidget::GetTimeShardsForTests() const
{
	return CurrentTimeShards;
}
#endif
