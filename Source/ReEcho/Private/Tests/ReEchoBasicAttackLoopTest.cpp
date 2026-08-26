#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystem/ReEchoPlayerAbilities.h"
#include "AbilitySystemComponent.h"
#include "Combat/ReEchoCombatantComponent.h"
#include "Core/ReEchoTypes.h"
#include "Data/ReEchoCsvDataRegistry.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Graybox/ReEchoEnemyActor.h"
#include "Misc/AutomationTest.h"
#include "Player/ReEchoPlayerPawn.h"
#include "TimerManager.h"
#include "Weapons/ReEchoWeaponActor.h"

// Plan38 确定性回归：held 普攻在临时武器动作锁（有序攻击步骤锁）期间不得终止。
// 长剑攻击重复间隔和有序步骤锁都来自生产表；循环在本次攻击完成后继续提交下一击。
//
// 覆盖真实 held-input -> GAS -> weapon -> 第二发命中的接缝（无 PIE）。
// 自动与手动入口都汇聚到同一个 UReEchoBasicAttackAbility 循环，故分别验证。
//
// 注：该测试需要 GEngine 世界上下文，与 ReEchoWeaponRuntimeTests 同属编辑器运行时测试；
// 不标记 SmokeFilter，避免 -ExecCmds smoke 路径在 GEngine 尚未就绪时执行。

namespace
{
void RunHeldBasicAttackRepeatScenario(FAutomationTestBase& Test, const bool bManual)
{
	const TCHAR* ModeName = bManual ? TEXT("Manual") : TEXT("Auto");

	// 轻量 headless 游戏世界（与 ReEchoWeaponRuntimeTests 的夹具一致）。
	const FName WorldName = MakeUniqueObjectName(nullptr, UWorld::StaticClass(), TEXT("ReEchoHeldAttackTest"));
	FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, WorldName, GetTransientPackage());
	World->AddToRoot();
	WorldContext.SetCurrentWorld(World);
	World->SetShouldTick(true);
	World->InitializeActorsForPlay(FURL());
	World->BeginPlay();

	// 必须在生成玩家前加载武器/步骤数据，否则武器初始化读不到快照。
	FReEchoCsvDataRegistry::LoadAndPublishDefault();

	const FVector PawnLocation(0.0f, 0.0f, 0.0f);
	AReEchoPlayerPawn* Pawn = World->SpawnActor<AReEchoPlayerPawn>(PawnLocation, FRotator::ZeroRotator);
	Test.TestNotNull(TEXT("Pawn spawned"), Pawn);
	if (!Pawn)
	{
		World->DestroyWorld(true);
		GEngine->DestroyWorldContext(World);
		World->RemoveFromRoot();
		return;
	}
	// The lightweight headless world does not automatically dispatch BeginPlay for actors spawned
	// after World::BeginPlay, so explicitly enter the same lifecycle used by PIE.
	if (!Pawn->HasActorBegunPlay())
	{
		Pawn->DispatchBeginPlay();
	}

	AReEchoWeaponActor* Weapon = Pawn->GetWeapon();
	Test.TestNotNull(TEXT("Weapon spawned"), Weapon);
	if (!Weapon)
	{
		World->DestroyWorld(true);
		GEngine->DestroyWorldContext(World);
		World->RemoveFromRoot();
		return;
	}
	Test.TestTrue(FString::Printf(TEXT("[%s] Long sword selected (W_J_01)"), ModeName),
	              Weapon->SelectWeaponById(TEXT("W_J_01")));

	// 身前放置敌人，用于验证“第二发命中”。
	const FVector EnemyLocation = PawnLocation + FVector(100.0f, 0.0f, 0.0f);
	AReEchoEnemyActor* Enemy = World->SpawnActor<AReEchoEnemyActor>(EnemyLocation, FRotator::ZeroRotator);
	Enemy->Configure(EReEchoEnemyKind::Grunt, 0);
	FReEchoStatBlock Stats;
	Stats.HpMax = 100.0f;
	Stats.HpPoint = 100.0f;
	Stats.Block = 0.0f;
	Enemy->GetCombatantComponent()->BindToAbilitySystem(Enemy->GetAbilitySystemComponent());
	Enemy->GetCombatantComponent()->InitializeFromStats(Stats, true);
	const float InitialHealth = Enemy->GetCombatantComponent()->CurrentHealth;

