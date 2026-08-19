#include "Presentation/VFX/ReEchoCombatVfxComponent.h"

#include "Graybox/ReEchoEnemyActor.h"
#include "Components/MaterialBillboardComponent.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Presentation/Animation2D/ReEcho2DAnimationComponent.h"
#include "Presentation/VFX/ReEchoCombatVfxCatalog.h"
#include "ReEcho.h"
#include "TimerManager.h"

namespace ReEchoCombatVfx
{
constexpr int32 CombatEffectSortOffset = 1;
constexpr int32 CombatEffectSortPriorityFloor = 100;
constexpr float RabbitProjectileOpaqueDiameterFraction = 154.0f / 512.0f;

void LogLayerState(const AActor* Owner,
                   const USceneComponent* AttachmentRoot,
                   const UNiagaraComponent* Effect,
                   const EReEchoCombatVfxSemantic Semantic,
                   const TCHAR* SpawnMode,
                   const TCHAR* Phase)
{
	const UReEcho2DAnimationComponent* Animation =
	    Owner ? Owner->FindComponentByClass<UReEcho2DAnimationComponent>() : nullptr;
	const bool bHasOwnerPriority = Animation != nullptr;
	const bool bHasEffectPriority = Effect != nullptr;
	const int32 OwnerPriority = bHasOwnerPriority ? Animation->TranslucencySortPriority : INDEX_NONE;
	const int32 EffectPriority = bHasEffectPriority ? Effect->TranslucencySortPriority : INDEX_NONE;
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
	       bHasOwnerPriority && bHasEffectPriority ? EffectPriority - OwnerPriority : INDEX_NONE);
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
	return FMath::Max(ReEchoCombatVfx::CombatEffectSortPriorityFloor,
	                  OwnerSortPriority + ReEchoCombatVfx::CombatEffectSortOffset);
}

FReEchoProjectileVisualKey
UReEchoCombatVfxComponent::ResolveProjectileVisualKey(const FReEchoEnemyProjectileEvent& Event)
{
	FReEchoProjectileVisualKey Key;
	Key.Source = Event.Attack.Source;
	Key.Sequence = Event.Attack.Sequence;
	Key.VolleyBallIndex = Event.VolleyBallIndex;
	return Key;
}

float UReEchoCombatVfxComponent::ResolveProjectileVisualScale(const float CollisionRadiusCm,
                                                              const int32 TextureSizePixels)
{
	if (CollisionRadiusCm <= 0.0f || TextureSizePixels <= 0)
	{
		return 1.0f;
	}
	const float OpaqueDiameterPixels = TextureSizePixels * ReEchoCombatVfx::RabbitProjectileOpaqueDiameterFraction;
	return CollisionRadiusCm * 2.0f / OpaqueDiameterPixels;
}

#if WITH_DEV_AUTOMATION_TESTS
int32 UReEchoCombatVfxComponent::GetProjectileVisualCountForTests() const
{
	return ProjectileVisuals.Num();
}

bool UReEchoCombatVfxComponent::TryGetProjectileVisualLocationForTests(const int64 AttackSequence,
                                                                       const int32 VolleyBallIndex,
                                                                       FVector& OutLocation) const
{
	FReEchoProjectileVisualKey Key;
	Key.Source = GetOwner();
	Key.Sequence = AttackSequence;
	Key.VolleyBallIndex = VolleyBallIndex;
	const TObjectPtr<UMaterialBillboardComponent>* Visual = ProjectileVisuals.Find(Key);
	if (!Visual || !Visual->Get())
	{
		return false;
	}
	OutLocation = (*Visual)->GetComponentLocation();
	return true;
}
#endif

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

UTexture2D* UReEchoCombatVfxComponent::ResolveRabbitProjectileTexture() const
{
	UTexture2D* Texture =
	    LoadObject<UTexture2D>(nullptr, FReEchoCombatVfxCatalog::ResolveRabbitProjectileTexturePath());
	if (!Texture && !bMissingRabbitProjectileTextureWarned)
	{
		bMissingRabbitProjectileTextureWarned = true;
		UE_LOG(LogReEcho,
		       Warning,
		       TEXT("[VFX] Missing rabbit projectile texture '%s'; gameplay continues without it"),
		       FReEchoCombatVfxCatalog::ResolveRabbitProjectileTexturePath());
	}
	return Texture;
}

