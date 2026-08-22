#pragma once

#include "CoreMinimal.h"
#include "Engine/Console.h"
#include "ReEchoConsole.generated.h"

/** Development console that pauses gameplay only while the console is visible. */
UCLASS(Within = GameViewportClient, Transient)

class REECHO_API UReEchoConsole : public UConsole
{
	GENERATED_BODY()

public:
	virtual void FakeGotoState(FName NextStateName) override;

private:
	bool bPausedByConsole = false;
};
