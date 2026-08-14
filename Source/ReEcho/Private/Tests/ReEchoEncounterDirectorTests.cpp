#if WITH_DEV_AUTOMATION_TESTS

#include "Core/ReEchoBalanceSettings.h"
#include "Encounter/ReEchoEncounterDirector.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

namespace
{
struct FReEchoEncounterDirectorWorldFixture
{
	UWorld* World = nullptr;

	FReEchoEncounterDirectorWorldFixture()
	{
		const FName WorldName =
		    MakeUniqueObjectName(nullptr, UWorld::StaticClass(), TEXT("ReEchoEncounterDirectorTestWorld"));
		FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
		World = UWorld::CreateWorld(EWorldType::Game, false, WorldName, GetTransientPackage());
		World->AddToRoot();
		Context.SetCurrentWorld(World);
		World->SetShouldTick(true);
		World->InitializeActorsForPlay(FURL());
		World->BeginPlay();
	}

	~FReEchoEncounterDirectorWorldFixture()
	{
		if (World)
		{
			World->DestroyWorld(true);
			GEngine->DestroyWorldContext(World);
			World->RemoveFromRoot();
		}
	}

	void Advance(AReEchoEncounterDirector* Director, const float Seconds) const
	{
		constexpr float StepSeconds = 0.25f;
		for (int32 Step = 0; Step < FMath::CeilToInt(Seconds / StepSeconds); ++Step)
		{
			Director->AdvanceForTesting(StepSeconds);
		}
	}
};
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEncounterDirectorBossDurationTest,
                                 "ReEcho.Encounter.BossContinuesAfterStandardDuration",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEncounterDirectorBossDurationTest::RunTest(const FString& Parameters)
{
	FReEchoEncounterDirectorWorldFixture Fixture;
	AReEchoEncounterDirector* Director = Fixture.World->SpawnActor<AReEchoEncounterDirector>();
	TestNotNull(TEXT("Encounter director spawns"), Director);
	if (!Director)
	{
		return false;
	}

	Director->SetEndsOnDuration(false);
	Director->StartEncounter();
	Fixture.Advance(Director, GetDefault<UReEchoBalanceSettings>()->EncounterDuration + 1.0f);
	TestTrue(TEXT("Boss encounter clock continues beyond the standard duration"),
	         Director->EncounterTime > GetDefault<UReEchoBalanceSettings>()->EncounterDuration);
	TestEqual(TEXT("Boss encounter HUD remaining time floors at zero"), Director->GetRemainingTime(), 0.0f);

	const float SavedBossTime = Director->EncounterTime;
	Director->EndEncounter();
	Director->SetEndsOnDuration(false);
	Director->ResumeEncounter(SavedBossTime);
	TestTrue(TEXT("Boss encounter resumes after the standard duration"),
	         Director->EncounterTime > GetDefault<UReEchoBalanceSettings>()->EncounterDuration);
	return true;
}

#endif
