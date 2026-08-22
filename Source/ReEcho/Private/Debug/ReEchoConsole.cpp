#include "Debug/ReEchoConsole.h"

#include "Kismet/GameplayStatics.h"

void UReEchoConsole::FakeGotoState(const FName NextStateName)
{
	const bool bWasActive = ConsoleActive();
	const bool bWillBeActive = !NextStateName.IsNone();

	if (!bWasActive && bWillBeActive && !UGameplayStatics::IsGamePaused(this))
	{
		bPausedByConsole = UGameplayStatics::SetGamePaused(this, true);
	}

	Super::FakeGotoState(NextStateName);

	if (bWasActive && !bWillBeActive && bPausedByConsole)
	{
		UGameplayStatics::SetGamePaused(this, false);
		bPausedByConsole = false;
	}
}
