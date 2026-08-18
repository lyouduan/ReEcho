#include "Presentation/VFX/ReEchoCombatVfxComponent.h"

#include "Graybox/ReEchoEnemyActor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "NiagaraComponent.h"
#include "NiagaraDataSet.h"
#include "NiagaraDataSetAccessor.h"
#include "NiagaraEmitterInstance.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "NiagaraSystemInstance.h"
#include "NiagaraSystemInstanceController.h"
#include "Presentation/Animation2D/ReEcho2DAnimationComponent.h"
#include "Presentation/VFX/ReEchoCombatVfxCatalog.h"
#include "ReEcho.h"
#include "TimerManager.h"

namespace ReEchoCombatVfx
{
constexpr int32 CombatEffectSortOffset = 1;
constexpr int32 TrajectoryLogEventStride = 6;
constexpr int32 TrajectoryLogMaximumEventCount = 60;

void LogLayerState(const AActor* Owner,
                   const USceneComponent* AttachmentRoot,
                   const UNiagaraComponent* Effect,
                   const EReEchoCombatVfxSemantic Semantic,
                   const TCHAR* SpawnMode,
                   const TCHAR* Phase)
{
	const UReEcho2DAnimationComponent* Animation =
	    Owner ? Owner->FindComponentByClass<UReEcho2DAnimationComponent>() : nullptr;
	const int32 OwnerPriority = Animation ? Animation->TranslucencySortPriority : INDEX_NONE;
	const int32 EffectPriority = Effect ? Effect->TranslucencySortPriority : INDEX_NONE;
	const float OwnerDistanceOffset = Animation ? Animation->TranslucencySortDistanceOffset : 0.0f;
	const float EffectDistanceOffset = Effect ? Effect->TranslucencySortDistanceOffset : 0.0f;
	const USceneComponent* EffectParent = Effect ? Effect->GetAttachParent() : nullptr;
	const USceneComponent* AttachmentParent = AttachmentRoot ? AttachmentRoot->GetAttachParent() : nullptr;
	UE_LOG(LogReEcho,
	       Warning,
	       TEXT("[CombatVfxLayerTrace] Phase=%s Mode=%s Semantic=%s Owner=%s Animation=%s "
	            "OwnerPriority=%d OwnerDistanceOffset=%.2f OwnerWorld=%s AnimationWorld=%s "
	            "Anchor=%s AnchorParent=%s AnchorWorld=%s Effect=%s EffectParent=%s EffectPriority=%d "
	            "EffectDistanceOffset=%.2f EffectWorld=%s Registered=%d Active=%d Visible=%d PriorityDelta=%d"),
	       Phase,
	       SpawnMode,
	       FReEchoCombatVfxCatalog::ResolvePath(Semantic),
	       *GetNameSafe(Owner),
	       *GetNameSafe(Animation),
	       OwnerPriority,
	       OwnerDistanceOffset,
	       Owner ? *Owner->GetActorLocation().ToCompactString() : TEXT("<none>"),
	       Animation ? *Animation->GetComponentLocation().ToCompactString() : TEXT("<none>"),
	       *GetNameSafe(AttachmentRoot),
	       *GetNameSafe(AttachmentParent),
	       AttachmentRoot ? *AttachmentRoot->GetComponentLocation().ToCompactString() : TEXT("<none>"),
	       *GetNameSafe(Effect),
	       *GetNameSafe(EffectParent),
	       EffectPriority,
	       EffectDistanceOffset,
	       Effect ? *Effect->GetComponentLocation().ToCompactString() : TEXT("<none>"),
	       Effect && Effect->IsRegistered(),
	       Effect && Effect->IsActive(),
	       Effect && Effect->IsVisible(),
	       OwnerPriority != INDEX_NONE && EffectPriority != INDEX_NONE ? EffectPriority - OwnerPriority : INDEX_NONE);
}

void LogLayerStateNowAndDelayed(UWorld* World,
                                AActor* Owner,
                                USceneComponent* AttachmentRoot,
                                UNiagaraComponent* Effect,
                                const EReEchoCombatVfxSemantic Semantic,
                                const TCHAR* SpawnMode)
{
	LogLayerState(Owner, AttachmentRoot, Effect, Semantic, SpawnMode, TEXT("Immediate"));
	if (!World || !Effect)
	{
		return;
	}

	const TWeakObjectPtr<AActor> WeakOwner(Owner);
	const TWeakObjectPtr<USceneComponent> WeakAttachmentRoot(AttachmentRoot);
	const TWeakObjectPtr<UNiagaraComponent> WeakEffect(Effect);
	const FString StableSpawnMode(SpawnMode);
	FTimerHandle TimerHandle;
	World->GetTimerManager().SetTimer(TimerHandle,
	                                  FTimerDelegate::CreateLambda(
	                                      [WeakOwner, WeakAttachmentRoot, WeakEffect, Semantic, StableSpawnMode]()
	                                      {
		                                      LogLayerState(WeakOwner.Get(),
		                                                    WeakAttachmentRoot.Get(),
		                                                    WeakEffect.Get(),
		                                                    Semantic,
		                                                    *StableSpawnMode,
		                                                    TEXT("Delayed"));
	                                      }),
	                                  0.1f,
	                                  false);
}
} // namespace ReEchoCombatVfx

