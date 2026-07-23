#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReEchoHealthBarWidget.generated.h"

class SWidget;
class UProgressBar;
class UReEchoCombatantComponent;

UCLASS()
class REECHO_API UReEchoHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeHealth(UReEchoCombatantComponent* InCombatant, const FLinearColor& InFillColor);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void BuildWidgetTree();
	void Refresh();

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> ProgressBar;

	TWeakObjectPtr<UReEchoCombatantComponent> Combatant;
	FLinearColor FillColor = FLinearColor::Green;
};