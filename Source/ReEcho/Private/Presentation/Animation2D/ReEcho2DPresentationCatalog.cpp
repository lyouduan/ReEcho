#include "Presentation/Animation2D/ReEcho2DPresentationCatalog.h"

#include "Presentation/Animation2D/ReEcho2DCharacterPresentationProfile.h"

UReEcho2DCharacterPresentationProfile* UReEcho2DPresentationCatalog::ResolveProfile(
	const FName AppearanceId) const
{
	if (AppearanceId.IsNone())
	{
		return nullptr;
	}
	for (UReEcho2DCharacterPresentationProfile* Profile : CharacterProfiles)
	{
		if (Profile && Profile->AppearanceId == AppearanceId)
		{
			return Profile;
		}
	}
	return nullptr;
}
