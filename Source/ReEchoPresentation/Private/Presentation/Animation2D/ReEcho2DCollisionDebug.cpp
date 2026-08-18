#include "Presentation/Animation2D/ReEcho2DCollisionDebug.h"

#include "HAL/IConsoleManager.h"

namespace
{
TAutoConsoleVariable<int32> CVarReEchoDebugCollision(
	TEXT("ReEcho.DebugCollision"),
	0,
	TEXT("Draw ReEcho collision: 0=off, 1=root, 2=root+Paper2D, 3=root+Paper2D+semantic tracks."),
	ECVF_Cheat);
}

int32 ReEcho2DCollisionDebug::GetLevel()
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	return FMath::Clamp(CVarReEchoDebugCollision.GetValueOnGameThread(), 0, 3);
#else
	return 0;
#endif
}
// ReEchoPresentation runtime implementation.
