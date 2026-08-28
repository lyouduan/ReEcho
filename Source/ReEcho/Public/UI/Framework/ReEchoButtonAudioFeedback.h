#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ReEchoButtonAudioFeedback.generated.h"

class UButton;
class UGameInstance;

/** Owns semantic hover/click audio bindings for one UI button. */
UCLASS()

class REECHO_API UReEchoButtonAudioFeedback : public UObject
{
	GENERATED_BODY()

public:
	void Bind(UButton* InButton, UGameInstance* InGameInstance, FName InHoverEventId, FName InClickEventId);
	bool IsBoundTo(const UButton* InButton) const;
	bool HasValidButton() const;

private:
	void PostEvent(FName EventId) const;

	UFUNCTION()
	void HandleHovered();

	UFUNCTION()
	void HandleClicked();

	TWeakObjectPtr<UButton> Button;
	TWeakObjectPtr<UGameInstance> GameInstance;
	FName HoverEventId = NAME_None;
	FName ClickEventId = NAME_None;
};
