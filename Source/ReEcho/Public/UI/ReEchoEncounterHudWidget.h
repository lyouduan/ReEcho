#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReEchoEncounterHudWidget.generated.h"

class SWidget;
class UTextBlock;

/** 右上角战斗状态面板：显示当前关卡与本关剩余秒数。 */
UCLASS()

class REECHO_API UReEchoEncounterHudWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetEncounterStatus(int32 EncounterIndex, int32 TotalEncounters, float RemainingSeconds);

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

	int32 CurrentEncounterIndex = 0;
	int32 EncounterCount = 6;
	float RemainingTime = 0.0f;
};
