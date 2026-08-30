#include "ReEchoGameMode.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/ReEchoGameplayTags.h"
#include "Camera/PlayerCameraManager.h"
#include "Combat/ReEchoCombatantComponent.h"
#include "Components/SceneComponent.h"
#include "Encounter/ReEchoEncounterDirector.h"
#include "Enemies/ReEchoEnemyLogicComponent.h"
#include "EngineUtils.h"
#include "GameFramework/MovementComponent.h"
#include "Graybox/ReEchoEnemyActor.h"
#include "Graybox/ReEchoProjectileActor.h"
#include "Kismet/GameplayStatics.h"
#include "Player/ReEchoPlayerPawn.h"
#include "Presentation/Animation2D/ReEcho2DPresentationController.h"
#include "Presentation/Enemy/ReEchoEnemyPresentationComponent.h"
#include "Presentation/Scene/ReEchoArenaCameraActor.h"
#include "Presentation/VFX/ReEchoCombatVfxComponent.h"

#if WITH_DEV_AUTOMATION_TESTS
void AReEchoGameMode::ConfigureBossTransformationForTests(AReEchoPlayerPawn* InPlayer,
                                                          AReEchoEncounterDirector* InDirector,
                                                          AReEchoArenaCameraActor* InCamera)
{
	Player = InPlayer;
	Director = InDirector;
	ArenaCameraActor = InCamera;
}
#endif

