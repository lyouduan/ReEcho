#include "Presentation/Animation2D/ReEcho2DAnimationClip.h"

bool FReEcho2DAnimationClip::IsValid() const
{
	return Flipbook && PlayRate > 0.0f && (bUseNativeScale || WorldHeight > 0.0f);
}