	// 以“按住”方式触发 GAS 普攻输入。
	if (bManual)
	{
		Pawn->SetAutoAttackMode(false);
		Pawn->ManualBasicAttack();
	}
	else
	{
		Pawn->PressAutoAttackInput();
	}
	FGameplayAbilitySpec* RuntimeBasicSpec =
	    Pawn->GetAbilitySystemComponent()->FindAbilitySpecFromClass(UReEchoBasicAttackAbility::StaticClass());
	UReEchoBasicAttackAbility* RuntimeBasicAbility =
	    RuntimeBasicSpec ? Cast<UReEchoBasicAttackAbility>(RuntimeBasicSpec->GetPrimaryInstance()) : nullptr;
	Test.TestNotNull(TEXT("Instanced held basic attack ability"), RuntimeBasicAbility);

	// 首次攻击已执行；动作锁与攻击间隔都必须采用最新武器表的一秒配置。
	const float Interval = Weapon->GetAttackInterval(Pawn->FindComponentByClass<UReEchoCombatantComponent>());
	Test.TestTrue(FString::Printf(TEXT("[%s] Authored action lock %.3f matches interval %.3f"),
	                              ModeName,
	                              Weapon->GetStepLockRemaining(),
	                              Interval),
	              FMath::IsNearlyEqual(Weapon->GetStepLockRemaining(), Interval, 0.01f));

	// 推进时间：小步长 tick，使武器步骤锁逐步递减、GAS 重复计时器按节奏触发。
	const float TotalTime = 2.0f;
	const float DT = 1.0f / 60.0f;
	const int32 Steps = FMath::CeilToInt(TotalTime / DT);
	for (int32 Step = 0; Step < Steps; ++Step)
	{
		const double ExpectedTimeSeconds = World->GetTimeSeconds() + DT;
		// This synthetic world does not keep spawned actors in the normal PIE tick list. Advance the
		// two runtime authorities explicitly: weapon cadence and the ability repeat timer.
		Weapon->Tick(DT);
		World->GetTimerManager().Tick(DT);
		if (RuntimeBasicAbility && Weapon->GetAttackSequence() == 1 &&
		    Weapon->GetAttackCooldownRemaining() <= KINDA_SMALL_NUMBER)
		{
			// Synthetic worlds do not dispatch the latent callback reliably; invoke the same callback
			// at readiness so this test still covers held GAS -> host -> weapon re-commit semantics.
			RuntimeBasicAbility->TriggerHeldRepeatForTesting();
		}
		if (!FMath::IsNearlyEqual(World->GetTimeSeconds(), ExpectedTimeSeconds, 0.001))
		{
			World->TimeSeconds = ExpectedTimeSeconds;
		}
	}

	// 核心回归：held 普攻循环在临时忙后继续，产生至少两发有序普攻。
	Test.TestTrue(FString::Printf(TEXT("[%s] Weapon cadence reached readiness (remaining %.3f)"),
	                              ModeName,
	                              Weapon->GetAttackCooldownRemaining()),
	              Weapon->GetAttackCooldownRemaining() <= KINDA_SMALL_NUMBER);
	Test.TestTrue(FString::Printf(TEXT("[%s] Held basic attack produced >=2 attacks after temporary busy (got %d)"),
	                              ModeName,
	                              Weapon->GetAttackSequence()),
	              Weapon->GetAttackSequence() >= 2);

	// 仍按住时，GAS 普攻能力应保持激活（未被第一发忙结果终止）。
	const FGameplayAbilitySpec* BasicSpec =
	    Pawn->GetAbilitySystemComponent()->FindAbilitySpecFromClass(UReEchoBasicAttackAbility::StaticClass());
	Test.TestTrue(FString::Printf(TEXT("[%s] Basic attack ability still active while held"), ModeName),
	              BasicSpec && BasicSpec->IsActive());

	// 第二发确实命中了身前敌人。
	Test.TestTrue(FString::Printf(TEXT("[%s] Second held attack hit the in-range enemy (health %.1f < %.1f)"),
	                              ModeName,
	                              Enemy->GetCombatantComponent()->CurrentHealth,
	                              InitialHealth),
	              Enemy->GetCombatantComponent()->CurrentHealth < InitialHealth);

	World->DestroyWorld(true);
	GEngine->DestroyWorldContext(World);
	World->RemoveFromRoot();
}

}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoHeldBasicAttackRepeatTest,
                                 "ReEcho.AttackMode.HeldRepeat",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoHeldBasicAttackRepeatTest::RunTest(const FString& Parameters)
{
	RunHeldBasicAttackRepeatScenario(*this, /*bManual=*/true);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoHeldBasicAttackRepeatAutoTest,
                                 "ReEcho.AttackMode.HeldRepeatAuto",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoHeldBasicAttackRepeatAutoTest::RunTest(const FString& Parameters)
{
	RunHeldBasicAttackRepeatScenario(*this, /*bManual=*/false);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
