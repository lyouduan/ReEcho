#include "Presentation/VFX/ReEchoCombatVfxComponent.h"

#include "Combat/ReEchoCombatantComponent.h"
#include "Combat/ReEchoCombatTarget.h"
#include "Data/ReEchoCsvDataRegistry.h"
#include "Graybox/ReEchoEnemyActor.h"
#include "Components/MaterialBillboardComponent.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Presentation/Animation2D/ReEcho2DAnimationComponent.h"
#include "Presentation/Combat/ReEchoCombatPresentationCoordinator.h"
#include "Presentation/VFX/ReEchoCombatVfxCatalog.h"
#include "Presentation/VFX/ReEchoElementReactionVfxCatalog.h"
#include "Presentation/Weapon/ReEchoWeaponPresentationProfile.h"
#include "ReEcho.h"
#include "TimerManager.h"
#include "Weapons/ReEchoWeaponVisualCatalog.h"

namespace ReEchoCombatVfx
{
constexpr int32 CombatEffectSortOffset = 1;
constexpr int32 CombatEffectSortPriorityFloor = 1000;
constexpr float RabbitProjectileGlowDiameterScale = 1.5f;

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
	       VeryVerbose,
	       TEXT("[CombatVfxLayerTrace] Phase=%s Mode=%s Semantic=%s Owner=%s Animation=%s "
	            "OwnerPriority=%d OwnerDistanceOffset=%.2f OwnerWorld=%s AnimationWorld=%s "
	            "Anchor=%s AnchorParent=%s AnchorWorld=%s Effect=%s EffectParent=%s EffectPriority=%d "
	            "EffectDistanceOffset=%.2f EffectWorld=%s Registered=%d Active=%d Visible=%d PriorityDelta=%d"),
	       Phase,
	       SpawnMode,
	       *FReEchoCombatVfxCatalog::ResolvePath(Semantic),
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

float UReEchoCombatVfxComponent::ResolveProjectileCoreDiameter(const float CollisionRadiusCm)
{
	return FMath::Max(0.0f, CollisionRadiusCm) * 2.0f;
}

float UReEchoCombatVfxComponent::ResolveProjectileGlowDiameter(const float CollisionRadiusCm)
{
	return ResolveProjectileCoreDiameter(CollisionRadiusCm) * ReEchoCombatVfx::RabbitProjectileGlowDiameterScale;
}

bool UReEchoCombatVfxComponent::IsElementReactionStateDriven(const FName ReactionId)
{
	return ReactionId == TEXT("Y_ER_F_G") || ReactionId == TEXT("Y_ER_L_G");
}

float UReEchoCombatVfxComponent::ResolveConductLinkScheduledTime(const int32 LinkIndex, const float DelaySeconds)
{
	return FMath::Max(0, LinkIndex) * FMath::Max(0.0f, DelaySeconds);
}

void UReEchoCombatVfxComponent::ResolveConductLinkWorldEndpoints(const FVector& StartWorld,
                                                                 const FVector& EndWorld,
                                                                 FVector& OutStartParameter,
                                                                 FVector& OutEndParameter)
{
	OutStartParameter = StartWorld;
	OutEndParameter = EndWorld;
}

float UReEchoCombatVfxComponent::ResolveConductPropagationDelaySeconds(const FName WeaponId)
{
	if (WeaponId.IsNone())
	{
		return 0.0f;
	}
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
	const FReEchoCsvWeaponRow* Weapon = Snapshot.IsValid() ? Snapshot->FindWeapon(WeaponId) : nullptr;
	const UReEchoWeaponPresentationProfile* Profile =
	    Weapon ? FReEchoWeaponVisualCatalog::ResolveProfile(Weapon->VisualKey) : nullptr;
	if (!Profile)
	{
		UE_LOG(LogReEcho,
		       Warning,
		       TEXT("Conduct presentation delay defaults to 0: WeaponId '%s' has no resolvable presentation profile"),
		       *WeaponId.ToString());
	}
	return Profile ? FMath::Max(0.0f, Profile->ConductLinkPropagationDelaySeconds) : 0.0f;
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
	CancelConductPropagation();
	BindEventSources(nullptr, nullptr);
	StopAllEffects();
	Super::EndPlay(EndPlayReason);
}

void UReEchoCombatVfxComponent::BindEventSources(UReEchoCombatEventsComponent* InCombatEvents,
                                                 UReEchoEnemyEventsComponent* InEnemyEvents)
{
	CancelConductPropagation();
	if (CombatEvents)
	{
		CombatEvents->OnAttackCommitted.RemoveAll(this);
		CombatEvents->OnHurt.RemoveAll(this);
		CombatEvents->OnDeath.RemoveAll(this);
		CombatEvents->OnElementStateChanged.RemoveAll(this);
		CombatEvents->OnElementReactionResolved.RemoveAll(this);
	}
	if (EnemyEvents)
	{
		EnemyEvents->OnProjectile.RemoveAll(this);
	}
	if (CombatPresentationCoordinator)
	{
		CombatPresentationCoordinator->OnActionPhase.RemoveAll(this);
	}
	CombatEvents = InCombatEvents;
	EnemyEvents = InEnemyEvents;
	CombatPresentationCoordinator =
	    GetOwner() ? GetOwner()->FindComponentByClass<UReEchoCombatPresentationCoordinator>() : nullptr;
	if (CombatEvents)
	{
		CombatEvents->OnAttackCommitted.AddDynamic(this, &UReEchoCombatVfxComponent::HandleAttackCommitted);
		CombatEvents->OnHurt.AddDynamic(this, &UReEchoCombatVfxComponent::HandleHurt);
		CombatEvents->OnDeath.AddDynamic(this, &UReEchoCombatVfxComponent::HandleDeath);
		CombatEvents->OnElementStateChanged.AddDynamic(this, &UReEchoCombatVfxComponent::HandleElementStateChanged);
		CombatEvents->OnElementReactionResolved.AddDynamic(this,
		                                                   &UReEchoCombatVfxComponent::HandleElementReactionResolved);
	}
	if (EnemyEvents)
	{
		EnemyEvents->OnProjectile.AddDynamic(this, &UReEchoCombatVfxComponent::HandleProjectile);
	}
	if (CombatPresentationCoordinator)
	{
		CombatPresentationCoordinator->OnActionPhase.AddDynamic(this,
		                                                        &UReEchoCombatVfxComponent::HandlePresentationAction);
	}
}

UNiagaraSystem* UReEchoCombatVfxComponent::ResolveSystem(const uint8 SemanticValue) const
{
	const EReEchoCombatVfxSemantic Semantic = static_cast<EReEchoCombatVfxSemantic>(SemanticValue);
	const FString SystemPath = FReEchoCombatVfxCatalog::ResolvePath(Semantic);
	UNiagaraSystem* System = LoadObject<UNiagaraSystem>(nullptr, *SystemPath);
	if (!System && !MissingSystemWarnings.Contains(SemanticValue))
	{
		MissingSystemWarnings.Add(SemanticValue);
		UE_LOG(
		    LogReEcho, Warning, TEXT("[VFX] Missing semantic asset '%s'; gameplay continues without it"), *SystemPath);
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

TArray<UMaterialInterface*> UReEchoCombatVfxComponent::ResolveRabbitProjectileGlowMaterials() const
{
	TArray<UMaterialInterface*> Materials;
	Materials.Reserve(FReEchoCombatVfxCatalog::GetRabbitProjectileGlowMaterialCount());
	for (int32 LayerIndex = 0; LayerIndex < FReEchoCombatVfxCatalog::GetRabbitProjectileGlowMaterialCount();
	     ++LayerIndex)
	{
		const TCHAR* Path = FReEchoCombatVfxCatalog::ResolveRabbitProjectileGlowMaterialPath(LayerIndex);
		if (UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, Path))
		{
			Materials.Add(Material);
		}
		else if (!bMissingRabbitProjectileGlowMaterialWarned)
		{
			bMissingRabbitProjectileGlowMaterialWarned = true;
			UE_LOG(LogReEcho,
			       Warning,
			       TEXT("[VFX] Missing rabbit projectile glow material '%s'; gameplay continues without it"),
			       Path);
		}
	}
	return Materials;
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
	StopEffect(ElementAttachmentEffect);
	StopEffect(BurnStatusEffect);
	ActiveAttachmentElement = EReEchoElement::None;
	for (TPair<FReEchoProjectileVisualKey, TObjectPtr<UMaterialBillboardComponent>>& Pair : ProjectileVisuals)
	{
		StopProjectileVisual(Pair.Value);
	}
	ProjectileVisuals.Reset();
}

FName UReEchoCombatVfxComponent::ResolveElementVfxTargetId(AActor* Target) const
{
	if (const AReEchoEnemyActor* Enemy = Cast<AReEchoEnemyActor>(Target))
	{
		return Enemy->GetPresentationId();
	}
	return NAME_None;
}

UNiagaraSystem* UReEchoCombatVfxComponent::ResolveElementSystem(const uint8 SemanticValue, AActor* Target) const
{
	const TCHAR* Path = FReEchoElementReactionVfxCatalog::ResolvePath(
	    static_cast<EReEchoElementReactionVfxSemantic>(SemanticValue), ResolveElementVfxTargetId(Target));
	UNiagaraSystem* System = LoadObject<UNiagaraSystem>(nullptr, Path);
	if (!System && !MissingElementSystemWarnings.Contains(Path))
	{
		MissingElementSystemWarnings.Add(Path);
		UE_LOG(LogReEcho, Warning, TEXT("[ElementVFX] Missing Niagara system '%s'; gameplay continues"), Path);
	}
	return System;
}

void UReEchoCombatVfxComponent::RefreshElementEffects(const FReEchoElementState& State)
{
	RefreshElementAttachment(State.Attached);
	RefreshBurnStatus(State.bBurnActive);
}

void UReEchoCombatVfxComponent::RefreshElementAttachment(const EReEchoElement Element)
{
	if (ActiveAttachmentElement == Element && ElementAttachmentEffect)
	{
		return;
	}
	StopEffect(ElementAttachmentEffect);
	ActiveAttachmentElement = EReEchoElement::None;
	EReEchoElementReactionVfxSemantic Semantic;
	if (Element == EReEchoElement::Grass)
	{
		Semantic = EReEchoElementReactionVfxSemantic::AttachmentGrass;
	}
	else if (Element == EReEchoElement::Water)
	{
		Semantic = EReEchoElementReactionVfxSemantic::AttachmentWater;
	}
	else
	{
		return;
	}
	UNiagaraSystem* System = ResolveElementSystem(static_cast<uint8>(Semantic), GetOwner());
	if (System && ResolveHurtVfxRoot())
	{
		ElementAttachmentEffect = UNiagaraFunctionLibrary::SpawnSystemAttached(System,
		                                                                       ResolveHurtVfxRoot(),
		                                                                       NAME_None,
		                                                                       FVector::ZeroVector,
		                                                                       FRotator::ZeroRotator,
		                                                                       FVector::OneVector,
		                                                                       EAttachLocation::KeepRelativeOffset,
		                                                                       false,
		                                                                       ENCPoolMethod::None,
		                                                                       true);
		if (ElementAttachmentEffect)
		{
			ElementAttachmentEffect->SetTranslucentSortPriority(ResolveOwnerSortPriority());
			ActiveAttachmentElement = Element;
		}
	}
}

void UReEchoCombatVfxComponent::RefreshBurnStatus(const bool bBurnActive)
{
	if (!bBurnActive)
	{
		StopEffect(BurnStatusEffect);
		return;
	}
	if (BurnStatusEffect)
	{
		if (!BurnStatusEffect->IsActive())
		{
			BurnStatusEffect->Activate(true);
		}
		return;
	}
	UNiagaraSystem* System =
	    ResolveElementSystem(static_cast<uint8>(EReEchoElementReactionVfxSemantic::Burn), GetOwner());
	if (System && ResolveHurtVfxRoot())
	{
		BurnStatusEffect = UNiagaraFunctionLibrary::SpawnSystemAttached(System,
		                                                                ResolveHurtVfxRoot(),
		                                                                NAME_None,
		                                                                FVector::ZeroVector,
		                                                                FRotator::ZeroRotator,
		                                                                FVector::OneVector,
		                                                                EAttachLocation::KeepRelativeOffset,
		                                                                false,
		                                                                ENCPoolMethod::None,
		                                                                true);
		if (BurnStatusEffect)
		{
			BurnStatusEffect->SetTranslucentSortPriority(ResolveOwnerSortPriority());
		}
	}
}

void UReEchoCombatVfxComponent::SpawnElementReactionAt(const uint8 SemanticValue, AActor* Target) const
{
	UNiagaraSystem* System = ResolveElementSystem(SemanticValue, Target);
	const IReEchoCombatTarget* CombatTarget = Target ? Cast<IReEchoCombatTarget>(Target) : nullptr;
	if (!System || !Target || !CombatTarget || !CombatTarget->IsCombatTargetAlive())
	{
		return;
	}
	UReEchoCombatVfxComponent* TargetVfx = Target->FindComponentByClass<UReEchoCombatVfxComponent>();
	USceneComponent* AttachmentRoot = TargetVfx ? TargetVfx->ResolveHurtVfxRoot() : Target->GetRootComponent();
	if (AttachmentRoot)
	{
		if (UNiagaraComponent* Effect =
		        UNiagaraFunctionLibrary::SpawnSystemAttached(System,
		                                                     AttachmentRoot,
		                                                     NAME_None,
		                                                     FVector::ZeroVector,
		                                                     FRotator::ZeroRotator,
		                                                     FVector::OneVector,
		                                                     EAttachLocation::KeepRelativeOffset,
		                                                     true,
		                                                     ENCPoolMethod::None,
		                                                     true))
		{
			Effect->SetTranslucentSortPriority(TargetVfx ? TargetVfx->ResolveOwnerSortPriority()
			                                             : ResolveOwnerSortPriority());
		}
	}
}

void UReEchoCombatVfxComponent::SpawnConductLink(const FReEchoElementReactionLink& Link) const
{
	AActor* SourceTarget = Link.SourceTarget;
	AActor* TargetTarget = Link.TargetTarget;
	const IReEchoCombatTarget* SourceCombatTarget = SourceTarget ? Cast<IReEchoCombatTarget>(SourceTarget) : nullptr;
	const IReEchoCombatTarget* TargetCombatTarget = TargetTarget ? Cast<IReEchoCombatTarget>(TargetTarget) : nullptr;
	if (!SourceCombatTarget || !TargetCombatTarget || !SourceCombatTarget->IsCombatTargetAlive() ||
	    !TargetCombatTarget->IsCombatTargetAlive() || !GetWorld())
	{
		return;
	}
	UNiagaraSystem* System =
	    ResolveElementSystem(static_cast<uint8>(EReEchoElementReactionVfxSemantic::Conduct), TargetTarget);
	if (!System)
	{
		return;
	}
	const FVector Start = SourceCombatTarget->GetCombatTargetLocation();
	const FVector End = TargetCombatTarget->GetCombatTargetLocation();
	FVector StartParameter = FVector::ZeroVector;
	FVector EndParameter = FVector::ZeroVector;
	ResolveConductLinkWorldEndpoints(Start, End, StartParameter, EndParameter);
	UNiagaraComponent* Effect = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(),
	                                                                           System,
	                                                                           FVector::ZeroVector,
	                                                                           FRotator::ZeroRotator,
	                                                                           FVector::OneVector,
	                                                                           true,
	                                                                           false,
	                                                                           ENCPoolMethod::None,
	                                                                           false);
	if (!Effect)
	{
		return;
	}
	Effect->SetVariablePosition(TEXT("User.StartPosition"), StartParameter);
	Effect->SetVariablePosition(TEXT("User.EndPosition"), EndParameter);
	UE_LOG(LogReEcho,
	       VeryVerbose,
	       TEXT("Conduct Source=%s Actor=%s Combat=%s Target=%s Actor=%s Combat=%s Start=%s End=%s Niagara=%s"),
	       *GetNameSafe(SourceTarget),
	       *SourceTarget->GetActorLocation().ToCompactString(),
	       *Start.ToCompactString(),
	       *GetNameSafe(TargetTarget),
	       *TargetTarget->GetActorLocation().ToCompactString(),
	       *End.ToCompactString(),
	       *StartParameter.ToCompactString(),
	       *EndParameter.ToCompactString(),
	       *Effect->GetComponentTransform().ToHumanReadableString());
	if (const UReEchoCombatVfxComponent* TargetVfx = TargetTarget->FindComponentByClass<UReEchoCombatVfxComponent>())
	{
		Effect->SetTranslucentSortPriority(TargetVfx->ResolveOwnerSortPriority());
	}
	Effect->Activate(true);
}

void UReEchoCombatVfxComponent::CancelConductPropagation()
{
	++ConductBatchSerial;
	if (UWorld* World = GetWorld())
	{
		for (FTimerHandle& Handle : ConductPropagationTimers)
		{
			World->GetTimerManager().ClearTimer(Handle);
		}
	}
	ConductPropagationTimers.Reset();
}

void UReEchoCombatVfxComponent::ScheduleConductLinks(const FReEchoElementReactionResolvedEvent& Event)
{
	const float DelaySeconds = ResolveConductPropagationDelaySeconds(Event.Attack.WeaponId);
	ScheduleConductLinksWithDelay(Event, DelaySeconds);
}

void UReEchoCombatVfxComponent::ScheduleConductLinksWithDelay(const FReEchoElementReactionResolvedEvent& Event,
                                                              const float DelaySeconds)
{
	CancelConductPropagation();
	const uint64 BatchSerial = ConductBatchSerial;
	for (int32 Index = 0; Index < Event.ReactionLinks.Num(); ++Index)
	{
		const FReEchoElementReactionLink& Link = Event.ReactionLinks[Index];
		const float ScheduledTime = ResolveConductLinkScheduledTime(Index, DelaySeconds);
		if (ScheduledTime <= 0.0f)
		{
			SpawnConductLink(Link);
			continue;
		}
		if (!GetWorld())
		{
			continue;
		}
		const TWeakObjectPtr<UReEchoCombatVfxComponent> WeakThis(this);
		const TWeakObjectPtr<AActor> WeakSource(Link.SourceTarget);
		const TWeakObjectPtr<AActor> WeakTarget(Link.TargetTarget);
		FTimerHandle& Handle = ConductPropagationTimers.AddDefaulted_GetRef();
		GetWorld()->GetTimerManager().SetTimer(Handle,
		                                       FTimerDelegate::CreateLambda(
		                                           [WeakThis, WeakSource, WeakTarget, BatchSerial]()
		                                           {
			                                           UReEchoCombatVfxComponent* Component = WeakThis.Get();
			                                           if (!Component || Component->ConductBatchSerial != BatchSerial ||
			                                               !WeakSource.IsValid() || !WeakTarget.IsValid())
			                                           {
				                                           return;
			                                           }
			                                           FReEchoElementReactionLink DelayedLink;
			                                           DelayedLink.SourceTarget = WeakSource.Get();
			                                           DelayedLink.TargetTarget = WeakTarget.Get();
			                                           Component->SpawnConductLink(DelayedLink);
		                                           }),
		                                       ScheduledTime,
		                                       false);
	}
}

#if WITH_DEV_AUTOMATION_TESTS
void UReEchoCombatVfxComponent::ScheduleConductLinksForTests(const FReEchoElementReactionResolvedEvent& Event,
                                                             const float DelaySeconds)
{
	ScheduleConductLinksWithDelay(Event, DelaySeconds);
}
#endif

void UReEchoCombatVfxComponent::HandleAttackCommitted(const FReEchoAttackCommittedEvent& Event)
{
	EReEchoCombatVfxSemantic Semantic = EReEchoCombatVfxSemantic::PlayerMeleeSlash;
	if (!FReEchoCombatVfxCatalog::ResolveMeleeAttackSemantic(Event.AttackPatternId, Semantic))
	{
		return;
	}
	SpawnAttached(static_cast<uint8>(Semantic), Event.Direction, ResolveAttackVfxRoot());
}

void UReEchoCombatVfxComponent::HandleHurt(const FReEchoDamageEvent& Event)
{
	if (Event.Target != GetOwner() || Event.AppliedDamage <= 0.0f || Event.bFatal)
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

void UReEchoCombatVfxComponent::HandleElementStateChanged(const FReEchoElementStateChangedEvent& Event)
{
	if (Event.Combatant && Event.Combatant->GetOwner() == GetOwner())
	{
		RefreshElementEffects(Event.State);
	}
}

void UReEchoCombatVfxComponent::HandleElementReactionResolved(const FReEchoElementReactionResolvedEvent& Event)
{
	EReEchoElementReactionVfxSemantic Semantic;
	if (IsElementReactionStateDriven(Event.ReactionId))
	{
		// Burn is driven by the authoritative timed status; Growth is driven by each target's attached Grass state.
		return;
	}
	if (Event.ReactionId == TEXT("Y_ER_L_W"))
	{
		ScheduleConductLinks(Event);
		return;
	}
	if (Event.ReactionId == TEXT("Y_ER_F_W"))
	{
		Semantic = EReEchoElementReactionVfxSemantic::Vaporize;
	}
	else if (Event.ReactionId == TEXT("Y_ER_G_W"))
	{
		Semantic = EReEchoElementReactionVfxSemantic::EnhanceGrass;
	}
	else if (Event.ReactionId == TEXT("Y_ER_W_G"))
	{
		Semantic = EReEchoElementReactionVfxSemantic::EnhanceWater;
	}
	else
	{
		return;
	}
	for (AActor* Target : Event.AffectedTargets)
	{
		SpawnElementReactionAt(static_cast<uint8>(Semantic), Target);
	}
}

void UReEchoCombatVfxComponent::HandlePresentationAction(const FReEchoPresentationActionEvent& Event)
{
	const bool bRabbit = Event.Key.AbilityId == TEXT("M_RABBIT_RangedBurst");
	const bool bFox = Event.Key.AbilityId == TEXT("M_FOX_Dash");
	if (!bRabbit && !bFox)
	{
		return;
	}
	if (Event.Phase == EReEchoPresentationActionPhase::Windup)
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
	if (Event.Phase == EReEchoPresentationActionPhase::Committed && bFox)
	{
		StopEffect(DashEffect);
		DashEffect = SpawnAttached(static_cast<uint8>(EReEchoCombatVfxSemantic::FoxDash),
		                           Event.LockedDirection,
		                           ResolveAttackVfxRoot(),
		                           false);
	}
	else if (Event.Phase == EReEchoPresentationActionPhase::Ended ||
	         Event.Phase == EReEchoPresentationActionPhase::Cancelled)
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
		const TArray<UMaterialInterface*> GlowMaterials = ResolveRabbitProjectileGlowMaterials();
		if (Owner && Texture && Material)
		{
			UMaterialBillboardComponent* Visual = NewObject<UMaterialBillboardComponent>(Owner);
			Owner->AddInstanceComponent(Visual);
			Visual->SetMobility(EComponentMobility::Movable);
			Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Visual->SetCastShadow(false);
			const float CoreWorldDiameter = ResolveProjectileCoreDiameter(Event.CollisionRadiusCm);
			const float GlowWorldDiameter = ResolveProjectileGlowDiameter(Event.CollisionRadiusCm);
			for (UMaterialInterface* GlowMaterial : GlowMaterials)
			{
				Visual->AddElement(GlowMaterial, nullptr, false, GlowWorldDiameter, GlowWorldDiameter, nullptr);
			}
			Visual->AddElement(Material, nullptr, false, CoreWorldDiameter, CoreWorldDiameter, nullptr);
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
