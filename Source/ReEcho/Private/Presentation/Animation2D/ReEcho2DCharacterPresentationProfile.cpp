#include "Presentation/Animation2D/ReEcho2DCharacterPresentationProfile.h"

const FReEcho2DAnimationClip* FReEcho2DCompositeAnimationSet::FindClip(const FGameplayTag SemanticKey) const
{
	const FReEcho2DAnimationClip* Clip = Clips.Find(SemanticKey);
	return Clip && Clip->IsValid() ? Clip : nullptr;
}

const FReEcho2DCompositeAnimationSet* UReEcho2DCharacterPresentationProfile::FindAnimationSet(
	const FName WeaponVisualSetId) const
{
	const FReEcho2DCompositeAnimationSet* DefaultSet = nullptr;
	for (const FReEcho2DCompositeAnimationSet& Set : AnimationSets)
	{
		if (Set.WeaponVisualSetId == WeaponVisualSetId && !WeaponVisualSetId.IsNone())
		{
			return &Set;
		}
		if (Set.WeaponVisualSetId.IsNone())
		{
			DefaultSet = &Set;
		}
	}
	return DefaultSet;
}

const FReEcho2DAnimationClip* UReEcho2DCharacterPresentationProfile::ResolveClip(
	const FName WeaponVisualSetId, const FGameplayTag SemanticKey) const
{
	if (const FReEcho2DCompositeAnimationSet* SelectedSet = FindAnimationSet(WeaponVisualSetId))
	{
		if (const FReEcho2DAnimationClip* Clip = SelectedSet->FindClip(SemanticKey))
		{
			return Clip;
		}
	}

	if (!WeaponVisualSetId.IsNone())
	{
		for (const FReEcho2DCompositeAnimationSet& Set : AnimationSets)
		{
			if (Set.WeaponVisualSetId.IsNone())
			{
				return Set.FindClip(SemanticKey);
			}
		}
	}
	return nullptr;
}
