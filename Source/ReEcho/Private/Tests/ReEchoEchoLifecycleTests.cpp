#if WITH_DEV_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Graybox/ReEchoEchoActor.h"
#include "Misc/AutomationTest.h"
#include "ReEchoGameMode.h"

namespace
{
struct FEchoLifecycleWorldFixture
{
	UWorld* World = nullptr;

	FEchoLifecycleWorldFixture()
	{
		const FName WorldName = MakeUniqueObjectName(nullptr, UWorld::StaticClass(), TEXT("EchoLifecycleWorld"));
		FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
		World = UWorld::CreateWorld(EWorldType::Game, false, WorldName, GetTransientPackage());
		World->AddToRoot();
		WorldContext.SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
		World->BeginPlay();
	}

	~FEchoLifecycleWorldFixture()
	{
		if (World)
		{
			World->DestroyWorld(true);
			GEngine->DestroyWorldContext(World);
			World->RemoveFromRoot();
		}
	}
};
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoMartyrEchoWorldRetirementTest,
	                             "ReEcho.Echo.Lifecycle.MartyrRetiresWithoutRemovingOrdinaryEcho",
	                             EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoMartyrEchoWorldRetirementTest::RunTest(const FString& Parameters)
{
	FEchoLifecycleWorldFixture Fixture;
	AReEchoGameMode* GameMode = Fixture.World->SpawnActor<AReEchoGameMode>();
	AReEchoEchoActor* OrdinaryEcho = Fixture.World->SpawnActor<AReEchoEchoActor>();
	AReEchoEchoActor* MartyrEcho = Fixture.World->SpawnActor<AReEchoEchoActor>();
	if (!TestNotNull(TEXT("GameMode spawns"), GameMode) || !TestNotNull(TEXT("Ordinary Echo spawns"), OrdinaryEcho) ||
	    !TestNotNull(TEXT("Martyr Echo spawns"), MartyrEcho))
	{
		return false;
	}

	FReEchoCardRuleSnapshot MartyrRules;
	MartyrRules.bEchoesCanAttack = false;
	MartyrRules.bRetireEchoOnDefeat = true;
	MartyrEcho->ConfigureCardRules(MartyrRules, FReEchoStatBlock{});
	GameMode->AddEchoForTests(OrdinaryEcho);
	GameMode->AddEchoForTests(MartyrEcho);
	MartyrEcho->NotifyDefeated(EReEchoDamageSource::Enemy);
	GameMode->RemoveRetiredEchoesForTests();

	TestEqual(TEXT("Only the ordinary Echo remains in the authoritative container"), GameMode->GetEchoCountForTests(), 1);
	TestTrue(TEXT("The ordinary Echo remains valid"), IsValid(OrdinaryEcho));
	TestFalse(TEXT("The Martyr Echo is destroyed after retirement cleanup"), IsValid(MartyrEcho));
	return true;
}

#endif
