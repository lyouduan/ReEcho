#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReEchoRestartWidget.generated.h"

class SWidget;
class UButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FReEchoRestartRequested);

UCLASS()
class REECHO_API UReEchoRestartWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FReEchoRestartRequested OnRestartRequested;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	void BuildWidgetTree();

	UFUNCTION()
	void HandleRestartClicked();

	UPROPERTY(Transient)
	TObjectPtr<UButton> RestartButton;
};