bool AReEchoGameMode::BeginBossTransformation(AReEchoEnemyActor* Boss, const int32 TargetPhase)
{
	if (BossTransformTargetPhase != 0)
	{
		return TransformingBoss == Boss && BossTransformTargetPhase == TargetPhase;
	}
	if (!IsValid(Boss) || !Player || !Director || bEncounterTransitioning || Boss->GetEnemyId() != TEXT("M_SHEEP") ||
	    (TargetPhase != 2 && TargetPhase != 3))
	{
		UE_LOG(LogTemp, Warning, TEXT("[BossTransformation] rejected phase=%d: invalid host/run state"), TargetPhase);
		return false;
	}
	const UReEchoEnemyLogicComponent* Logic = Boss->GetEnemyLogicComponent();
	// During the fatal intercept Combat health is already zero; Logic is still alive until the hook decides.
	if (!Logic || !Logic->GetSnapshot().bAlive ||
	    !Logic->GetDefinition().BossPhases.ContainsByPredicate(
	        [TargetPhase](const FReEchoBossPhaseDefinition& Phase)
	        {
		        return Phase.bEnabled && Phase.PhaseIndex == TargetPhase;
	        }))
	{
		UE_LOG(
		    LogTemp, Warning, TEXT("[BossTransformation] rejected phase=%d: dead logic or missing phase"), TargetPhase);
		return false;
	}
	if (QueuedBossTransformPhase != 0)
	{
		return QueuedTransformingBoss == Boss && QueuedBossTransformPhase == TargetPhase;
	}
	if (UGameplayStatics::IsGamePaused(this))
	{
		QueuedTransformingBoss = Boss;
		QueuedBossTransformPhase = TargetPhase;
		UE_LOG(LogTemp, Display, TEXT("[BossTransformation] queued phase=%d until world resumes"), TargetPhase);
		return true;
	}
	TransformingBoss = Boss;
	BossTransformTargetPhase = TargetPhase;
	BossTransformSettings = BossPhase3Config ? (TargetPhase == 3 ? BossPhase3Config->Phase3Presentation
	                                                             : BossPhase3Config->Phase2Presentation)
	                                         : (TargetPhase == 3 ? UReEchoBossPhase3Config::MakePhase3Presentation()
	                                                             : FReEchoBossTransformPresentation());
	BossTransformSettings.DurationSeconds = FMath::Max(0.1f, BossTransformSettings.DurationSeconds);
	BossTransformSettings.BurstSeconds =
	    FMath::Clamp(BossTransformSettings.BurstSeconds, 0.0f, BossTransformSettings.DurationSeconds);
	BossTransformSettings.CameraMoveSeconds = FMath::Max(0.01f, BossTransformSettings.CameraMoveSeconds);
	BossTransformSettings.CameraPushSeconds = FMath::Max(0.01f, BossTransformSettings.CameraPushSeconds);
	const float PushExtension =
	    FMath::Max(0.0f, BossTransformSettings.CameraPushSeconds - BossTransformSettings.CameraMoveSeconds);
	BossTransformSettings.BurstSeconds += PushExtension;
	BossTransformSettings.DurationSeconds += PushExtension;
	BossTransformSettings.BurstSeconds =
	    FMath::Max(BossTransformSettings.BurstSeconds, BossTransformSettings.CameraPushSeconds);
	BossTransformSettings.DurationSeconds =
	    FMath::Max(BossTransformSettings.DurationSeconds,
	               BossTransformSettings.BurstSeconds + BossTransformSettings.CameraMoveSeconds);
	BossTransformElapsed = 0.0f;
	BossSacrificeBornWaitSeconds = 0.0f;
	BossSacrificeTransformWaitSeconds = 0.0f;
	const UReEchoBossPhase3Config* Config =
	    BossPhase3Config ? BossPhase3Config.Get() : GetDefault<UReEchoBossPhase3Config>();
	bBossSacrificeEnabled = TargetPhase == 2 && Config->bEnablePhase2Sacrifice;
	bBossSacrificeStarted = false;
	bBossSacrificeChargeStarted = false;
	bBossSacrificeReappearing = false;
	bBossTransformVfxStarted = false;
	if (bBossSacrificeEnabled)
	{
		BossSacrificeRiseStart =
		    BossTransformSettings.CameraPushSeconds + FMath::Max(0.0f, Config->SacrificeMarkSeconds);
		BossSacrificeRiseEnd = BossSacrificeRiseStart + FMath::Max(0.01f, Config->SacrificeRiseSeconds);
		BossSacrificeReappearStart = BossSacrificeRiseEnd + FMath::Max(0.0f, Config->SacrificeHiddenSeconds);
		BossTransformSettings.BurstSeconds = BossSacrificeReappearStart;
		BossSacrificeReappearEnd = BossSacrificeReappearStart + FMath::Max(0.01f, Config->SacrificeReappearSeconds);
		BossTransformSettings.DurationSeconds = BossSacrificeReappearEnd + BossTransformSettings.CameraMoveSeconds;
	}
	bBossTransformBurst = false;
	bBossTransformCameraReturning = false;
	bBossTransformPreviousDirectorTick = Director->IsActorTickEnabled();
	Director->SetActorTickEnabled(false);
	Player->ReleaseAllBasicAttackInputs();
	if (Player->AbilitySystem)
	{
		Player->AbilitySystem->AddLooseGameplayTag(ReEchoGameplayTags::State_Menu);
		bBossTransformInputBlocked = true;
	}
	if (APlayerController* Controller = Cast<APlayerController>(Player->GetController()))
	{
		Controller->SetIgnoreMoveInput(true);
		Controller->SetIgnoreLookInput(true);
	}
	FreezeBossTransformationActors();
	Boss->PrepareBossTransformation();
	if (bBossSacrificeEnabled)
	{
		// Spawn once at the start of the camera push. Presentation ticks remain enabled for Born.
		PrepareBossSacrifice();
	}
	if (ArenaCameraActor)
	{
		ArenaCameraActor->BeginStage01To02CameraSequence(false);
		ArenaCameraActor->FocusStage01To02Target(
		    Boss, BossTransformSettings.CameraWidthRatio, BossTransformSettings.CameraPushSeconds);
	}
	UE_LOG(LogTemp,
	       Display,
	       TEXT("[BossTransformation] begin phase=%d camera=%s duration=%.2f"),
	       TargetPhase,
	       *GetNameSafe(ArenaCameraActor),
	       BossTransformSettings.DurationSeconds);
	if (!bBossSacrificeEnabled)
	{
		if (UReEchoCombatVfxComponent* Vfx = Boss->FindComponentByClass<UReEchoCombatVfxComponent>())
		{
			Vfx->BeginBossTransformationEffects(BossTransformSettings.EffectScale);
		}
		bBossTransformVfxStarted = true;
	}
	return true;
}

void AReEchoGameMode::FreezeBossTransformationActors()
{
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Actor = *It;
		UReEchoCombatantComponent* Combatant = Actor->FindComponentByClass<UReEchoCombatantComponent>();
		if (!Combatant && !Actor->IsA<AReEchoProjectileActor>())
		{
			continue;
		}
		if (!BossTransformActorTicks.Contains(Actor))
		{
			if (AReEchoEnemyActor* Enemy = Cast<AReEchoEnemyActor>(Actor))
			{
				BossTransformEnemyFreezeTimes.Add(Enemy, GetWorld()->GetTimeSeconds());
			}
			BossTransformActorTicks.Add(Actor, Actor->IsActorTickEnabled());
			Actor->SetActorTickEnabled(false);
		}
		if (Combatant && !BossTransformCombatGates.Contains(Combatant))
		{
			BossTransformCombatGates.Add(Combatant, Combatant->IsPresentationSuspended());
			Combatant->SetPresentationSuspended(true);
		}
		TInlineComponentArray<UActorComponent*> Components(Actor);
		for (UActorComponent* Component : Components)
		{
			// Keep scene/animation/Niagara and read-only presentation running. Stop gameplay and movement ticks.
			if (Component->IsA<USceneComponent>() || Component->IsA<UReEchoCombatVfxComponent>() ||
			    Component->IsA<UReEchoEnemyPresentationComponent>() ||
			    Component->IsA<UReEcho2DPresentationController>())
			{
				continue;
			}
			if (!BossTransformComponentTicks.Contains(Component))
			{
				BossTransformComponentTicks.Add(Component, Component->IsComponentTickEnabled());
				Component->SetComponentTickEnabled(false);
			}
		}
	}
}

