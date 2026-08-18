#pragma once

// Public API of the ReEchoPresentation runtime module.

#include "CoreMinimal.h"

namespace ReEcho2DCollisionDebug
{
/**
 * Shared collision visualization level.
 * 0 = off, 1 = Actor root collision, 2 = root + Paper2D authored shapes,
 * 3 = root + Paper2D + reviewed semantic body/attack tracks.
 */
REECHOPRESENTATION_API int32 GetLevel();
}