UReEchoCombatVfxComponent::UReEchoCombatVfxComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

int32 UReEchoCombatVfxComponent::ResolveCombatEffectSortPriority(const int32 OwnerSortPriority)
{
	return OwnerSortPriority + ReEchoCombatVfx::CombatEffectSortOffset;
}

void UReEchoCombatVfxComponent::ConfigureAttachmentRoots(USceneComponent* InAttackVfxRoot,
                                                         USceneComponent* InHurtVfxRoot)
{
	AttackVfxRoot = InAttackVfxRoot;
	HurtVfxRoot = InHurtVfxRoot;
}

void UReEchoCombatVfxComponent::BeginPlay()
{
	Super::BeginPlay();
	AActor* Owner = GetOwner();
	BindEventSources(Owner ? Owner->FindComponentByClass<UReEchoCombatEventsComponent>() : nullptr,
	                 Owner ? Owner->FindComponentByClass<UReEchoEnemyEventsComponent>() : nullptr);
}

void UReEchoCombatVfxComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	BindEventSources(nullptr, nullptr);
	StopAllEffects();
	Super::EndPlay(EndPlayReason);
}

void UReEchoCombatVfxComponent::BindEventSources(UReEchoCombatEventsComponent* InCombatEvents,
                                                 UReEchoEnemyEventsComponent* InEnemyEvents)
{
	if (CombatEvents)
	{
		CombatEvents->OnAttackCommitted.RemoveAll(this);
		CombatEvents->OnHurt.RemoveAll(this);
		CombatEvents->OnDeath.RemoveAll(this);
	}
	if (EnemyEvents)
	{
		EnemyEvents->OnSpecialAction.RemoveAll(this);
		EnemyEvents->OnProjectile.RemoveAll(this);
	}
	CombatEvents = InCombatEvents;
	EnemyEvents = InEnemyEvents;
	if (CombatEvents)
	{
		CombatEvents->OnAttackCommitted.AddDynamic(this, &UReEchoCombatVfxComponent::HandleAttackCommitted);
		CombatEvents->OnHurt.AddDynamic(this, &UReEchoCombatVfxComponent::HandleHurt);
		CombatEvents->OnDeath.AddDynamic(this, &UReEchoCombatVfxComponent::HandleDeath);
	}
	if (EnemyEvents)
	{
		EnemyEvents->OnSpecialAction.AddDynamic(this, &UReEchoCombatVfxComponent::HandleSpecialAction);
		EnemyEvents->OnProjectile.AddDynamic(this, &UReEchoCombatVfxComponent::HandleProjectile);
	}
}