void AReEchoGameMode::AdvanceBossTransformation(const float DeltaSeconds)
{
	if (UGameplayStatics::IsGamePaused(this))
	{
		return;
	}
	if (BossTransformTargetPhase == 0 && QueuedBossTransformPhase != 0)
	{
		AReEchoEnemyActor* QueuedBoss = QueuedTransformingBoss.Get();
		const int32 QueuedPhase = QueuedBossTransformPhase;
		QueuedTransformingBoss.Reset();
		QueuedBossTransformPhase = 0;
		// Revalidate after resuming: the host or encounter may have changed while paused.
		BeginBossTransformation(QueuedBoss, QueuedPhase);
		return;
	}
	if (BossTransformTargetPhase == 0)
	{
		return;
	}
	AReEchoEnemyActor* Boss = TransformingBoss.Get();
	if (!Boss || !Boss->IsAlive() || !Player || !Player->Combatant || !Player->Combatant->IsAlive())
	{
		EndBossTransformation(false);
		return;
	}
	FreezeBossTransformationActors();
	if (bBossSacrificeEnabled && !bBossSacrificeStarted)
	{
		bool bGroundReady = true;
		for (const TWeakObjectPtr<AReEchoEnemyActor>& Weak : BossSacrificeEnemies)
		{
			if (AReEchoEnemyActor* Enemy = Weak.Get())
			{
				if (UReEchoEnemyPresentationComponent* Visual =
				        Enemy->FindComponentByClass<UReEchoEnemyPresentationComponent>())
				{
					bGroundReady &= Visual->PrepareSacrificeGroundPose();
				}
			}
		}
		if (!bGroundReady &&
		    BossTransformElapsed + FMath::Max(0.0f, DeltaSeconds) >= BossTransformSettings.CameraPushSeconds)
		{
			BossTransformElapsed = BossTransformSettings.CameraPushSeconds;
			BossSacrificeBornWaitSeconds += FMath::Max(0.0f, DeltaSeconds);
			if (BossSacrificeBornWaitSeconds >= 3.0f)
			{
				UE_LOG(LogTemp,
				       Warning,
				       TEXT("[BossSacrifice] Born did not complete; cancelling cinematic without forcing takeoff"));
				EndBossTransformation(false);
			}
			return;
		}
	}
	BossTransformElapsed += FMath::Max(0.0f, DeltaSeconds);
	if (bBossSacrificeEnabled && bBossSacrificeStarted && BossTransformElapsed >= BossSacrificeReappearStart &&
	    !bBossSacrificeReappearing)
	{
		bool bTransformsComplete = true;
		for (const TWeakObjectPtr<AReEchoEnemyActor>& Weak : BossSacrificeEnemies)
		{
			if (AReEchoEnemyActor* Enemy = Weak.Get())
			{
				if (const UReEchoEnemyPresentationComponent* Visual =
				        Enemy->FindComponentByClass<UReEchoEnemyPresentationComponent>())
				{
					bTransformsComplete &= Visual->IsSacrificeTransformComplete();
				}
			}
		}
		if (!bTransformsComplete)
		{
			const float Extension = BossTransformElapsed - BossSacrificeReappearStart + KINDA_SMALL_NUMBER;
			BossSacrificeReappearStart += Extension;
			BossSacrificeReappearEnd += Extension;
			BossTransformSettings.DurationSeconds += Extension;
			BossSacrificeTransformWaitSeconds += FMath::Max(0.0f, DeltaSeconds);
			// Keep the sheep's burst time unchanged; only crowd descent and camera return wait.
			if (BossSacrificeTransformWaitSeconds >= 10.0f)
			{
				UE_LOG(LogTemp, Warning, TEXT("[BossSacrifice] transformation playback stalled; cancelling cinematic"));
				EndBossTransformation(false);
				return;
			}
		}
	}
	if (!bBossTransformVfxStarted && BossTransformElapsed >= BossTransformSettings.CameraPushSeconds)
	{
		bBossTransformVfxStarted = true;
		if (UReEchoCombatVfxComponent* Vfx = Boss->FindComponentByClass<UReEchoCombatVfxComponent>())
		{
			Vfx->BeginBossTransformationEffects(BossTransformSettings.EffectScale);
		}
	}
	UpdateBossSacrifice();
	if (!bBossTransformBurst && BossTransformElapsed >= BossTransformSettings.BurstSeconds)
	{
		bBossTransformBurst = true;
		if (!Boss->CommitCinematicBossPhase(BossTransformTargetPhase))
		{
			EndBossTransformation(false);
			return;
		}
		if (UReEchoCombatVfxComponent* Vfx = Boss->FindComponentByClass<UReEchoCombatVfxComponent>())
		{
			Vfx->BurstBossTransformationEffects(BossTransformSettings.EffectScale);
		}
	}
	const float Remaining = BossTransformSettings.DurationSeconds - BossTransformElapsed;
	if (!bBossTransformCameraReturning && bBossTransformBurst && Remaining <= BossTransformSettings.CameraMoveSeconds)
	{
		bBossTransformCameraReturning = true;
		if (ArenaCameraActor)
		{
			ArenaCameraActor->FocusStage01To02TargetAtStandardWidth(Player, FMath::Max(0.0f, Remaining));
		}
	}
	if (APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		const float Envelope = FMath::Min(FMath::Clamp(BossTransformElapsed / 0.2f, 0.0f, 1.0f),
		                                  FMath::Clamp(Remaining / 0.2f, 0.0f, 1.0f));
		const float BurstAge = BossTransformElapsed - BossTransformSettings.BurstSeconds;
		const bool bFlash = bBossTransformBurst && BurstAge >= 0.0f && BurstAge < 0.08f;
		Camera->SetManualCameraFade(bFlash ? 0.18f * (1.0f - BurstAge / 0.08f)
		                                   : BossTransformSettings.DarkenAmount * Envelope,
		                            bFlash ? FLinearColor(0.7f, 0.6f, 1.0f) : FLinearColor::Black,
		                            false);
	}
	if (Remaining <= 0.0f)
	{
		EndBossTransformation(true);
	}
}

