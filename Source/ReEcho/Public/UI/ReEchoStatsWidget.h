#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/ReEchoTypes.h"
#include "ReEchoStatsWidget.generated.h"

class SWidget;
class UButton;
class UImage;
class UTextBlock;
class UTexture2D;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FReEchoStatsClosed);

/** 并排展示玩家与当前回响的即时属性；无回响时显示空状态。 */
UCLASS()

class REECHO_API UReEchoStatsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UReEchoStatsWidget(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(BlueprintAssignable)
	FReEchoStatsClosed OnClosed;

	/** 用调用时的快照刷新双方属性，不持有玩法对象。 */
	void InitializeStats(const FReEchoStatBlock& PlayerStats,
	                     float PlayerCurrentHealth,
	                     const FReEchoStatBlock& EchoStats,
	                     float EchoCurrentHealth,
	                     bool bHasEcho);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	void BuildWidgetTree();
	void Refresh();
	FText FormatStats(const FReEchoStatBlock& Stats, float CurrentHealth) const;

	UFUNCTION()
	void HandleCloseClicked();

	UPROPERTY()
	TObjectPtr<UTexture2D> BackgroundTexture;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> BackgroundImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PlayerStatsText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EchoStatsText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

	FReEchoStatBlock CurrentPlayerStats;
	FReEchoStatBlock CurrentEchoStats;
	float CurrentPlayerHealth = 0.0f;
	float CurrentEchoHealth = 0.0f;
	bool bCurrentHasEcho = false;
};
