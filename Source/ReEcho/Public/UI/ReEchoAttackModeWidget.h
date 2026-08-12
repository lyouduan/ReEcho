#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReEchoAttackModeWidget.generated.h"

class SWidget;
class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FReEchoAttackModeRequested);

/** Pause-menu child that presents attack mode without owning pause/resume behavior. */
UCLASS(Blueprintable)
class REECHO_API UReEchoAttackModeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FReEchoAttackModeRequested OnAutomaticRequested;

	UPROPERTY(BlueprintAssignable)
	FReEchoAttackModeRequested OnManualRequested;

	void SetAutomaticMode(bool bAutomatic);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	void BuildWidgetTree();
	void Refresh();

	UFUNCTION()
	void HandleAutomaticClicked();

	UFUNCTION()
	void HandleManualClicked();

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CurrentModeText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> AutomaticButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ManualButton;

	bool bAutomaticMode = true;
};
