#include "Presentation/VFX/ReEchoCombatVfxComponent.h"

#include "Graybox/ReEchoEnemyActor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Presentation/Animation2D/ReEcho2DAnimationComponent.h"
#include "Presentation/VFX/ReEchoCombatVfxCatalog.h"
#include "ReEcho.h"

namespace ReEchoCombatVfx
{
constexpr int32 ForegroundSortOffset = 1;
constexpr int32 BackgroundSortOffset = -1;
constexpr int32 TrajectoryLogEventStride = 6;
constexpr int32 TrajectoryLogMaximumEventCount = 60;
} // namespace ReEchoCombatVfx

UReEchoCombatVfxComponent::UReEchoCombatVfxComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
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
                                                         const bool bLocalYAxisForward,
                                                         const bool bAutoDestroy) const
{
	UNiagaraSystem* System = ResolveSystem(SemanticValue);
	UWorld* World = GetWorld();
	if (!System || !World)
	{
		return nullptr;
	}
	UNiagaraComponent* Effect = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
	    World,
	    System,
	    Location,
	    FReEchoCombatVfxCatalog::ResolveRotation(Direction, bLocalYAxisForward),
	    FVector::OneVector,
	    bAutoDestroy,
	    true,
	    ENCPoolMethod::None,
	    true);
	if (Effect)
	{
		Effect->SetTranslucentSortPriority(ResolveOwnerSortPriority(true));
	}
	return Effect;
}

UNiagaraComponent* UReEchoCombatVfxComponent::SpawnAttached(const uint8 SemanticValue, const FVector& Direction) const
{
	UNiagaraSystem* System = ResolveSystem(SemanticValue);
	AActor* Owner = GetOwner();
	USceneComponent* Root = Owner ? Owner->GetRootComponent() : nullptr;
	if (!System || !Root)
	{
		return nullptr;
	}
	UNiagaraComponent* Effect =
	    UNiagaraFunctionLibrary::SpawnSystemAttached(System,
	                                                 Root,
	                                                 NAME_None,
	                                                 FVector::ZeroVector,
	                                                 FReEchoCombatVfxCatalog::ResolveRotation(Direction, false),
	                                                 FVector::OneVector,
	                                                 EAttachLocation::KeepRelativeOffset,
	                                                 true,
	                                                 ENCPoolMethod::None,
	                                                 true);
	if (Effect)
	{
		Effect->SetTranslucentSortPriority(ResolveOwnerSortPriority(false));
	}
	return Effect;
}

int32 UReEchoCombatVfxComponent::ResolveOwnerSortPriority(const bool bForeground) const
{
	const AActor* Owner = GetOwner();
	const UReEcho2DAnimationComponent* Animation =
	    Owner ? Owner->FindComponentByClass<UReEcho2DAnimationComponent>() : nullptr;
	const int32 OwnerPriority = Animation ? Animation->TranslucencySortPriority : 0;
	return OwnerPriority +
	       (bForeground ? ReEchoCombatVfx::ForegroundSortOffset : ReEchoCombatVfx::BackgroundSortOffset);
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
	const FVector LocalYWorld = Effect ? Effect->GetComponentQuat().RotateVector(FVector::YAxisVector).GetSafeNormal()
	                                   : Event.Direction.GetSafeNormal();
	const FVector AxisProbeWorld = NiagaraWorld + LocalYWorld * 100.0f;

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
	const FVector2D LocalYScreen = AxisProbeScreen - NiagaraScreen;
	const float ScreenDirectionDot =
	    !ToPlayerScreen.IsNearlyZero() && !LocalYScreen.IsNearlyZero()
	        ? FVector2D::DotProduct(ToPlayerScreen.GetSafeNormal(), LocalYScreen.GetSafeNormal())
	        : 0.0f;
	int32 ViewportWidth = 0;
	int32 ViewportHeight = 0;
	PlayerController->GetViewportSize(ViewportWidth, ViewportHeight);

	UE_LOG(LogReEcho,
	       Warning,
	       TEXT("[RabbitAimTrace] Seq=%lld Phase=%s Sample=%d Viewport=(%d,%d) "
	            "PlayerWorld=%s PlayerScreen=(%.1f,%.1f,%d) RabbitWorld=%s RabbitScreen=(%.1f,%.1f,%d) "
	            "ProjectileWorld=%s ProjectileScreen=(%.1f,%.1f,%d) NiagaraWorld=%s NiagaraScreen=(%.1f,%.1f,%d) "
	            "EventDir=%s LocalYWorld=%s LocalYScreen=(%.1f,%.1f,%d) ToPlayerScreen=(%.1f,%.1f) Dot=%.3f"),
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
	       *LocalYWorld.ToCompactString(),
	       LocalYScreen.X,
	       LocalYScreen.Y,
	       bAxisProjected,
	       ToPlayerScreen.X,
	       ToPlayerScreen.Y,
	       ScreenDirectionDot);
}

void UReEchoCombatVfxComponent::HandleAttackCommitted(const FReEchoAttackCommittedEvent& Event)
{
	if (!FReEchoCombatVfxCatalog::IsMeleeAttackPattern(Event.AttackPatternId))
	{
		return;
	}
	SpawnWorld(static_cast<uint8>(EReEchoCombatVfxSemantic::PlayerMeleeSlash), Event.Origin, Event.Direction);
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
	SpawnWorld(static_cast<uint8>(Semantic), Event.WorldLocation, FVector::ForwardVector);
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
		    SpawnWorld(static_cast<uint8>(ChargingSemantic), Event.Origin, Event.LockedDirection, false, false);
		if (bFox)
		{
			DirectionEffect = SpawnWorld(static_cast<uint8>(EReEchoCombatVfxSemantic::FoxDirection),
			                             Event.Origin,
			                             Event.LockedDirection,
			                             false,
			                             false);
			if (DirectionEffect)
			{
				DirectionEffect->SetTranslucentSortPriority(ResolveOwnerSortPriority(false));
			}
		}
		return;
	}
	StopEffect(ChargingEffect);
	StopEffect(DirectionEffect);
	if (Event.Type == EReEchoEnemySpecialActionEventType::ActionCommitted && bFox)
	{
		StopEffect(DashEffect);
		DashEffect = SpawnAttached(static_cast<uint8>(EReEchoCombatVfxSemantic::FoxDash), Event.LockedDirection);
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
		}
		UNiagaraComponent* Effect = SpawnWorld(static_cast<uint8>(EReEchoCombatVfxSemantic::RabbitProjectile),
		                                       Event.Location,
		                                       Event.Direction,
		                                       true,
		                                       false);
		if (Effect)
		{
			ProjectileEffects.Add(Event.Attack.Sequence, Effect);
		}
		LogRabbitProjectileTrajectory(Event, Effect, TEXT("Spawned"));
		return;
	}
	if (TObjectPtr<UNiagaraComponent>* Effect = ProjectileEffects.Find(Event.Attack.Sequence))
	{
		if (*Effect && Event.Type == EReEchoEnemyProjectileEventType::Moved)
		{
			(*Effect)->SetWorldLocationAndRotation(Event.Location,
			                                       FReEchoCombatVfxCatalog::ResolveRotation(Event.Direction, true));
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
			ProjectileTrajectoryEventCounts.Remove(Event.Attack.Sequence);
		}
	}
}
