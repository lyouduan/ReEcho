#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "NiagaraEmitter.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraSystem.h"
#include "Presentation/VFX/ReEchoCombatVfxCatalog.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoCombatVfxCatalogTest,
                                 "ReEcho.Presentation.VFX.Catalog",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoCombatVfxCatalogTest::RunTest(const FString& Parameters)
{
	const FString PlayerHurt = FReEchoCombatVfxCatalog::ResolvePath(EReEchoCombatVfxSemantic::PlayerHurt);
	const FString EnemyHurt = FReEchoCombatVfxCatalog::ResolvePath(EReEchoCombatVfxSemantic::EnemyHurt);
	TestTrue(TEXT("Player hurt uses the Rabbit-folder authority"), PlayerHurt.Contains(TEXT("/Monster/Rabbit/")));
	TestTrue(TEXT("Enemy hurt uses the Sword-folder authority"), EnemyHurt.Contains(TEXT("/People/Sword/")));
	TestNotEqual(TEXT("Same short name resolves to distinct packages"), PlayerHurt, EnemyHurt);
	TestTrue(TEXT("Long sword is melee"),
	         FReEchoCombatVfxCatalog::IsMeleeAttackPattern(TEXT("Pattern.LongSwordCombo")));
	TestTrue(TEXT("Dagger is melee"), FReEchoCombatVfxCatalog::IsMeleeAttackPattern(TEXT("Pattern.DaggerCombo")));
	TestFalse(TEXT("Staff projectile is not melee"),
	          FReEchoCombatVfxCatalog::IsMeleeAttackPattern(TEXT("Pattern.StaffProjectile")));
	const FVector LockedPlayerDirection = FVector(0.6f, 0.8f, 0.0f).GetSafeNormal();
	const FRotator RabbitProjectileRotation =
	    FReEchoCombatVfxCatalog::ResolveRotation(EReEchoCombatVfxSemantic::RabbitProjectile, LockedPlayerDirection);
	const FVector RotatedThreeBallCenterAxis = RabbitProjectileRotation
	                                               .RotateVector(FReEchoCombatVfxCatalog::ResolveAuthoredForwardAxis(
	                                                   EReEchoCombatVfxSemantic::RabbitProjectile))
	                                               .GetSafeNormal2D();
	TestTrue(TEXT("Rabbit three-ball authored center points at the locked player"),
	         RotatedThreeBallCenterAxis.Equals(LockedPlayerDirection, KINDA_SMALL_NUMBER));

	const EReEchoCombatVfxSemantic RequiredSystems[] = {
	    EReEchoCombatVfxSemantic::RabbitCharging,
	    EReEchoCombatVfxSemantic::RabbitProjectile,
	    EReEchoCombatVfxSemantic::PlayerHurt,
	    EReEchoCombatVfxSemantic::FoxCharging,
	    EReEchoCombatVfxSemantic::FoxDirection,
	    EReEchoCombatVfxSemantic::FoxDash,
	    EReEchoCombatVfxSemantic::PlayerMeleeSlash,
	    EReEchoCombatVfxSemantic::EnemyHurt,
	};
	for (const EReEchoCombatVfxSemantic Semantic : RequiredSystems)
	{
		const TCHAR* AssetPath = FReEchoCombatVfxCatalog::ResolvePath(Semantic);
		UNiagaraSystem* System = LoadObject<UNiagaraSystem>(nullptr, AssetPath);
		TestNotNull(FString::Printf(TEXT("Niagara system loads: %s"), AssetPath), System);
	}

	UNiagaraSystem* RabbitProjectileSystem = LoadObject<UNiagaraSystem>(
	    nullptr, FReEchoCombatVfxCatalog::ResolvePath(EReEchoCombatVfxSemantic::RabbitProjectile));
	if (TestNotNull(TEXT("Rabbit projectile Niagara system loads for emitter-space validation"),
	                RabbitProjectileSystem))
	{
		for (const FNiagaraEmitterHandle& EmitterHandle : RabbitProjectileSystem->GetEmitterHandles())
		{
			if (!EmitterHandle.GetIsEnabled())
			{
				continue;
			}
			const FVersionedNiagaraEmitterData* EmitterData = EmitterHandle.GetEmitterData();
			TestTrue(FString::Printf(TEXT("Rabbit projectile emitter '%s' uses local space"),
			                         *EmitterHandle.GetName().ToString()),
			         EmitterData && EmitterData->bLocalSpace);
		}
	}
	return true;
}

#endif
