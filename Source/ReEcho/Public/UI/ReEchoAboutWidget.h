#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReEchoAboutWidget.generated.h"

class UButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FReEchoAboutClosed);

/** 「关于我们」面板骨架。Plan60 换皮占位，后续由美术素材与文案接入。 */
UCLASS()
class REECHO_API UReEchoAboutWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FReEchoAboutClosed OnClosed;

protected:
	virtual void NativeConstruct() override;

private:
	UFUNCTION()
	void HandleCloseClicked();

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Close;
};