UNiagaraSystem* UReEchoCombatVfxComponent::ResolveSystem(const uint8 SemanticValue) const
{
	const EReEchoCombatVfxSemantic Semantic = static_cast<EReEchoCombatVfxSemantic>(SemanticValue);
	UNiagaraSystem* System = LoadObject<UNiagaraSystem>(nullptr, FReEchoCombatVfxCatalog::ResolvePath(Semantic));
	if (!System && !MissingSystemWarnings.Contains(SemanticValue))
	{
		MissingSystemWarnings.Add(SemanticValue);
		UE_LOG(LogReEcho,
		       Warning,
		       TEXT("[VFX] Missing semantic asset '%s'; gameplay continues without it"),
		       FReEchoCombatVfxCatalog::ResolvePath(Semantic));
	}
	return System;
}

UNiagaraComponent* UReEchoCombatVfxComponent::SpawnWorld(const uint8 SemanticValue,
                                                         const FVector& Location,
                                                         const FVector& Direction,
                                                         const bool bAutoDestroy) const
{
	const EReEchoCombatVfxSemantic Semantic = static_cast<EReEchoCombatVfxSemantic>(SemanticValue);
	UNiagaraSystem* System = ResolveSystem(SemanticValue);
	UWorld* World = GetWorld();
	if (!System || !World)
	{
		return nullptr;
	}
	UNiagaraComponent* Effect =
	    UNiagaraFunctionLibrary::SpawnSystemAtLocation(World,
	                                                   System,
	                                                   Location,
	                                                   FReEchoCombatVfxCatalog::ResolveRotation(Semantic, Direction),
	                                                   FVector::OneVector,
	                                                   bAutoDestroy,
	                                                   true,
	                                                   ENCPoolMethod::None,
	                                                   true);
	if (Effect)
	{
		Effect->SetTranslucentSortPriority(ResolveOwnerSortPriority());
		ReEchoCombatVfx::LogLayerStateNowAndDelayed(World, GetOwner(), nullptr, Effect, Semantic, TEXT("World"));
	}
	return Effect;
}

UNiagaraComponent* UReEchoCombatVfxComponent::SpawnAttached(const uint8 SemanticValue,
                                                            const FVector& Direction,
                                                            USceneComponent* AttachmentRoot,
                                                            const bool bAutoDestroy) const
{
	UNiagaraSystem* System = ResolveSystem(SemanticValue);
	if (!System || !AttachmentRoot)
	{
		return nullptr;
	}
	UNiagaraComponent* Effect = UNiagaraFunctionLibrary::SpawnSystemAttached(
	    System,
	    AttachmentRoot,
	    NAME_None,
	    FVector::ZeroVector,
	    FReEchoCombatVfxCatalog::ResolveRotation(static_cast<EReEchoCombatVfxSemantic>(SemanticValue), Direction),
	    FVector::OneVector,
	    EAttachLocation::KeepRelativeOffset,
	    bAutoDestroy,
	    ENCPoolMethod::None,
	    true);
	if (Effect)
	{
		Effect->SetTranslucentSortPriority(ResolveOwnerSortPriority());
		ReEchoCombatVfx::LogLayerStateNowAndDelayed(GetWorld(),
		                                            GetOwner(),
		                                            AttachmentRoot,
		                                            Effect,
		                                            static_cast<EReEchoCombatVfxSemantic>(SemanticValue),
		                                            TEXT("Attached"));
	}
	return Effect;
}

USceneComponent* UReEchoCombatVfxComponent::ResolveAttackVfxRoot() const
{
	AActor* Owner = GetOwner();
	return AttackVfxRoot ? AttackVfxRoot.Get() : (Owner ? Owner->GetRootComponent() : nullptr);
}

USceneComponent* UReEchoCombatVfxComponent::ResolveHurtVfxRoot() const
{
	AActor* Owner = GetOwner();
	return HurtVfxRoot ? HurtVfxRoot.Get() : (Owner ? Owner->GetRootComponent() : nullptr);
}

