#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraEmitter.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraSpriteRendererProperties.h"
#include "NiagaraSystem.h"
#include "NiagaraVariant.h"
#include "Presentation/VFX/ReEchoCombatVfxCatalog.h"
#include "Presentation/VFX/ReEchoElementReactionVfxCatalog.h"
#include "Presentation/VFX/ReEchoCombatVfxComponent.h"
#include "Presentation/VFX/ReEchoVfxPreviewActor.h"
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
	TestEqual(TEXT("Conduct delay zero preserves simultaneous compatibility"),
	          UReEchoCombatVfxComponent::ResolveConductLinkScheduledTime(4, 0.0f),
	          0.0f);
	TestTrue(TEXT("Conduct BFS schedule preserves authoritative index order"),
	         FMath::IsNearlyEqual(UReEchoCombatVfxComponent::ResolveConductLinkScheduledTime(3, 0.2f), 0.6f));
	const FVector StartWorld(1250.0f, -375.0f, 80.0f);
	const FVector EndWorld(1550.0f, 25.0f, 180.0f);
	FVector StartParameter = FVector::ZeroVector;
	FVector EndParameter = FVector::ZeroVector;
	UReEchoCombatVfxComponent::ResolveConductLinkWorldEndpoints(StartWorld, EndWorld, StartParameter, EndParameter);
	TestTrue(TEXT("World-space Conduct preserves a non-zero source origin"),
	         StartParameter.Equals(StartWorld, KINDA_SMALL_NUMBER));
	TestTrue(TEXT("World-space Conduct preserves oblique direction and distance"),
	         EndParameter.Equals(EndWorld, KINDA_SMALL_NUMBER));
	FVector BeamStart = FVector::ZeroVector;
	FVector BeamEnd = FVector::ZeroVector;
	const FVector BeamOrigin(300.0f, -120.0f, 40.0f);
	const FVector BeamDirection(0.6f, 0.8f, 0.0f);
	UReEchoCombatVfxComponent::ResolveBossBeamWorldEndpoints(
	    BeamOrigin, BeamDirection, 750.0f, BeamStart, BeamEnd);
	TestTrue(TEXT("Boss beam starts at the authoritative attack origin"),
	         BeamStart.Equals(BeamOrigin, KINDA_SMALL_NUMBER));
	TestTrue(TEXT("Boss beam endpoint consumes the locked direction and gameplay length"),
	         BeamEnd.Equals(BeamOrigin + BeamDirection * 750.0f, KINDA_SMALL_NUMBER));
	const FReEchoVfxPlacement GoatChargingPlacement =
	    FReEchoCombatVfxCatalog::ResolvePlacement(EReEchoCombatVfxSemantic::GoatSkill02Charging);
	TestTrue(TEXT("Goat body charging preserves authored world size"),
	         GoatChargingPlacement.ScalePolicy == EReEchoVfxScalePolicy::PreserveWorldSize);
	TestTrue(TEXT("Preserved world-size VFX cancels inherited uniform owner scale"),
	         UReEchoCombatVfxComponent::ResolveAttachedScale(
	             FVector::OneVector, FVector(2.0f), true)
	             .Equals(FVector(0.5f), KINDA_SMALL_NUMBER));
	TestTrue(TEXT("Ordinary attached VFX retains its configured relative scale"),
	         UReEchoCombatVfxComponent::ResolveAttachedScale(
	             FVector(1.2f, 0.8f, 1.0f), FVector(2.0f), false)
	             .Equals(FVector(1.2f, 0.8f, 1.0f), KINDA_SMALL_NUMBER));
	const FReEchoVfxPlacement SwordPlacement =
	    FReEchoCombatVfxCatalog::ResolvePlacement(EReEchoCombatVfxSemantic::PlayerMeleeSlash);
	TestTrue(TEXT("Sword slash placement comes from its weapon profile"),
	         SwordPlacement.LocalOffset.Equals(FVector(0.0f, 0.0f, 60.0f), KINDA_SMALL_NUMBER));
	TestTrue(TEXT("Sword slash tilt comes from its weapon profile"),
	         FMath::IsNearlyEqual(SwordPlacement.LocalRotation.Roll, -45.0f, KINDA_SMALL_NUMBER));
	TestTrue(TEXT("Sword slash cancels different host scales"),
	         UReEchoCombatVfxComponent::ResolveAttachedScale(
	             SwordPlacement.Scale, FVector(2.0f), SwordPlacement.ScalePolicy == EReEchoVfxScalePolicy::PreserveWorldSize)
	             .Equals(SwordPlacement.Scale * 0.5f, KINDA_SMALL_NUMBER));
	const FVector MovedEndWorld(-240.0f, 910.0f, 25.0f);
	UReEchoCombatVfxComponent::ResolveConductLinkWorldEndpoints(
	    StartWorld, MovedEndWorld, StartParameter, EndParameter);
	TestTrue(TEXT("Moved target is recomputed rather than retaining the event snapshot"),
	         EndParameter.Equals(MovedEndWorld, KINDA_SMALL_NUMBER));
	const FString ConductPath =
	    FReEchoElementReactionVfxCatalog::ResolvePath(EReEchoElementReactionVfxSemantic::Conduct, TEXT("Enemy.Slime"));
	UNiagaraSystem* ConductSystem = LoadObject<UNiagaraSystem>(nullptr, *ConductPath);
	TestNotNull(TEXT("Conduct Niagara loads for coordinate-space contract validation"), ConductSystem);
	if (ConductSystem)
	{
		TArray<FNiagaraVariable> UserParameters;
		ConductSystem->GetExposedParameters().GetUserParameters(UserParameters);
		for (const FNiagaraVariable& Variable : UserParameters)
		{
			AddInfo(FString::Printf(TEXT("Conduct user parameter Name=%s Type=%s"),
			                        *Variable.GetName().ToString(),
			                        *Variable.GetType().GetName()));
		}
		const FNiagaraVariable* StartPositionParameter = UserParameters.FindByPredicate(
		    [](const FNiagaraVariable& Variable)
		    {
			    return Variable.GetName() == TEXT("StartPosition");
		    });
		const FNiagaraVariable* EndPositionParameter = UserParameters.FindByPredicate(
		    [](const FNiagaraVariable& Variable)
		    {
			    return Variable.GetName() == TEXT("EndPosition");
		    });
		TestTrue(TEXT("Conduct exposes exact User.StartPosition Niagara Position"),
		         StartPositionParameter &&
		             StartPositionParameter->GetType() == FNiagaraTypeDefinition::GetPositionDef());
		TestTrue(TEXT("Conduct exposes exact User.EndPosition Niagara Position"),
		         EndPositionParameter && EndPositionParameter->GetType() == FNiagaraTypeDefinition::GetPositionDef());
		UNiagaraComponent* ParameterComponent = NewObject<UNiagaraComponent>();
		ParameterComponent->SetAsset(ConductSystem);
		ParameterComponent->SetAutoActivate(false);
		ParameterComponent->SetVariablePosition(TEXT("User.StartPosition"), StartWorld);
		ParameterComponent->SetVariablePosition(TEXT("User.EndPosition"), EndWorld);
		const FNiagaraVariant StartOverride = ParameterComponent->FindParameterOverride(
		    FNiagaraVariableBase(FNiagaraTypeDefinition::GetPositionDef(), TEXT("StartPosition")));
		const FNiagaraVariant EndOverride = ParameterComponent->FindParameterOverride(
		    FNiagaraVariableBase(FNiagaraTypeDefinition::GetPositionDef(), TEXT("EndPosition")));
		TestTrue(TEXT("Position API writes Start before activation"), StartOverride.IsValid());
		TestTrue(TEXT("Position API writes End before activation"), EndOverride.IsValid());
		TestEqual(TEXT("Start Position override stores an LWC FVector"),
		          StartOverride.GetNumBytes(),
		          static_cast<int32>(sizeof(FVector)));
		TestEqual(TEXT("End Position override stores an LWC FVector"),
		          EndOverride.GetNumBytes(),
		          static_cast<int32>(sizeof(FVector)));
		if (StartOverride.IsValid() && EndOverride.IsValid() && StartOverride.GetNumBytes() == sizeof(FVector) &&
		    EndOverride.GetNumBytes() == sizeof(FVector))
		{
			FVector StartReadback;
			FVector EndReadback;
			FMemory::Memcpy(&StartReadback, StartOverride.GetBytes(), sizeof(FVector));
			FMemory::Memcpy(&EndReadback, EndOverride.GetBytes(), sizeof(FVector));
			TestTrue(TEXT("Start Position override reads back the combat target world location"),
			         StartReadback.Equals(StartWorld, KINDA_SMALL_NUMBER));
			TestTrue(TEXT("End Position override reads back the combat target world location"),
			         EndReadback.Equals(EndWorld, KINDA_SMALL_NUMBER));
		}
		for (const FNiagaraEmitterHandle& Handle : ConductSystem->GetEmitterHandles())
		{
			const FVersionedNiagaraEmitterData* EmitterData = Handle.GetEmitterData();
			TestTrue(*FString::Printf(TEXT("Conduct emitter %s exposes coordinate-space data"),
			                          *Handle.GetName().ToString()),
			         EmitterData != nullptr);
			if (EmitterData)
			{
				TestFalse(*FString::Printf(TEXT("Conduct emitter %s uses world space"), *Handle.GetName().ToString()),
				          EmitterData->bLocalSpace);
			}
		}
	}
	UWorld* TimerWorld = UWorld::CreateWorld(EWorldType::EditorPreview, false);
	AActor* TimerOwner = TimerWorld ? TimerWorld->SpawnActor<AActor>() : nullptr;
	UReEchoCombatVfxComponent* TimerComponent = TimerOwner ? NewObject<UReEchoCombatVfxComponent>(TimerOwner) : nullptr;
	if (TimerComponent)
	{
		TimerOwner->AddInstanceComponent(TimerComponent);
		TimerComponent->RegisterComponent();
		AReEchoVfxScenarioTargetActor* Target0 = TimerWorld->SpawnActor<AReEchoVfxScenarioTargetActor>();
		AReEchoVfxScenarioTargetActor* Target1 = TimerWorld->SpawnActor<AReEchoVfxScenarioTargetActor>();
		AReEchoVfxScenarioTargetActor* Target2 = TimerWorld->SpawnActor<AReEchoVfxScenarioTargetActor>();
		FReEchoElementReactionResolvedEvent TimedEvent;
		FReEchoElementReactionLink& FirstLink = TimedEvent.ReactionLinks.AddDefaulted_GetRef();
		FirstLink.SourceTarget = Target0;
		FirstLink.TargetTarget = Target1;
		FReEchoElementReactionLink& SecondLink = TimedEvent.ReactionLinks.AddDefaulted_GetRef();
		SecondLink.SourceTarget = Target1;
		SecondLink.TargetTarget = Target2;
		TimerComponent->ScheduleConductLinksForTests(TimedEvent, 0.2f);
		TestEqual(TEXT("Only delayed BFS links retain cancellable timers"),
		          TimerComponent->GetPendingConductTimerCountForTests(),
		          1);
		TimerComponent->ScheduleConductLinksForTests(TimedEvent, 0.2f);
		TestEqual(TEXT("A new reaction batch cancels instead of interleaving old links"),
		          TimerComponent->GetPendingConductTimerCountForTests(),
		          1);
		Target2->Destroy();
		TimerWorld->GetTimerManager().Tick(0.25f);
		TimerComponent->CancelConductPropagationForTests();
		TestEqual(TEXT("Explicit cleanup and EndPlay contract leave no pending Conduct timers"),
		          TimerComponent->GetPendingConductTimerCountForTests(),
		          0);
	}
	if (TimerWorld)
	{
		TimerWorld->DestroyWorld(false);
	}
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
	TestEqual(TEXT("Echo card auras render immediately below their owning character"),
	          UReEchoCombatVfxComponent::ResolveEchoAuraSortPriority(23),
	          22);
	TestNotEqual(TEXT("Water and Grass Echo auras use distinct systems"),
	             FReEchoCombatVfxCatalog::ResolvePath(EReEchoCombatVfxSemantic::EchoWaterAura),
	             FReEchoCombatVfxCatalog::ResolvePath(EReEchoCombatVfxSemantic::EchoGrassAura));
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
	const FVector FoxDashDirection = FVector(0.0f, -1.0f, 0.0f);
	const FRotator FoxDirectionRotation =
	    FReEchoCombatVfxCatalog::ResolveRotation(EReEchoCombatVfxSemantic::FoxDirection, FoxDashDirection);
	TestTrue(TEXT("Fox windup arrow points along the locked dash direction"),
	         FoxDirectionRotation.RotateVector(FVector::ForwardVector).Equals(FoxDashDirection, KINDA_SMALL_NUMBER));
	TestFalse(TEXT("Gun impact production slot resolves a configured Niagara path"),
	          FReEchoCombatVfxCatalog::ResolvePath(EReEchoCombatVfxSemantic::PlayerGunImpact).IsEmpty());

	const EReEchoCombatVfxSemantic RequiredSystems[] = {
	    EReEchoCombatVfxSemantic::RabbitCharging,
	    EReEchoCombatVfxSemantic::RabbitProjectile,
	    EReEchoCombatVfxSemantic::PlayerHurt,
	    EReEchoCombatVfxSemantic::FoxCharging,
	    EReEchoCombatVfxSemantic::FoxDirection,
	    EReEchoCombatVfxSemantic::FoxDash,
	    EReEchoCombatVfxSemantic::FoxImpact,
	    EReEchoCombatVfxSemantic::PlayerMeleeSlash,
	    EReEchoCombatVfxSemantic::PlayerScytheSlash,
	    EReEchoCombatVfxSemantic::PlayerBowFlight,
	    EReEchoCombatVfxSemantic::PlayerBowImpact,
	    EReEchoCombatVfxSemantic::PlayerGunFlight,
	    EReEchoCombatVfxSemantic::PlayerGunImpact,
	    EReEchoCombatVfxSemantic::EnemyHurt,
	    EReEchoCombatVfxSemantic::EchoWaterAura,
	    EReEchoCombatVfxSemantic::EchoGrassAura,
	    EReEchoCombatVfxSemantic::GoatSkill02Charging,
	    EReEchoCombatVfxSemantic::GoatSkill02Bullet,
	    EReEchoCombatVfxSemantic::GoatSkill02Impact,
	    EReEchoCombatVfxSemantic::GoatSkill03Charging,
	    EReEchoCombatVfxSemantic::GoatSkill03Alarming,
	    EReEchoCombatVfxSemantic::GoatSkill03Impact,
	    EReEchoCombatVfxSemantic::GoatSkill04Charging,
	    EReEchoCombatVfxSemantic::GoatSkill04Lighting,
	};
	TestEqual(TEXT("Sheep projectile flight uses the authored Skill02 bullet"),
	          FReEchoCombatVfxCatalog::ResolvePath(EReEchoCombatVfxSemantic::GoatSkill02Bullet),
	          FString(TEXT("/Game/VFX/Monster/Goat/Particle/NS_Goat_Skill02_Bullet.NS_Goat_Skill02_Bullet")));
	TestEqual(TEXT("Sheep blink telegraph uses the authored Skill03 alarming system"),
	          FReEchoCombatVfxCatalog::ResolvePath(EReEchoCombatVfxSemantic::GoatSkill03Alarming),
	          FString(TEXT("/Game/VFX/Monster/Goat/Particle/NS_Goat_Skill03_Alarming.NS_Goat_Skill03_Alarming")));
	TestEqual(TEXT("Sheep beam active window uses the authored Skill04 lighting system"),
	          FReEchoCombatVfxCatalog::ResolvePath(EReEchoCombatVfxSemantic::GoatSkill04Lighting),
	          FString(TEXT("/Game/VFX/Monster/Goat/Particle/NS_Goat_Skill04_Lighting.NS_Goat_Skill04_Lighting")));
	TestEqual(TEXT("Fox windup uses the authored charging system"),
	          FReEchoCombatVfxCatalog::ResolvePath(EReEchoCombatVfxSemantic::FoxCharging),
	          FString(TEXT("/Game/VFX/Monster/Fox/Particle/NS_Fox_Rush_Charging.NS_Fox_Rush_Charging")));
	TestEqual(TEXT("Fox windup direction uses the authored arrow system"),
	          FReEchoCombatVfxCatalog::ResolvePath(EReEchoCombatVfxSemantic::FoxDirection),
	          FString(TEXT("/Game/VFX/Monster/Fox/Particle/NS_Fox_Rush_arrow.NS_Fox_Rush_arrow")));
	TestEqual(TEXT("Fox committed dash uses the authored rush system"),
	          FReEchoCombatVfxCatalog::ResolvePath(EReEchoCombatVfxSemantic::FoxDash),
	          FString(TEXT("/Game/VFX/Monster/Fox/Particle/NS_Fox_Rush_Trail.NS_Fox_Rush_Trail")));
	TestEqual(TEXT("Fox applied hit uses the authored impact system"),
	          FReEchoCombatVfxCatalog::ResolvePath(EReEchoCombatVfxSemantic::FoxImpact),
	          FString(TEXT("/Game/VFX/Monster/Fox/Particle/NS_Fox_Rush_BeAttacked.NS_Fox_Rush_BeAttacked")));
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
		                                     Semantic == EReEchoCombatVfxSemantic::PlayerGunFlight ||
		                                     Semantic == EReEchoCombatVfxSemantic::FoxDirection ||
		                                     Semantic == EReEchoCombatVfxSemantic::FoxDash ||
		                                     Semantic == EReEchoCombatVfxSemantic::EchoWaterAura ||
		                                     Semantic == EReEchoCombatVfxSemantic::EchoGrassAura;
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
