#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ReEchoButtonVisualFeedback.generated.h"

class UButton;
class UWidget;

/** Keeps generic button hover feedback independent from screen-specific behavior. */
UCLASS()

class REECHO_API UReEchoButtonVisualFeedback : public UObject
{
	GENERATED_BODY()

public:
	void Bind(UButton* InButton,
	          UWidget* InVisualRoot,
	          const FString& InScreenName = TEXT("Unregistered"),
	          const FString& InWidgetName = TEXT("Unknown"));
	bool IsBoundTo(const UButton* InButton) const;
	bool HasValidButton() const;

private:
	UFUNCTION()
	void HandleHovered();

	UFUNCTION()
	void HandleUnhovered();

	UFUNCTION()
	void HandleClicked();

	TWeakObjectPtr<UButton> Button;
	TWeakObjectPtr<UWidget> VisualRoot;
	FString ScreenName;
	FString WidgetName;
	FVector2D RestingScale = FVector2D(1.0f);
	FVector2D RestingPivot = FVector2D(0.0f);
};
