#include "Presentation/Animation2D/ReEcho2DPresentationCatalog.h"

#include "Presentation/Animation2D/ReEcho2DCharacterPresentationProfile.h"
#include "Presentation/Animation2D/ReEcho2DAnimationTags.h"

UReEcho2DCharacterPresentationProfile* UReEcho2DPresentationCatalog::ResolveProfile(const FName PresentationId) const
{
	if (PresentationId.IsNone())
	{
		return nullptr;
	}
	for (const FReEcho2DPresentationCatalogEntry& Entry : PresentationEntries)
	{
		if (Entry.PresentationId == PresentationId)
		{
			return Entry.Profile;
		}
	}
	for (UReEcho2DCharacterPresentationProfile* Profile : CharacterProfiles)
	{
		if (Profile && Profile->AppearanceId == PresentationId)
		{
			return Profile;
		}
	}
	return nullptr;
}

FGameplayTag UReEcho2DPresentationCatalog::ResolveSemanticTag(const FName TagName)
{
	const FGameplayTag Candidates[] = {
	    ReEcho2DAnimationTags::Idle,
	    ReEcho2DAnimationTags::Move,
	    ReEcho2DAnimationTags::Attack_Basic,
	    ReEcho2DAnimationTags::Attack_Charge,
	    ReEcho2DAnimationTags::Transform_Phase2,
	    ReEcho2DAnimationTags::Hit,
	    ReEcho2DAnimationTags::Death,
	};
	for (const FGameplayTag Candidate : Candidates)
	{
		if (Candidate.GetTagName() == TagName)
		{
			return Candidate;
		}
	}
	return FGameplayTag();
}

// ReEchoPresentation runtime implementation.
