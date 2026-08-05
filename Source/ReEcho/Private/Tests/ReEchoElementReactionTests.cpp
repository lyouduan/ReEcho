#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Combat/ReEchoElementReaction.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoElementReactionTest,
                                 "ReEcho.Combat.ElementReactions",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoElementReactionTest::RunTest(const FString& Parameters)
{
	FReEchoElementState State;
	FReEchoElementHitResult Result = ReEchoElementReaction::ResolveHit(State, EReEchoElement::Water, 10.0f);
	TestFalse(TEXT("First water hit only attaches"), Result.bTriggeredReaction);
	TestEqual(TEXT("Water becomes attached"), State.Attached, EReEchoElement::Water);
	TestEqual(TEXT("Non-reaction keeps base damage"), Result.Damage, 10.0f);

	Result = ReEchoElementReaction::ResolveHit(State, EReEchoElement::Grass, 10.0f);
	TestTrue(TEXT("Water and grass react"), Result.bTriggeredReaction);
	TestEqual(TEXT("Water-grass reaction doubles damage"), Result.Damage, 20.0f);
	TestEqual(TEXT("Reaction consumes attachment"), State.Attached, EReEchoElement::None);

	ReEchoElementReaction::ResolveHit(State, EReEchoElement::Flame, 12.0f);
	Result = ReEchoElementReaction::ResolveHit(State, EReEchoElement::Grass, 12.0f);
	TestTrue(TEXT("Flame and grass react in reverse order"), Result.bTriggeredReaction);
	TestEqual(TEXT("Grass-flame reaction doubles damage"), Result.Damage, 24.0f);

	ReEchoElementReaction::ResolveHit(State, EReEchoElement::Water, 8.0f);
	Result = ReEchoElementReaction::ResolveHit(State, EReEchoElement::Flame, 8.0f);
	TestFalse(TEXT("Water and flame do not react"), Result.bTriggeredReaction);
	TestEqual(TEXT("Unsupported pair keeps base damage"), Result.Damage, 8.0f);
	TestEqual(TEXT("Unsupported pair replaces attachment"), State.Attached, EReEchoElement::Flame);

	Result = ReEchoElementReaction::ResolveHit(State, EReEchoElement::Flame, 8.0f);
	TestFalse(TEXT("Same elements do not react"), Result.bTriggeredReaction);
	TestEqual(TEXT("Same element remains attached"), State.Attached, EReEchoElement::Flame);

	State.Attached = EReEchoElement::Water;
	Result = ReEchoElementReaction::ResolveHit(State, EReEchoElement::Grass, 10.0f, 1.05f);
	TestEqual(TEXT("Poet reaction efficiency scales reaction damage"), Result.Damage, 21.0f);
	return true;
}

#endif
