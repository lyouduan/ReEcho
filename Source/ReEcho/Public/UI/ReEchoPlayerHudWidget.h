#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReEchoPlayerHudWidget.generated.h"

class SWidget;
class UImage;
class UProgressBar;
class UReEchoCombatantComponent;
class UTextBlock;
class UTexture2D;

/** 左上角玩家状态 HUD：头像、当前生命值与最大生命值。 */
UCLASS()

class REECHO_API UReEchoPlayerHudWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializePlayerHud(UReEchoCombatantComponent* InCombatant, UTexture2D* InPortraitTexture);

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
	TObjectPtr<UImage> PlayerPortrait;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> PlayerHealthProgress;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PlayerHealthText;

	TWeakObjectPtr<UReEchoCombatantComponent> Combatant;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> PortraitTexture;
};
