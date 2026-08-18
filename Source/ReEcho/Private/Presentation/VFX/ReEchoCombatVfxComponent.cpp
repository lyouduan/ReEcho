#include "Presentation/VFX/ReEchoCombatVfxComponent.h"

#include "Graybox/ReEchoEnemyActor.h"
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
		return;
	}
	if (TObjectPtr<UNiagaraComponent>* Effect = ProjectileEffects.Find(Event.Attack.Sequence))
	{
		if (*Effect && Event.Type == EReEchoEnemyProjectileEventType::Moved)
		{
			(*Effect)->SetWorldLocationAndRotation(Event.Location,
			                                       FReEchoCombatVfxCatalog::ResolveRotation(Event.Direction, true));
		}
		else if (Event.Type == EReEchoEnemyProjectileEventType::Ended)
		{
			if (*Effect)
			{
				(*Effect)->Deactivate();
				(*Effect)->DestroyComponent();
			}
			ProjectileEffects.Remove(Event.Attack.Sequence);
		}
	}
}
