#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReEchoStartMenuWidget.generated.h"

class SWidget;
class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FReEchoNewGameRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FReEchoContinueGameRequested);

/** Blocking pre-run panel that offers actions based on resumable-save availability. */
UCLASS()

class REECHO_API UReEchoStartMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FReEchoNewGameRequested OnNewGameRequested;

	UPROPERTY(BlueprintAssignable)
	FReEchoContinueGameRequested OnContinueGameRequested;

	void InitializeMenu(bool bInHasSavedRun);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	void BuildWidgetTree();
	void RefreshMenu();

	UFUNCTION()
	void HandleNewGameClicked();

	UFUNCTION()
	void HandleContinueClicked();

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ContinueButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> NewGameButton;

	bool bHasSavedRun = false;
};
