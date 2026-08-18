#include "Presentation/Animation2D/ReEcho2DAnimationStateMachineAsset.h"

const FReEcho2DAnimationStateDefinition*
UReEcho2DAnimationStateMachineAsset::FindState(const FGameplayTag StateTag) const
{
	return States.FindByPredicate(
	    [StateTag](const FReEcho2DAnimationStateDefinition& State) { return State.StateTag == StateTag; });
}

const FReEcho2DAnimationStateDefinition*
UReEcho2DAnimationStateMachineAsset::FindStateBySemantic(const FGameplayTag SemanticKey) const
{
	return States.FindByPredicate([SemanticKey](const FReEcho2DAnimationStateDefinition& State)
	{
		return State.SemanticKey == SemanticKey;
	});
}
// ReEchoPresentation runtime implementation.
