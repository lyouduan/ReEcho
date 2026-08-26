#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/ReEchoMinimapCanvasWidget.h"
#include "ReEchoEncounterHudWidget.generated.h"

class SWidget;
class UImage;
class UTextBlock;
class UWidget;

/** 顶部中央战斗状态：普通关显示倒计时；Boss 关切换为只读 Boss 生命条。 */
UCLASS()

class REECHO_API UReEchoEncounterHudWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetEncounterStatus(int32 EncounterIndex,
	                        int32 TotalEncounters,
	                        float RemainingSeconds,
	                        float DurationSeconds,
	                        bool bInBossEncounter = false,
	                        float BossCurrentHealth = 0.0f,
	                        float BossMaximumHealth = 0.0f);

	static FText FormatEncounterLabel(int32 EncounterIndex);
	static FText FormatCountdown(float RemainingSeconds);
	/** Map authoritative countdown progress onto the source needle's right-to-left counter-clockwise upper semicircle.
	 */
	static float CalculateCountdownNeedleAngle(float RemainingSeconds, float DurationSeconds);
	static float CalculateBossHealthRatio(float CurrentHealth, float MaximumHealth);

	/** 由 GameMode::Tick 每帧调用，转发小地图视图给 WBP 中放入的 ReEchoMinimapCanvasWidget。 */
	void SetMinimapView(const FReEchoMinimapView& View);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	void BuildWidgetTree();
	void RefreshText();

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EncounterText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CountdownText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> ArtClockNeedle;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> ArtClockFrame;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> BossHealthPanel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> BossHealthFill;

	/** WBP 中放入的小地图画布控件（按类型查找，命名不限）。 */
	TObjectPtr<UReEchoMinimapCanvasWidget> MinimapCanvas = nullptr;

	int32 CurrentEncounterIndex = 0;
	int32 EncounterCount = 6;
	float RemainingTime = 0.0f;
	float EncounterDuration = 0.0f;
	bool bBossEncounter = false;
	float CurrentBossHealth = 0.0f;
	float MaximumBossHealth = 0.0f;
};
