#include "UI/ReEchoBossPhase3AnnouncementWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"

namespace
{
constexpr float AnnouncementHoldSeconds = 0.75f;
constexpr float AnnouncementFadeSeconds = 1.25f;
}

TSharedRef<SWidget> UReEchoBossPhase3AnnouncementWidget::RebuildWidget()
{
	BuildWidgetTree();
	return Super::RebuildWidget();
}

void UReEchoBossPhase3AnnouncementWidget::BuildWidgetTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Phase3AnnouncementRoot"));
	Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	WidgetTree->RootWidget = Root;

	AnnouncementText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Phase3AnnouncementText"));
	AnnouncementText->SetText(NSLOCTEXT("ReEcho", "BossPhase3Announcement", "彩蛋环节\n派对时刻，全属性翻倍"));
	AnnouncementText->SetJustification(ETextJustify::Center);
	AnnouncementText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.82f, 0.30f, 1.0f)));
	AnnouncementText->SetShadowColorAndOpacity(FLinearColor(0.05f, 0.01f, 0.08f, 0.9f));
	AnnouncementText->SetShadowOffset(FVector2D(3.0f, 3.0f));
	FSlateFontInfo Font = AnnouncementText->GetFont();
	Font.Size = 84;
	Font.OutlineSettings.OutlineSize = 3;
	Font.OutlineSettings.OutlineColor = FLinearColor(0.12f, 0.025f, 0.16f, 1.0f);
	AnnouncementText->SetFont(Font);
	UCanvasPanelSlot* TextSlot = Root->AddChildToCanvas(AnnouncementText);
	TextSlot->SetAnchors(FAnchors(0.5f, 0.38f));
	TextSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	TextSlot->SetAutoSize(true);
}

void UReEchoBossPhase3AnnouncementWidget::ShowAnnouncement()
{
	BuildWidgetTree();
	ElapsedSeconds = 0.0f;
	bPlaying = true;
	SetRenderOpacity(1.0f);
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

float UReEchoBossPhase3AnnouncementWidget::CalculateAnnouncementOpacity(const float ElapsedSeconds,
	                                                                     const float HoldSeconds,
	                                                                     const float FadeSeconds)
{
	const float SafeElapsed = FMath::Max(0.0f, ElapsedSeconds);
	const float SafeHold = FMath::Max(0.0f, HoldSeconds);
	const float SafeFade = FMath::Max(FadeSeconds, UE_SMALL_NUMBER);
	if (SafeElapsed <= SafeHold)
	{
		return 1.0f;
	}
	return 1.0f - FMath::Clamp((SafeElapsed - SafeHold) / SafeFade, 0.0f, 1.0f);
}

void UReEchoBossPhase3AnnouncementWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!bPlaying)
	{
		return;
	}
	ElapsedSeconds += FMath::Max(0.0f, InDeltaTime);
	SetRenderOpacity(CalculateAnnouncementOpacity(ElapsedSeconds, AnnouncementHoldSeconds, AnnouncementFadeSeconds));
	if (ElapsedSeconds >= AnnouncementHoldSeconds + AnnouncementFadeSeconds)
	{
		bPlaying = false;
		RemoveFromParent();
	}
}