int32 UReEchoCombatVfxComponent::ResolveOwnerSortPriority() const
{
	const AActor* Owner = GetOwner();
	const UReEcho2DAnimationComponent* Animation =
	    Owner ? Owner->FindComponentByClass<UReEcho2DAnimationComponent>() : nullptr;
	const int32 OwnerPriority = Animation ? Animation->TranslucencySortPriority : 0;
	return ResolveCombatEffectSortPriority(OwnerPriority);
}

void UReEchoCombatVfxComponent::StopEffect(TObjectPtr<UNiagaraComponent>& Effect)
{
	if (Effect)
	{
		Effect->Deactivate();
		Effect->DestroyComponent();
		Effect = nullptr;
	}
}

void UReEchoCombatVfxComponent::StopAllEffects()
{
	StopEffect(ChargingEffect);
	StopEffect(DirectionEffect);
	StopEffect(DashEffect);
	for (TPair<int64, TObjectPtr<UNiagaraComponent>>& Pair : ProjectileEffects)
	{
		if (Pair.Value)
		{
			Pair.Value->Deactivate();
			Pair.Value->DestroyComponent();
		}
	}
	ProjectileEffects.Reset();
	ProjectileVisualOffsets.Reset();
	ProjectileTrajectoryEventCounts.Reset();
}

