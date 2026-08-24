#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraEmitter.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraSpriteRendererProperties.h"
#include "NiagaraSystem.h"
#include "Presentation/VFX/ReEchoCombatVfxCatalog.h"
#include "Presentation/VFX/ReEchoElementReactionVfxCatalog.h"
#include "Presentation/VFX/ReEchoCombatVfxComponent.h"
#include "Graybox/ReEchoProjectileActor.h"
#include "Graybox/ReEchoSwordArcActor.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoCombatVfxCatalogTest,
                                 "ReEcho.Presentation.VFX.Catalog",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoCombatVfxCatalogTest::RunTest(const FString& Parameters)
{
	TestNotEqual(
	    TEXT("Ordered Enhance reactions use distinct entered-element Niagara"),
	    FString(FReEchoElementReactionVfxCatalog::ResolvePath(EReEchoElementReactionVfxSemantic::EnhanceGrass)),
	    FString(FReEchoElementReactionVfxCatalog::ResolvePath(EReEchoElementReactionVfxSemantic::EnhanceWater)));
	TestNotEqual(TEXT("Rabbit and Fox burn body variants are distinct"),
	             FString(FReEchoElementReactionVfxCatalog::ResolvePath(EReEchoElementReactionVfxSemantic::Burn,
	                                                                   TEXT("Enemy.Rabbit"))),
	             FString(FReEchoElementReactionVfxCatalog::ResolvePath(EReEchoElementReactionVfxSemantic::Burn,
	                                                                   TEXT("Enemy.Fox"))));
	TestTrue(TEXT("Burn visual lifetime follows the timed Burn status"),
	         UReEchoCombatVfxComponent::IsElementReactionStateDriven(TEXT("Y_ER_F_G")));
	TestTrue(TEXT("Growth visual lifetime follows authoritative Grass attachments"),
	         UReEchoCombatVfxComponent::IsElementReactionStateDriven(TEXT("Y_ER_L_G")));
	TestFalse(TEXT("Vaporize remains a target-bound one-shot"),
	          UReEchoCombatVfxComponent::IsElementReactionStateDriven(TEXT("Y_ER_F_W")));
	const FName DebugReactionNames[] = {
	    TEXT("Burn"), TEXT("Vaporize"), TEXT("Growth"), TEXT("Conduct"), TEXT("EnhanceGrass"), TEXT("EnhanceWater")};
	for (const FName DebugReactionName : DebugReactionNames)
	{
		uint8 SemanticValue = 0;
		TestTrue(FString::Printf(TEXT("GM reaction '%s' resolves directly to VFX"), *DebugReactionName.ToString()),
		         UReEchoCombatVfxComponent::TryResolveDebugElementReactionSemantic(DebugReactionName, SemanticValue));
	}
	uint8 InvalidSemanticValue = 0;
	TestFalse(TEXT("Unknown GM reaction does not spawn an unrelated VFX"),
	          UReEchoCombatVfxComponent::TryResolveDebugElementReactionSemantic(TEXT("Unknown"), InvalidSemanticValue));
	const FString PlayerHurt = FReEchoCombatVfxCatalog::ResolvePath(EReEchoCombatVfxSemantic::PlayerHurt);
	const FString EnemyHurt = FReEchoCombatVfxCatalog::ResolvePath(EReEchoCombatVfxSemantic::EnemyHurt);
	TestTrue(TEXT("Player hurt uses the Rabbit-folder authority"), PlayerHurt.Contains(TEXT("/Monster/Rabbit/")));
	TestTrue(TEXT("Enemy hurt uses the Sword-folder authority"), EnemyHurt.Contains(TEXT("/People/Sword/")));
	TestNotEqual(TEXT("Same short name resolves to distinct packages"), PlayerHurt, EnemyHurt);
	TestTrue(TEXT("Long sword is melee"),
	         FReEchoCombatVfxCatalog::IsMeleeAttackPattern(TEXT("Pattern.LongSwordCombo")));
	TestTrue(TEXT("Scythe uses its dedicated melee Niagara"),
	         FReEchoCombatVfxCatalog::IsMeleeAttackPattern(TEXT("Pattern.ScytheSweep")));
	TestFalse(TEXT("Whip does not reuse the longsword Niagara"),
	          FReEchoCombatVfxCatalog::IsMeleeAttackPattern(TEXT("Pattern.WhipCombo")));
	TestFalse(TEXT("Staff projectile is not melee"),
	          FReEchoCombatVfxCatalog::IsMeleeAttackPattern(TEXT("Pattern.StaffProjectile")));
	TestTrue(TEXT("Whip safely omits its unavailable legacy attack texture"),
	         AReEchoSwordArcActor::ResolveWeaponTexturePath(TEXT("Whip")).IsEmpty());
	TestTrue(TEXT("Bow no longer resolves a legacy projectile texture"),
	         AReEchoProjectileActor::ResolveWeaponTexturePath(TEXT("Bow")).IsEmpty());
	TestTrue(TEXT("Gun no longer resolves a legacy projectile texture"),
	         AReEchoProjectileActor::ResolveWeaponTexturePath(TEXT("Gun")).IsEmpty());
	TestTrue(TEXT("Canonical staff projectile reuses the existing light-wave art contract"),
	         AReEchoProjectileActor::ResolveWeaponTexturePath(TEXT("Staff")).Contains(TEXT("StaffLightWave")));
	TestEqual(TEXT("Combat effects use the global foreground band above ordinary actors"),
	          UReEchoCombatVfxComponent::ResolveCombatEffectSortPriority(23),
	          1000);
	TestEqual(TEXT("Combat effects still render above an owner already beyond the foreground band"),
	          UReEchoCombatVfxComponent::ResolveCombatEffectSortPriority(1500),
	          1501);
	FReEchoEnemyProjectileEvent BallZero;
	BallZero.Attack.Sequence = 17;
	BallZero.VolleyBallIndex = 0;
	FReEchoEnemyProjectileEvent BallOne = BallZero;
	BallOne.VolleyBallIndex = 1;
	const FReEchoProjectileVisualKey BallZeroKey = UReEchoCombatVfxComponent::ResolveProjectileVisualKey(BallZero);
	const FReEchoProjectileVisualKey BallOneKey = UReEchoCombatVfxComponent::ResolveProjectileVisualKey(BallOne);
	TestTrue(TEXT("Same committed volley and same ball resolve a stable visual key"),
	         BallZeroKey == UReEchoCombatVfxComponent::ResolveProjectileVisualKey(BallZero));
	TestFalse(TEXT("Two balls in one committed volley cannot overwrite the same visual"), BallZeroKey == BallOneKey);
	TestEqual(TEXT("Rabbit material core diameter matches the gameplay collider"),
	          UReEchoCombatVfxComponent::ResolveProjectileCoreDiameter(50.0f),
	          100.0f);
	TestEqual(TEXT("Rabbit additive glow is larger without changing collision"),
	          UReEchoCombatVfxComponent::ResolveProjectileGlowDiameter(50.0f),
	          150.0f);
	const FVector LockedPlayerDirection = FVector(0.6f, 0.8f, 0.0f).GetSafeNormal();
	const FRotator RabbitProjectileRotation =
	    FReEchoCombatVfxCatalog::ResolveRotation(EReEchoCombatVfxSemantic::RabbitProjectile, LockedPlayerDirection);
	const FVector RotatedThreeBallCenterAxis = RabbitProjectileRotation
	                                               .RotateVector(FReEchoCombatVfxCatalog::ResolveAuthoredForwardAxis(
	                                                   EReEchoCombatVfxSemantic::RabbitProjectile))
	                                               .GetSafeNormal2D();
	TestTrue(TEXT("Rabbit three-ball authored center points at the locked player"),
	         RotatedThreeBallCenterAxis.Equals(LockedPlayerDirection, KINDA_SMALL_NUMBER));
	const FVector BowTargetDirection = FVector(-0.8f, 0.6f, 0.0f).GetSafeNormal();
	const FVector BowAuthoredForwardAxis =
	    FReEchoCombatVfxCatalog::ResolveAuthoredForwardAxis(EReEchoCombatVfxSemantic::PlayerBowFlight);
	TestTrue(TEXT("Bow delivered Niagara arrowhead is authored along local positive Y"),
	         BowAuthoredForwardAxis.Equals(FVector::RightVector, KINDA_SMALL_NUMBER));
	const FRotator BowFlightRotation =
	    FReEchoCombatVfxCatalog::ResolveRotation(EReEchoCombatVfxSemantic::PlayerBowFlight, BowTargetDirection);
	const FVector RotatedBowAuthoredAxis = BowFlightRotation.RotateVector(BowAuthoredForwardAxis).GetSafeNormal2D();
	TestTrue(TEXT("Bow authored arrow axis points from the shooter toward the target"),
	         RotatedBowAuthoredAxis.Equals(BowTargetDirection, KINDA_SMALL_NUMBER));
	TestFalse(TEXT("Gun impact production slot resolves a configured Niagara path"),
	          FReEchoCombatVfxCatalog::ResolvePath(EReEchoCombatVfxSemantic::PlayerGunImpact).IsEmpty());

	const EReEchoCombatVfxSemantic RequiredSystems[] = {
	    EReEchoCombatVfxSemantic::RabbitCharging,
	    EReEchoCombatVfxSemantic::RabbitProjectile,
	    EReEchoCombatVfxSemantic::PlayerHurt,
	    EReEchoCombatVfxSemantic::FoxCharging,
	    EReEchoCombatVfxSemantic::FoxDirection,
	    EReEchoCombatVfxSemantic::FoxDash,
	    EReEchoCombatVfxSemantic::PlayerMeleeSlash,
	    EReEchoCombatVfxSemantic::PlayerScytheSlash,
	    EReEchoCombatVfxSemantic::PlayerBowFlight,
	    EReEchoCombatVfxSemantic::PlayerBowImpact,
	    EReEchoCombatVfxSemantic::PlayerGunFlight,
	    EReEchoCombatVfxSemantic::PlayerGunImpact,
	    EReEchoCombatVfxSemantic::EnemyHurt,
	};
	for (const EReEchoCombatVfxSemantic Semantic : RequiredSystems)
	{
		const FString AssetPath = FReEchoCombatVfxCatalog::ResolvePath(Semantic);
		UNiagaraSystem* System = LoadObject<UNiagaraSystem>(nullptr, *AssetPath);
		if (!TestNotNull(FString::Printf(TEXT("Niagara system loads: %s"), *AssetPath), System))
		{
			continue;
		}
		const bool bRequiresComponentSpace = Semantic == EReEchoCombatVfxSemantic::PlayerMeleeSlash ||
		                                     Semantic == EReEchoCombatVfxSemantic::PlayerScytheSlash ||
		                                     Semantic == EReEchoCombatVfxSemantic::PlayerBowFlight ||
		                                     Semantic == EReEchoCombatVfxSemantic::PlayerGunFlight;
		if (bRequiresComponentSpace)
		{
			int32 BowSpriteRendererCount = 0;
			for (const FNiagaraEmitterHandle& EmitterHandle : System->GetEmitterHandles())
			{
				if (!EmitterHandle.GetIsEnabled())
				{
					continue;
				}
				const FVersionedNiagaraEmitterData* EmitterData = EmitterHandle.GetEmitterData();
				if (Semantic == EReEchoCombatVfxSemantic::PlayerBowFlight && EmitterData)
				{
					for (const UNiagaraRendererProperties* Renderer : EmitterData->GetRenderers())
					{
						if (const UNiagaraSpriteRendererProperties* Sprite =
						        Cast<UNiagaraSpriteRendererProperties>(Renderer))
						{
							++BowSpriteRendererCount;
							TestEqual(TEXT("Bow sprite renderer preserves its authored alignment"),
							          Sprite->Alignment,
							          ENiagaraSpriteAlignment::Automatic);
						}
					}
				}
				TestTrue(FString::Printf(TEXT("Weapon VFX emitter '%s' uses local space"),
				                         *EmitterHandle.GetName().ToString()),
				         EmitterData && EmitterData->bLocalSpace);
			}
			if (Semantic == EReEchoCombatVfxSemantic::PlayerBowFlight)
			{
				TestEqual(TEXT("Bow flight keeps its three authored sprite layers"), BowSpriteRendererCount, 3);
			}
		}
	}
	const TCHAR* FireSystems[] = {
	    FReEchoElementReactionVfxCatalog::ResolvePath(EReEchoElementReactionVfxSemantic::Burn, TEXT("Enemy.Slime")),
	    FReEchoElementReactionVfxCatalog::ResolvePath(EReEchoElementReactionVfxSemantic::Burn, TEXT("Enemy.Rabbit")),
	    FReEchoElementReactionVfxCatalog::ResolvePath(EReEchoElementReactionVfxSemantic::Burn, TEXT("Enemy.Fox")),
	};
	for (const TCHAR* FirePath : FireSystems)
	{
		UNiagaraSystem* FireSystem = LoadObject<UNiagaraSystem>(nullptr, FirePath);
		if (!TestNotNull(FString::Printf(TEXT("Fire Niagara loads: %s"), FirePath), FireSystem))
		{
			continue;
		}
		for (const FNiagaraEmitterHandle& EmitterHandle : FireSystem->GetEmitterHandles())
		{
			if (EmitterHandle.GetIsEnabled())
			{
				const FVersionedNiagaraEmitterData* EmitterData = EmitterHandle.GetEmitterData();
				TestTrue(FString::Printf(TEXT("Attached Fire emitter '%s' uses local space"),
				                         *EmitterHandle.GetName().ToString()),
				         EmitterData && EmitterData->bLocalSpace);
			}
		}
	}
	TestNotNull(TEXT("Logic-driven rabbit projectile texture loads"),
	            LoadObject<UTexture2D>(nullptr, FReEchoCombatVfxCatalog::ResolveRabbitProjectileTexturePath()));
	TestNotNull(
	    TEXT("Logic-driven rabbit projectile keeps the authored emissive material"),
	    LoadObject<UMaterialInterface>(nullptr, FReEchoCombatVfxCatalog::ResolveRabbitProjectileMaterialPath()));
	for (int32 GlowLayerIndex = 0; GlowLayerIndex < FReEchoCombatVfxCatalog::GetRabbitProjectileGlowMaterialCount();
	     ++GlowLayerIndex)
	{
		const TCHAR* GlowPath = FReEchoCombatVfxCatalog::ResolveRabbitProjectileGlowMaterialPath(GlowLayerIndex);
		TestNotNull(FString::Printf(TEXT("Rabbit additive glow material loads: %s"), GlowPath),
		            LoadObject<UMaterialInterface>(nullptr, GlowPath));
	}

	UNiagaraSystem* RabbitProjectileSystem = LoadObject<UNiagaraSystem>(
	    nullptr, *FReEchoCombatVfxCatalog::ResolvePath(EReEchoCombatVfxSemantic::RabbitProjectile));
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
