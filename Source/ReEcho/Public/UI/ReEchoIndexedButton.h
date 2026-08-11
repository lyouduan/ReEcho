#pragma once

#include "CoreMinimal.h"
#include "Components/Button.h"
#include "ReEchoIndexedButton.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FReEchoIndexedButtonClicked, int32, Index);

/** Reusable button that reports a caller-owned entry index without containing gameplay state. */
UCLASS()

class REECHO_API UReEchoIndexedButton : public UButton
{
	GENERATED_BODY()

public:
	UReEchoIndexedButton(const FObjectInitializer& ObjectInitializer);

	void SetEntryIndex(int32 InIndex);

	UPROPERTY(BlueprintAssignable)
	FReEchoIndexedButtonClicked OnIndexedClicked;

private:
	UFUNCTION()
	void HandleClicked();

	int32 EntryIndex = INDEX_NONE;
};
