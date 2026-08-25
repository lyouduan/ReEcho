#if WITH_DEV_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Data/ReEchoCsvDataRegistry.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Player/ReEchoPlayerPawn.h"

namespace
{
struct FPlayerPresentationResetWorldFixture
{
	UWorld* World = nullptr;

	FPlayerPresentationResetWorldFixture()
	{
		const FName WorldName =
		    MakeUniqueObjectName(nullptr, UWorld::StaticClass(), TEXT("PlayerPresentationResetWorld"));
		FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
		World = UWorld::CreateWorld(EWorldType::Game, false, WorldName, GetTransientPackage());
		World->AddToRoot();
		WorldContext.SetCurrentWorld(World);
		World->SetShouldTick(true);
		World->InitializeActorsForPlay(FURL());
		World->BeginPlay();
	}

	~FPlayerPresentationResetWorldFixture()
	{
		if (World)
		{
			World->DestroyWorld(true);
			GEngine->DestroyWorldContext(World);
			World->RemoveFromRoot();
		}
	}
};

USceneComponent* FindSceneComponent(AActor& Actor, const FName Name)
{
	return Cast<USceneComponent>(Actor.GetDefaultSubobjectByName(Name));
}
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoPlayerPresentationBaselineResetTest,
                                 "ReEcho.Player.PresentationBaseline.ResetOnCharacterConfigure",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoPlayerPresentationBaselineResetTest::RunTest(const FString& Parameters)
{
	const FReEchoCsvLoadResult LoadResult = FReEchoCsvDataRegistry::LoadAndPublishDefault();
	if (!TestTrue(TEXT("Default CSV data loads"), LoadResult.bSuccess))
	{
		AddError(LoadResult.FormatIssues());
		return false;
	}

	FPlayerPresentationResetWorldFixture Fixture;
	AReEchoPlayerPawn* Player = Fixture.World->SpawnActor<AReEchoPlayerPawn>();
	if (!TestNotNull(TEXT("Player spawns"), Player))
	{
		return false;
	}
	if (!Player->HasActorBegunPlay())
	{
		Player->DispatchBeginPlay();
	}

	USceneComponent* FlipbookRoot = FindSceneComponent(*Player, TEXT("FlipbookRoot"));
	USceneComponent* EffectsRoot = FindSceneComponent(*Player, TEXT("EffectsRoot"));
	if (!TestNotNull(TEXT("Player has FlipbookRoot"), FlipbookRoot) ||
	    !TestNotNull(TEXT("Player has EffectsRoot"), EffectsRoot))
	{
		return false;
	}
	const FVector AuthoredFlipbookScale = FlipbookRoot->GetRelativeScale3D();
	const FVector AuthoredEffectsScale = EffectsRoot->GetRelativeScale3D();

	Player->PlayHitVisual();
	Fixture.World->Tick(ELevelTick::LEVELTICK_All, 1.0f / 120.0f);
	TestTrue(TEXT("Hit feedback temporarily deforms FlipbookRoot"),
	         !FlipbookRoot->GetRelativeScale3D().Equals(AuthoredFlipbookScale, KINDA_SMALL_NUMBER));
	TestTrue(TEXT("Hit feedback temporarily deforms EffectsRoot"),
	         !EffectsRoot->GetRelativeScale3D().Equals(AuthoredEffectsScale, KINDA_SMALL_NUMBER));

	TestTrue(TEXT("Character switch succeeds during active hit feedback"),
	         Player->ConfigureCharacter(TEXT("J_DIAMOND")));
	TestTrue(TEXT("Character switch restores authored FlipbookRoot scale"),
	         FlipbookRoot->GetRelativeScale3D().Equals(AuthoredFlipbookScale, KINDA_SMALL_NUMBER));
	TestTrue(TEXT("Character switch restores authored EffectsRoot scale"),
	         EffectsRoot->GetRelativeScale3D().Equals(AuthoredEffectsScale, KINDA_SMALL_NUMBER));

	Fixture.World->Tick(ELevelTick::LEVELTICK_All, 1.0f / 120.0f);
	TestTrue(TEXT("Cleared hit feedback cannot reapply after character switch"),
	         FlipbookRoot->GetRelativeScale3D().Equals(AuthoredFlipbookScale, KINDA_SMALL_NUMBER));
	TestTrue(TEXT("Repeated configuration keeps the authored baseline"),
	         Player->ConfigureCharacter(TEXT("J_CLOVER")) && Player->ConfigureCharacter(TEXT("J_HEART")) &&
	             Player->ConfigureCharacter(TEXT("J_SPADE")) &&
	             FlipbookRoot->GetRelativeScale3D().Equals(AuthoredFlipbookScale, KINDA_SMALL_NUMBER) &&
	             EffectsRoot->GetRelativeScale3D().Equals(AuthoredEffectsScale, KINDA_SMALL_NUMBER));
	return true;
}

#endif
