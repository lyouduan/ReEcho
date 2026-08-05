#include "Misc/AutomationTest.h"

#include "Graybox/ReEchoBomberRules.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoBomberRangeTest,
                                 "ReEcho.Enemies.BomberRanges",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoBomberRangeTest::RunTest(const FString& Parameters)
{
	constexpr float TriggerRadius = 260.0f;
	constexpr float DamageRadius = 180.0f;

	TestFalse(TEXT("A distant bomber does not start its fuse"),
	          ReEchoBomberRules::ShouldStartFuse(TriggerRadius + 1.0f, TriggerRadius));
	TestTrue(TEXT("The trigger boundary starts the fuse"),
	         ReEchoBomberRules::ShouldStartFuse(TriggerRadius, TriggerRadius));
	TestTrue(TEXT("The explosion boundary applies damage"),
	         ReEchoBomberRules::IsInsideExplosion(DamageRadius, DamageRadius));
	TestFalse(TEXT("A player outside the explosion is not damaged"),
	          ReEchoBomberRules::IsInsideExplosion(DamageRadius + 1.0f, DamageRadius));
	TestTrue(TEXT("Trigger range leaves an escape margin"), TriggerRadius > DamageRadius);
	return true;
}

#endif
