#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReEchoHealthBarWidget.generated.h"

class SWidget;
class UProgressBar;
class UReEchoCombatantComponent;

/** 跟随战斗组件生命值刷新的轻量进度条。 */
UCLASS()

class REECHO_API UReEchoHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 绑定生命来源，并设置该单位的血条颜色。 */
	void InitializeHealth(UReEchoCombatantComponent* InCombatant, const FLinearColor& InFillColor);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void BuildWidgetTree();
	void Refresh();
	void BindCombatant(UReEchoCombatantComponent* InCombatant);

	UFUNCTION()
	void HandleHealthChanged(float CurrentHealth, float MaximumHealth);

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> ProgressBar;

	TWeakObjectPtr<UReEchoCombatantComponent> Combatant;
	FLinearColor FillColor = FLinearColor::Green;
};
