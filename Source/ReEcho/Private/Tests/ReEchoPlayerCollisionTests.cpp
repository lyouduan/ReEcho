#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/ReEchoCombatantComponent.h"
#include "Combat/ReEchoHitResolver.h"
#include "Components/BoxComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Graybox/ReEchoEnemyActor.h"
#include "Misc/AutomationTest.h"
#include "Player/ReEchoPlayerPawn.h"

namespace
{
struct FReEchoPlayerCollisionWorldFixture
{
	UWorld* World = nullptr;

	FReEchoPlayerCollisionWorldFixture()
	{
		const FName WorldName =
		    MakeUniqueObjectName(nullptr, UWorld::StaticClass(), TEXT("ReEchoPlayerCollisionTestWorld"));
		FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
		World = UWorld::CreateWorld(EWorldType::Game, false, WorldName, GetTransientPackage());
		World->AddToRoot();
		Context.SetCurrentWorld(World);
		World->SetShouldTick(true);
		World->InitializeActorsForPlay(FURL());
		World->BeginPlay();
	}

	~FReEchoPlayerCollisionWorldFixture()
	{
		if (World)
		{
			World->DestroyWorld(true);
			GEngine->DestroyWorldContext(World);
			World->RemoveFromRoot();
		}
	}
};

void TickPlayerFor(AReEchoPlayerPawn& Player, const float DurationSeconds)
{
	constexpr float StepSeconds = 0.1f;
	float RemainingSeconds = DurationSeconds;
	while (RemainingSeconds > KINDA_SMALL_NUMBER)
	{
		const float DeltaSeconds = FMath::Min(StepSeconds, RemainingSeconds);
		Player.Tick(DeltaSeconds);
		RemainingSeconds -= DeltaSeconds;
	}
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoPlayerHurtCollisionIgnoreTest,
                                 "ReEcho.Player.HurtCollisionIgnore",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoPlayerHurtCollisionIgnoreTest::RunTest(const FString& Parameters)
{
	FReEchoPlayerCollisionWorldFixture Fixture;
	AReEchoPlayerPawn* Player = Fixture.World->SpawnActor<AReEchoPlayerPawn>();
	AReEchoEnemyActor* Enemy = Fixture.World->SpawnActor<AReEchoEnemyActor>();
	if (!TestNotNull(TEXT("Player spawns"), Player) || !TestNotNull(TEXT("Enemy source spawns"), Enemy))
	{
		return false;
	}
	if (!Player->HasActorBegunPlay())
	{
		Player->DispatchBeginPlay();
	}
	if (!Enemy->HasActorBegunPlay())
	{
		Enemy->DispatchBeginPlay();
	}

	FReEchoStatBlock PlayerStats;
	PlayerStats.HpMax = 100.0f;
	PlayerStats.HpPoint = 100.0f;
	Player->Combatant->InitializeFromStats(PlayerStats, true);
	const ECollisionResponse AuthoredPawnResponse = Player->Collision->GetCollisionResponseToChannel(ECC_Pawn);

	auto ApplyEnemyDamage = [Player, Enemy]()
	{
		FReEchoHitIntent Intent;
		Intent.Attack.Source = Enemy;
		Intent.Target = Player;
		Intent.RawDamage = 5.0f;
		Intent.DamageSource = EReEchoDamageSource::Enemy;
		Intent.SourceLocation = Enemy->GetActorLocation();
		Intent.HitLocation = Player->GetActorLocation();
		return ReEchoHitResolver::ResolvePhysicalHit(Intent);
	};

	const FReEchoHitResolved FirstHit = ApplyEnemyDamage();
	TestEqual(TEXT("The first hit still applies damage"), FirstHit.AppliedDamage, 5.0f);
	TestEqual(TEXT("Player health is reduced during collision ignore"), Player->Combatant->CurrentHealth, 95.0f);
	TestEqual(TEXT("A real hurt temporarily ignores Pawn collision"),
	          Player->Collision->GetCollisionResponseToChannel(ECC_Pawn),
	          ECollisionResponse::ECR_Ignore);

	TickPlayerFor(*Player, 0.5f);
	const FReEchoHitResolved SecondHit = ApplyEnemyDamage();
	TestEqual(TEXT("Collision ignore does not grant damage invulnerability"), SecondHit.AppliedDamage, 5.0f);
	TestEqual(TEXT("A repeated hurt still reduces health"), Player->Combatant->CurrentHealth, 90.0f);

	TickPlayerFor(*Player, 0.9f);
	TestEqual(TEXT("Repeated hurt extends the one-second collision-ignore window"),
	          Player->Collision->GetCollisionResponseToChannel(ECC_Pawn),
	          ECollisionResponse::ECR_Ignore);
	TickPlayerFor(*Player, 0.2f);
	TestEqual(TEXT("Pawn collision returns to its authored response after one second"),
	          Player->Collision->GetCollisionResponseToChannel(ECC_Pawn),
	          AuthoredPawnResponse);
	return true;
}

#endif