void UReEchoCombatVfxComponent::LogRabbitProjectileTrajectory(const FReEchoEnemyProjectileEvent& Event,
                                                              const UNiagaraComponent* Effect,
                                                              const TCHAR* Phase)
{
	int32& EventCount = ProjectileTrajectoryEventCounts.FindOrAdd(Event.Attack.Sequence);
	if (Event.Type == EReEchoEnemyProjectileEventType::Moved)
	{
		++EventCount;
		if (EventCount > ReEchoCombatVfx::TrajectoryLogMaximumEventCount ||
		    EventCount % ReEchoCombatVfx::TrajectoryLogEventStride != 0)
		{
			return;
		}
	}
	else if (Event.Type == EReEchoEnemyProjectileEventType::Spawned)
	{
		EventCount = 0;
	}

	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	const APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!PlayerController || !PlayerPawn)
	{
		UE_LOG(LogReEcho,
		       Warning,
		       TEXT("[RabbitAimTrace] Seq=%lld Phase=%s Sample=%d projection unavailable"),
		       Event.Attack.Sequence,
		       Phase,
		       EventCount);
		return;
	}

	const FVector PlayerWorld = PlayerPawn->GetActorLocation();
	const FVector RabbitWorld = GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;
	const FVector NiagaraWorld = Effect ? Effect->GetComponentLocation() : Event.Location;
	const FVector AuthoredCenterWorld = Effect ? Effect->GetComponentQuat()
	                                                 .RotateVector(FReEchoCombatVfxCatalog::ResolveAuthoredForwardAxis(
	                                                     EReEchoCombatVfxSemantic::RabbitProjectile))
	                                                 .GetSafeNormal()
	                                           : Event.Direction.GetSafeNormal();
	const FVector AxisProbeWorld = NiagaraWorld + AuthoredCenterWorld * 100.0f;

	FVector2D PlayerScreen = FVector2D::ZeroVector;
	FVector2D RabbitScreen = FVector2D::ZeroVector;
	FVector2D ProjectileScreen = FVector2D::ZeroVector;
	FVector2D NiagaraScreen = FVector2D::ZeroVector;
	FVector2D AxisProbeScreen = FVector2D::ZeroVector;
	const bool bPlayerProjected = PlayerController->ProjectWorldLocationToScreen(PlayerWorld, PlayerScreen, true);
	const bool bRabbitProjected = PlayerController->ProjectWorldLocationToScreen(RabbitWorld, RabbitScreen, true);
	const bool bProjectileProjected =
	    PlayerController->ProjectWorldLocationToScreen(Event.Location, ProjectileScreen, true);
	const bool bNiagaraProjected = PlayerController->ProjectWorldLocationToScreen(NiagaraWorld, NiagaraScreen, true);
	const bool bAxisProjected = PlayerController->ProjectWorldLocationToScreen(AxisProbeWorld, AxisProbeScreen, true);

	const FVector2D ToPlayerScreen = PlayerScreen - NiagaraScreen;
	const FVector2D AuthoredCenterScreen = AxisProbeScreen - NiagaraScreen;
	const float ScreenDirectionDot =
	    !ToPlayerScreen.IsNearlyZero() && !AuthoredCenterScreen.IsNearlyZero()
	        ? FVector2D::DotProduct(ToPlayerScreen.GetSafeNormal(), AuthoredCenterScreen.GetSafeNormal())
	        : 0.0f;
	int32 ViewportWidth = 0;
	int32 ViewportHeight = 0;
	PlayerController->GetViewportSize(ViewportWidth, ViewportHeight);

	UE_LOG(LogReEcho,
	       Warning,
	       TEXT("[RabbitAimTrace] Owner=%s Seq=%lld Phase=%s Sample=%d Viewport=(%d,%d) "
	            "PlayerWorld=%s PlayerScreen=(%.1f,%.1f,%d) RabbitWorld=%s RabbitScreen=(%.1f,%.1f,%d) "
	            "ProjectileWorld=%s ProjectileScreen=(%.1f,%.1f,%d) NiagaraWorld=%s NiagaraScreen=(%.1f,%.1f,%d) "
	            "EventDir=%s AuthoredCenterWorld=%s AuthoredCenterScreen=(%.1f,%.1f,%d) "
	            "ToPlayerScreen=(%.1f,%.1f) Dot=%.3f"),
	       *GetNameSafe(GetOwner()),
	       Event.Attack.Sequence,
	       Phase,
	       EventCount,
	       ViewportWidth,
	       ViewportHeight,
	       *PlayerWorld.ToCompactString(),
	       PlayerScreen.X,
	       PlayerScreen.Y,
	       bPlayerProjected,
	       *RabbitWorld.ToCompactString(),
	       RabbitScreen.X,
	       RabbitScreen.Y,
	       bRabbitProjected,
	       *Event.Location.ToCompactString(),
	       ProjectileScreen.X,
	       ProjectileScreen.Y,
	       bProjectileProjected,
	       *NiagaraWorld.ToCompactString(),
	       NiagaraScreen.X,
	       NiagaraScreen.Y,
	       bNiagaraProjected,
	       *Event.Direction.ToCompactString(),
	       *AuthoredCenterWorld.ToCompactString(),
	       AuthoredCenterScreen.X,
	       AuthoredCenterScreen.Y,
	       bAxisProjected,
	       ToPlayerScreen.X,
	       ToPlayerScreen.Y,
	       ScreenDirectionDot);

	LogRabbitParticleState(Effect, PlayerController, PlayerScreen, Event.Attack.Sequence, EventCount);
}

