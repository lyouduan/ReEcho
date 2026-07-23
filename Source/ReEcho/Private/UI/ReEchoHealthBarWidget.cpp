#include "UI/ReEchoHealthBarWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Combat/ReEchoCombatantComponent.h"
#include "Components/ProgressBar.h"

TSharedRef<SWidget> UReEchoHealthBarWidget::RebuildWidget()
{
	BuildWidgetTree();
	return Super::RebuildWidget();
}

void UReEchoHealthBarWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildWidgetTree();
	Refresh();
}

void UReEchoHealthBarWidget::InitializeHealth(
	UReEchoCombatantComponent* InCombatant,
	const FLinearColor& InFillColor)
{
	Combatant = InCombatant;
	FillColor = InFillColor;

	if (ProgressBar)
	{
		ProgressBar->SetFillColorAndOpacity(FillColor);
	}

	Refresh();
}

void UReEchoHealthBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Refresh();
}

void UReEchoHealthBarWidget::BuildWidgetTree()
{
	if (ProgressBar || !WidgetTree)
	{
		return;
	}

	ProgressBar = WidgetTree->ConstructWidget<UProgressBar>(
		UProgressBar::StaticClass(),
		TEXT("HealthProgress"));
	WidgetTree->RootWidget = ProgressBar;
	ProgressBar->SetBarFillType(EProgressBarFillType::LeftToRight);
	ProgressBar->SetFillColorAndOpacity(FillColor);
	ProgressBar->SetIsEnabled(true);
	ProgressBar->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UReEchoHealthBarWidget::Refresh()
{
	if (!ProgressBar || !Combatant.IsValid())
	{
		return;
	}

	const float MaximumHealth = FMath::Max(1.0f, Combatant->Stats.HpMax);
	const float HealthPercent = FMath::Clamp(
		Combatant->CurrentHealth / MaximumHealth,
		0.0f,
		1.0f);
	ProgressBar->SetPercent(HealthPercent);
}