UMaterialInterface* UReEchoCombatVfxComponent::ResolveRabbitProjectileMaterial() const
{
	UMaterialInterface* Material =
	    LoadObject<UMaterialInterface>(nullptr, FReEchoCombatVfxCatalog::ResolveRabbitProjectileMaterialPath());
	if (!Material && !bMissingRabbitProjectileMaterialWarned)
	{
		bMissingRabbitProjectileMaterialWarned = true;
		UE_LOG(LogReEcho,
		       Warning,
		       TEXT("[VFX] Missing rabbit projectile material '%s'; gameplay continues without it"),
		       FReEchoCombatVfxCatalog::ResolveRabbitProjectileMaterialPath());
	}
	return Material;
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

void UReEchoCombatVfxComponent::StopProjectileVisual(UMaterialBillboardComponent* Visual) const
{
	if (Visual)
	{
		Visual->DestroyComponent();
	}
}

void UReEchoCombatVfxComponent::StopAllEffects()
{
	StopEffect(ChargingEffect);
	StopEffect(DirectionEffect);
	StopEffect(DashEffect);
	for (TPair<FReEchoProjectileVisualKey, TObjectPtr<UMaterialBillboardComponent>>& Pair : ProjectileVisuals)
	{
		StopProjectileVisual(Pair.Value);
	}
	ProjectileVisuals.Reset();
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
	if (Event.AbilityId != TEXT("M_RABBIT_RangedBurst") || Event.Attack.Sequence <= 0 || Event.VolleyBallIndex < 0)
	{
		return;
	}
	const FReEchoProjectileVisualKey Key = ResolveProjectileVisualKey(Event);
	if (Event.Type == EReEchoEnemyProjectileEventType::Spawned)
	{
		if (TObjectPtr<UMaterialBillboardComponent>* Existing = ProjectileVisuals.Find(Key))
		{
			StopProjectileVisual(*Existing);
			ProjectileVisuals.Remove(Key);
		}
		AActor* Owner = GetOwner();
		UTexture2D* Texture = ResolveRabbitProjectileTexture();
		UMaterialInterface* Material = ResolveRabbitProjectileMaterial();
		if (Owner && Texture && Material)
		{
			UMaterialBillboardComponent* Visual = NewObject<UMaterialBillboardComponent>(Owner);
			Owner->AddInstanceComponent(Visual);
			Visual->SetMobility(EComponentMobility::Movable);
			Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Visual->SetCastShadow(false);
			const int32 TextureExtentPixels = FMath::Max(Texture->GetSizeX(), Texture->GetSizeY());
			const float WorldDiameter =
			    TextureExtentPixels * ResolveProjectileVisualScale(Event.CollisionRadiusCm, TextureExtentPixels);
			Visual->AddElement(Material, nullptr, false, WorldDiameter, WorldDiameter, nullptr);
			Visual->SetTranslucentSortPriority(ResolveOwnerSortPriority());
			Visual->SetWorldLocation(Event.Location);
			Visual->SetHiddenInGame(false);
			Visual->SetVisibility(true);
			Visual->RegisterComponent();
			ProjectileVisuals.Add(Key, Visual);
		}
		return;
	}
	if (TObjectPtr<UMaterialBillboardComponent>* Visual = ProjectileVisuals.Find(Key))
	{
		if (*Visual && Event.Type == EReEchoEnemyProjectileEventType::Moved)
		{
			(*Visual)->SetWorldLocation(Event.Location);
		}
		else if (Event.Type == EReEchoEnemyProjectileEventType::Ended)
		{
			StopProjectileVisual(*Visual);
			ProjectileVisuals.Remove(Key);
		}
	}
}