void UReEchoCombatVfxComponent::LogRabbitParticleState(const UNiagaraComponent* Effect,
                                                       APlayerController* PlayerController,
                                                       const FVector2D& PlayerScreen,
                                                       const int64 AttackSequence,
                                                       const int32 EventCount) const
{
	if (!Effect || !PlayerController)
	{
		return;
	}

	const auto Controller = Effect->GetSystemInstanceController();
	FNiagaraSystemInstance* SystemInstance =
	    Controller.IsValid() && Controller->IsSolo() ? Controller->GetSoloSystemInstance() : nullptr;
	if (!SystemInstance)
	{
		UE_LOG(LogReEcho,
		       Warning,
		       TEXT("[RabbitParticleTrace] Owner=%s Seq=%lld Sample=%d readback unavailable solo=%d"),
		       *GetNameSafe(GetOwner()),
		       AttackSequence,
		       EventCount,
		       Controller.IsValid() && Controller->IsSolo());
		return;
	}

	FVector2D ComponentScreen = FVector2D::ZeroVector;
	PlayerController->ProjectWorldLocationToScreen(Effect->GetComponentLocation(), ComponentScreen, true);
	const FTransform& SystemTransform = SystemInstance->GetWorldTransform();
	const FNiagaraLWCConverter LwcConverter = SystemInstance->GetLWCConverter(false);
	static const FName PositionName(TEXT("Position"));
	static const FName VelocityName(TEXT("Velocity"));
	constexpr uint32 MaxParticlesPerEmitter = 3;

	for (const FNiagaraEmitterInstanceRef& EmitterRef : SystemInstance->GetEmitters())
	{
		const FNiagaraEmitterInstance& Emitter = EmitterRef.Get();
		const FName EmitterName = Emitter.GetEmitterHandle().GetName();
		if (EmitterName != TEXT("Fountain004") && EmitterName != TEXT("Fountain005"))
		{
			continue;
		}

		if (Emitter.GetSimTarget() != ENiagaraSimTarget::CPUSim)
		{
			UE_LOG(LogReEcho,
			       Warning,
			       TEXT("[RabbitParticleTrace] Owner=%s Seq=%lld Sample=%d Emitter=%s SimTarget=GPU readback skipped"),
			       *GetNameSafe(GetOwner()),
			       AttackSequence,
			       EventCount,
			       *EmitterName.ToString());
			continue;
		}

		const FNiagaraDataSet& ParticleData = Emitter.GetParticleData();
		const FNiagaraDataBuffer* DataBuffer = ParticleData.GetCurrentData();
		const uint32 ParticleCount = DataBuffer ? DataBuffer->GetNumInstances() : 0;
		FNiagaraDataSetAccessor<FNiagaraPosition> PositionAccessor(ParticleData, PositionName);
		const auto PositionReader = PositionAccessor.GetReader(ParticleData);
		FNiagaraDataSetAccessor<FVector3f> VelocityAccessor(ParticleData, VelocityName);
		const auto VelocityReader = VelocityAccessor.GetReader(ParticleData);
		if (!PositionReader.IsValid() || ParticleCount == 0)
		{
			UE_LOG(
			    LogReEcho,
			    Warning,
			    TEXT("[RabbitParticleTrace] Owner=%s Seq=%lld Sample=%d Emitter=%s Local=%d Count=%u PositionValid=%d"),
			    *GetNameSafe(GetOwner()),
			    AttackSequence,
			    EventCount,
			    *EmitterName.ToString(),
			    Emitter.IsLocalSpace(),
			    ParticleCount,
			    PositionReader.IsValid());
			continue;
		}

		const uint32 LoggedParticleCount = FMath::Min(ParticleCount, MaxParticlesPerEmitter);
		for (uint32 ParticleIndex = 0; ParticleIndex < LoggedParticleCount; ++ParticleIndex)
		{
			const FVector SimulationPosition =
			    LwcConverter.ConvertSimulationPositionToWorld(PositionReader.Get(ParticleIndex));
			const FVector ParticleWorld =
			    Emitter.IsLocalSpace() ? SystemTransform.TransformPosition(SimulationPosition) : SimulationPosition;
			FVector WorldVelocity = FVector::ZeroVector;
			if (VelocityReader.IsValid())
			{
				WorldVelocity = LwcConverter.ConvertSimulationVectorToWorld(VelocityReader.Get(ParticleIndex));
				if (Emitter.IsLocalSpace())
				{
					WorldVelocity = SystemTransform.TransformVectorNoScale(WorldVelocity);
				}
			}

			FVector2D ParticleScreen = FVector2D::ZeroVector;
			FVector2D VelocityProbeScreen = FVector2D::ZeroVector;
			const bool bParticleProjected =
			    PlayerController->ProjectWorldLocationToScreen(ParticleWorld, ParticleScreen, true);
			const bool bVelocityProjected = PlayerController->ProjectWorldLocationToScreen(
			    ParticleWorld + WorldVelocity * 0.1f, VelocityProbeScreen, true);
			const FVector2D VelocityScreen = VelocityProbeScreen - ParticleScreen;
			const FVector2D ToPlayerScreen = PlayerScreen - ParticleScreen;
			const float VelocityDot =
			    !VelocityScreen.IsNearlyZero() && !ToPlayerScreen.IsNearlyZero()
			        ? FVector2D::DotProduct(VelocityScreen.GetSafeNormal(), ToPlayerScreen.GetSafeNormal())
			        : 0.0f;

			UE_LOG(LogReEcho,
			       Warning,
			       TEXT("[RabbitParticleTrace] Owner=%s Seq=%lld Sample=%d Emitter=%s Local=%d Count=%u "
			            "Particle=%u World=%s Screen=(%.1f,%.1f,%d) FromComponent=(%.1f,%.1f) "
			            "Velocity=%s VelocityScreen=(%.1f,%.1f,%d) ToPlayer=(%.1f,%.1f) VelocityDot=%.3f"),
			       *GetNameSafe(GetOwner()),
			       AttackSequence,
			       EventCount,
			       *EmitterName.ToString(),
			       Emitter.IsLocalSpace(),
			       ParticleCount,
			       ParticleIndex,
			       *ParticleWorld.ToCompactString(),
			       ParticleScreen.X,
			       ParticleScreen.Y,
			       bParticleProjected,
			       ParticleScreen.X - ComponentScreen.X,
			       ParticleScreen.Y - ComponentScreen.Y,
			       *WorldVelocity.ToCompactString(),
			       VelocityScreen.X,
			       VelocityScreen.Y,
			       bVelocityProjected,
			       ToPlayerScreen.X,
			       ToPlayerScreen.Y,
			       VelocityDot);
		}
	}
}

