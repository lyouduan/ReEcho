#include "UI/ReEchoHealthBarWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Combat/ReEchoCombatantComponent.h"
#include "Components/ProgressBar.h"

TSharedRef<SWidget> UReEchoHealthBarWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		BuildWidgetTree();
	}
	return Super::RebuildWidget();
}

void UReEchoHealthBarWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildWidgetTree();
	Refresh();
}

void UReEchoHealthBarWidget::InitializeHealth(UReEchoCombatantComponent* InCombatant, const FLinearColor& InFillColor)
{
	BindCombatant(InCombatant);
	FillColor = InFillColor;

	if (ProgressBar)
	{
		ProgressBar->SetFillColorAndOpacity(FillColor);
	}

	Refresh();
}

void UReEchoHealthBarWidget::NativeDestruct()
{
	BindCombatant(nullptr);
	Super::NativeDestruct();
}

void UReEchoHealthBarWidget::BuildWidgetTree()
{
	if (ProgressBar || !WidgetTree)
	{
		return;
	}

	ProgressBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("HealthProgress"));
	WidgetTree->RootWidget = ProgressBar;
	ProgressBar->SetBarFillType(EProgressBarFillType::LeftToRight);
	ProgressBar->SetFillColorAndOpacity(FillColor);
	ProgressBar->SetIsEnabled(true);
	ProgressBar->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UReEchoHealthBarWidget::Refresh()
{
	if (!Combatant.IsValid())
	{
		return;
	}

	HandleHealthChanged(Combatant->CurrentHealth, Combatant->Stats.HpMax);
}

void UReEchoHealthBarWidget::BindCombatant(UReEchoCombatantComponent* InCombatant)
{
	if (Combatant.IsValid())
	{
		Combatant->OnHealthChanged.RemoveDynamic(this, &UReEchoHealthBarWidget::HandleHealthChanged);
	}
	Combatant = InCombatant;
	if (Combatant.IsValid())
	{
		Combatant->OnHealthChanged.AddUniqueDynamic(this, &UReEchoHealthBarWidget::HandleHealthChanged);
	}
}

void UReEchoHealthBarWidget::HandleHealthChanged(const float CurrentHealth, const float MaximumHealth)
{
	if (!ProgressBar)
	{
		return;
	}

	const float SafeMaximumHealth = FMath::Max(1.0f, MaximumHealth);
	ProgressBar->SetPercent(FMath::Clamp(CurrentHealth / SafeMaximumHealth, 0.0f, 1.0f));
}