void AReEchoGameMode::EndBossTransformation(const bool bCompleted)
{
	QueuedTransformingBoss.Reset();
	QueuedBossTransformPhase = 0;
	if (BossTransformTargetPhase == 0)
	{
		return;
	}
	const int32 CompletedPhase = BossTransformTargetPhase;
	EndBossSacrifice();
	UE_LOG(LogTemp, Display, TEXT("[BossTransformation] end phase=%d completed=%d"), CompletedPhase, bCompleted);
	BossTransformTargetPhase = 0;
	if (AReEchoEnemyActor* Boss = TransformingBoss.Get())
	{
		if (UReEchoCombatVfxComponent* Vfx = Boss->FindComponentByClass<UReEchoCombatVfxComponent>())
		{
			Vfx->EndBossTransformationEffects();
		}
	}
	for (const auto& Entry : BossTransformEnemyFreezeTimes)
	{
		if (AReEchoEnemyActor* Enemy = Entry.Key.Get())
		{
			Enemy->CompensateBossTransformationPause(Entry.Value);
		}
	}
	BossTransformEnemyFreezeTimes.Reset();
	for (const auto& Entry : BossTransformComponentTicks)
	{
		if (UActorComponent* Component = Entry.Key.Get())
		{
			Component->SetComponentTickEnabled(Entry.Value);
		}
	}
	for (const auto& Entry : BossTransformActorTicks)
	{
		if (AActor* Actor = Entry.Key.Get())
		{
			Actor->SetActorTickEnabled(Entry.Value);
		}
	}
	for (const auto& Entry : BossTransformCombatGates)
	{
		if (UReEchoCombatantComponent* Combatant = Entry.Key.Get())
		{
			Combatant->SetPresentationSuspended(Entry.Value);
		}
	}
	BossTransformComponentTicks.Reset();
	BossTransformActorTicks.Reset();
	BossTransformCombatGates.Reset();
	if (Director)
	{
		Director->SetActorTickEnabled(bBossTransformPreviousDirectorTick);
	}
	if (Player)
	{
		if (bBossTransformInputBlocked && Player->AbilitySystem)
		{
			Player->AbilitySystem->RemoveLooseGameplayTag(ReEchoGameplayTags::State_Menu);
		}
		if (APlayerController* Controller = Cast<APlayerController>(Player->GetController()))
		{
			Controller->SetIgnoreMoveInput(false);
			Controller->SetIgnoreLookInput(false);
		}
	}
	bBossTransformInputBlocked = false;
	TransformingBoss.Reset();
	if (ArenaCameraActor)
	{
		ArenaCameraActor->EndStage01To02CameraSequence();
		if (bCompleted)
		{
			ArenaCameraActor->PlayImpactShake(BossTransformSettings.ShakeAmplitudeCm, 0.25f);
		}
	}
	if (APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		Camera->StopCameraFade();
	}
	if (bCompleted && CompletedPhase == 3)
	{
		ShowBossPhase3Announcement();
	}
}

void AReEchoGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	EndBossTransformation(false);
	Super::EndPlay(EndPlayReason);
}