void UReEchoCombatVfxComponent::HandleAttackCommitted(const FReEchoAttackCommittedEvent& Event)
{
	if (!FReEchoCombatVfxCatalog::IsMeleeAttackPattern(Event.AttackPatternId))
	{
		return;
	}
	SpawnAttached(
	    static_cast<uint8>(EReEchoCombatVfxSemantic::PlayerMeleeSlash), Event.Direction, ResolveAttackVfxRoot());
}

void UReEchoCombatVfxComponent::HandleHurt(const FReEchoDamageEvent& Event)
{
	if (Event.Target != GetOwner() || Event.AppliedDamage <= 0.0f)
	{
		return;
	}
	const EReEchoCombatVfxSemantic Semantic = Cast<AReEchoEnemyActor>(GetOwner())
	                                              ? EReEchoCombatVfxSemantic::EnemyHurt
	                                              : EReEchoCombatVfxSemantic::PlayerHurt;
	SpawnAttached(static_cast<uint8>(Semantic), FVector::ForwardVector, ResolveHurtVfxRoot());
}

void UReEchoCombatVfxComponent::HandleDeath(const FReEchoDamageEvent& Event)
{
	if (Event.Target == GetOwner())
	{
		StopAllEffects();
	}
}

void UReEchoCombatVfxComponent::HandleSpecialAction(const FReEchoEnemySpecialActionEvent& Event)
{
	const bool bRabbit = Event.AbilityId == TEXT("M_RABBIT_RangedBurst");
	const bool bFox = Event.AbilityId == TEXT("M_FOX_Dash");
	if (!bRabbit && !bFox)
	{
		return;
	}
	if (Event.Type == EReEchoEnemySpecialActionEventType::WindupStarted)
	{
		StopEffect(ChargingEffect);
		StopEffect(DirectionEffect);
		const EReEchoCombatVfxSemantic ChargingSemantic =
		    bRabbit ? EReEchoCombatVfxSemantic::RabbitCharging : EReEchoCombatVfxSemantic::FoxCharging;
		ChargingEffect =
		    SpawnAttached(static_cast<uint8>(ChargingSemantic), Event.LockedDirection, ResolveAttackVfxRoot(), false);
		if (bFox)
		{
			DirectionEffect = SpawnAttached(static_cast<uint8>(EReEchoCombatVfxSemantic::FoxDirection),
			                                Event.LockedDirection,
			                                ResolveAttackVfxRoot(),
			                                false);
		}
		return;
	}
	StopEffect(ChargingEffect);
	StopEffect(DirectionEffect);
	if (Event.Type == EReEchoEnemySpecialActionEventType::ActionCommitted && bFox)
	{
		StopEffect(DashEffect);
		DashEffect = SpawnAttached(static_cast<uint8>(EReEchoCombatVfxSemantic::FoxDash),
		                           Event.LockedDirection,
		                           ResolveAttackVfxRoot(),
		                           false);
	}
	else if (Event.Type == EReEchoEnemySpecialActionEventType::ActionEnded)
	{
		StopEffect(DashEffect);
	}
}

void UReEchoCombatVfxComponent::HandleProjectile(const FReEchoEnemyProjectileEvent& Event)
{
	if (Event.AbilityId != TEXT("M_RABBIT_RangedBurst") || Event.Attack.Sequence <= 0)
	{
		return;
	}
	if (Event.Type == EReEchoEnemyProjectileEventType::Spawned)
	{
		if (TObjectPtr<UNiagaraComponent>* Existing = ProjectileEffects.Find(Event.Attack.Sequence))
		{
			if (*Existing)
			{
				(*Existing)->DestroyComponent();
			}
			ProjectileEffects.Remove(Event.Attack.Sequence);
			ProjectileVisualOffsets.Remove(Event.Attack.Sequence);
		}
		const USceneComponent* AttackRoot = ResolveAttackVfxRoot();
		const FVector VisualOffset =
		    AttackRoot ? AttackRoot->GetComponentLocation() - Event.Location : FVector::ZeroVector;
		UNiagaraComponent* Effect = SpawnWorld(static_cast<uint8>(EReEchoCombatVfxSemantic::RabbitProjectile),
		                                       Event.Location + VisualOffset,
		                                       Event.Direction,
		                                       false);
		if (Effect)
		{
			Effect->SetForceSolo(true);
			ProjectileEffects.Add(Event.Attack.Sequence, Effect);
			ProjectileVisualOffsets.Add(Event.Attack.Sequence, VisualOffset);
		}
		LogRabbitProjectileTrajectory(Event, Effect, TEXT("Spawned"));
		return;
	}
	if (TObjectPtr<UNiagaraComponent>* Effect = ProjectileEffects.Find(Event.Attack.Sequence))
	{
		if (*Effect && Event.Type == EReEchoEnemyProjectileEventType::Moved)
		{
			const FVector VisualOffset = ProjectileVisualOffsets.FindRef(Event.Attack.Sequence);
			(*Effect)->SetWorldLocationAndRotation(
			    Event.Location + VisualOffset,
			    FReEchoCombatVfxCatalog::ResolveRotation(EReEchoCombatVfxSemantic::RabbitProjectile, Event.Direction));
			LogRabbitProjectileTrajectory(Event, *Effect, TEXT("Moved"));
		}
		else if (Event.Type == EReEchoEnemyProjectileEventType::Ended)
		{
			LogRabbitProjectileTrajectory(Event, *Effect, TEXT("Ended"));
			if (*Effect)
			{
				(*Effect)->Deactivate();
				(*Effect)->DestroyComponent();
			}
			ProjectileEffects.Remove(Event.Attack.Sequence);
			ProjectileVisualOffsets.Remove(Event.Attack.Sequence);
			ProjectileTrajectoryEventCounts.Remove(Event.Attack.Sequence);
		}
	}
}
