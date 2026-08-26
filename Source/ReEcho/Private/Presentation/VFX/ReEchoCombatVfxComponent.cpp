#include "Presentation/VFX/ReEchoCombatVfxComponent.h"

#include "Combat/ReEchoCombatantComponent.h"
#include "Combat/ReEchoCombatTarget.h"
#include "Camera/PlayerCameraManager.h"
#include "Data/ReEchoCsvDataRegistry.h"
#include "Graybox/ReEchoEnemyActor.h"
#include "Components/MaterialBillboardComponent.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraEmitter.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraMeshRendererProperties.h"
#include "NiagaraSpriteRendererProperties.h"
#include "NiagaraSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Math/RotationMatrix.h"
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
constexpr float DebugElementReactionPreviewSeconds = 2.0f;
constexpr int32 EchoAuraSortOffset = -1;
const FBox FoxDirectionRuntimeBounds(FVector(-500.0f, -500.0f, -650.0f), FVector(500.0f, 500.0f, 350.0f));
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

bool UReEchoCombatVfxComponent::SetNiagaraSystemEmittersLocalSpace(UNiagaraSystem* System)
{
#if WITH_EDITOR
	if (!System)
	{
		return false;
	}
	System->Modify();
	bool bHasEnabledEmitter = false;
	for (FNiagaraEmitterHandle& EmitterHandle : System->GetEmitterHandles())
	{
		if (!EmitterHandle.GetIsEnabled())
		{
			continue;
		}
		FVersionedNiagaraEmitterData* EmitterData = EmitterHandle.GetEmitterData();
		if (!EmitterData)
		{
			return false;
		}
		bHasEnabledEmitter = true;
		if (UNiagaraEmitterBase* EmitterBase = EmitterHandle.GetEmitterBase())
		{
			EmitterBase->Modify();
		}
		EmitterData->bLocalSpace = true;
	}
	// The attached Direction arrow can be culled before its first dynamic-bounds update. Its authored
	// fixed box is already non-degenerate, so the named Editor repair also opts the System into that box.
	if (bHasEnabledEmitter && System->GetFixedBounds().IsValid)
	{
		System->bFixedBounds = true;
	}
	System->MarkPackageDirty();
	return bHasEnabledEmitter && System->bFixedBounds != 0;
#else
	return false;
#endif
}

UReEchoCombatVfxComponent::UReEchoCombatVfxComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

int32 UReEchoCombatVfxComponent::ResolveCombatEffectSortPriority(const int32 OwnerSortPriority)
{
	return FMath::Max(ReEchoCombatVfx::CombatEffectSortPriorityFloor,
	                  OwnerSortPriority + ReEchoCombatVfx::CombatEffectSortOffset);
}

int32 UReEchoCombatVfxComponent::ResolveEchoAuraSortPriority(const int32 OwnerSortPriority)
{
	return OwnerSortPriority + ReEchoCombatVfx::EchoAuraSortOffset;
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

bool UReEchoCombatVfxComponent::TryResolveDebugElementReactionSemantic(const FName ReactionName,
                                                                       uint8& OutSemanticValue)
{
	EReEchoElementReactionVfxSemantic Semantic;
	if (ReactionName.IsEqual(TEXT("Burn"), ENameCase::IgnoreCase))
	{
		Semantic = EReEchoElementReactionVfxSemantic::Burn;
	}
	else if (ReactionName.IsEqual(TEXT("Vaporize"), ENameCase::IgnoreCase))
	{
		Semantic = EReEchoElementReactionVfxSemantic::Vaporize;
	}
	else if (ReactionName.IsEqual(TEXT("Growth"), ENameCase::IgnoreCase))
	{
		Semantic = EReEchoElementReactionVfxSemantic::Growth;
	}
	else if (ReactionName.IsEqual(TEXT("Conduct"), ENameCase::IgnoreCase))
	{
		Semantic = EReEchoElementReactionVfxSemantic::Conduct;
	}
	else if (ReactionName.IsEqual(TEXT("EnhanceGrass"), ENameCase::IgnoreCase))
	{
		Semantic = EReEchoElementReactionVfxSemantic::EnhanceGrass;
	}
	else if (ReactionName.IsEqual(TEXT("EnhanceWater"), ENameCase::IgnoreCase))
	{
		Semantic = EReEchoElementReactionVfxSemantic::EnhanceWater;
	}
	else
	{
		return false;
	}
	OutSemanticValue = static_cast<uint8>(Semantic);
	return true;
}

bool UReEchoCombatVfxComponent::PlayElementReactionForDebug(const uint8 SemanticValue, AActor* Target) const
{
	UNiagaraComponent* Effect = SpawnElementReactionAt(SemanticValue, Target);
	UWorld* World = GetWorld();
	if (!Effect || !World)
	{
		return false;
	}
	TWeakObjectPtr<UNiagaraComponent> WeakEffect = Effect;
	FTimerHandle StopTimer;
	World->GetTimerManager().SetTimer(StopTimer,
	                                  FTimerDelegate::CreateWeakLambda(this,
	                                                                   [WeakEffect]()
	                                                                   {
		                                                                   if (WeakEffect.IsValid())
		                                                                   {
			                                                                   WeakEffect->Deactivate();
		                                                                   }
	                                                                   }),
	                                  ReEchoCombatVfx::DebugElementReactionPreviewSeconds,
	                                  false);
	return true;
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
#endif

bool UReEchoCombatVfxComponent::SetNiagaraSystemSpriteFacingOwnerUp(UNiagaraSystem* System)
{
#if WITH_EDITOR
	if (!System)
	{
		return false;
	}
	System->Modify();
	const FNiagaraVariable GroundNormalParameter(FNiagaraTypeDefinition::GetVec3Def(), TEXT("User.GroundNormal"));
	System->GetExposedParameters().SetParameterValue(FVector3f::UpVector, GroundNormalParameter, true);
	bool bModifiedSpriteRenderer = false;
	for (FNiagaraEmitterHandle& EmitterHandle : System->GetEmitterHandles())
	{
		if (!EmitterHandle.GetIsEnabled())
		{
			continue;
		}
		FVersionedNiagaraEmitterData* EmitterData = EmitterHandle.GetEmitterData();
		UNiagaraEmitterBase* EmitterBase = EmitterHandle.GetEmitterBase();
		if (!EmitterData || !EmitterBase)
		{
			return false;
		}
		EmitterBase->Modify();
		const FVersionedNiagaraEmitterBase VersionedEmitter = EmitterHandle.GetInstance().ToBase();
		for (UNiagaraRendererProperties* Renderer : EmitterData->GetRenderers())
		{
			UNiagaraSpriteRendererProperties* Sprite = Cast<UNiagaraSpriteRendererProperties>(Renderer);
			if (!Sprite)
			{
				continue;
			}
			Sprite->Modify();
			Sprite->FacingMode = ENiagaraSpriteFacingMode::CustomFacingVector;
			Sprite->SpriteFacingBinding.SetValue(TEXT("User.GroundNormal"), VersionedEmitter, Sprite->SourceMode);
			if (!Sprite->SpriteFacingBinding.DoesBindingExistOnSource())
			{
				UE_LOG(LogReEcho,
				       Error,
				       TEXT("[VFX] User.GroundNormal is not a valid renderer source for emitter '%s'"),
				       *EmitterHandle.GetName().ToString());
				return false;
			}
			Sprite->PostEditChange();
			bModifiedSpriteRenderer = true;
		}
	}
	if (bModifiedSpriteRenderer)
	{
		System->RequestCompile(true);
		System->MarkPackageDirty();
	}
	return bModifiedSpriteRenderer;
#else
	return false;
#endif
}

bool UReEchoCombatVfxComponent::SetNiagaraSystemMeshFacingCameraPlane(UNiagaraSystem* System)
{
#if WITH_EDITOR
	if (!System)
	{
		return false;
	}
	System->Modify();
	bool bModifiedMeshRenderer = false;
	for (FNiagaraEmitterHandle& EmitterHandle : System->GetEmitterHandles())
	{
		if (!EmitterHandle.GetIsEnabled())
		{
			continue;
		}
		FVersionedNiagaraEmitterData* EmitterData = EmitterHandle.GetEmitterData();
		UNiagaraEmitterBase* EmitterBase = EmitterHandle.GetEmitterBase();
		if (!EmitterData || !EmitterBase)
		{
			return false;
		}
		EmitterBase->Modify();
		for (UNiagaraRendererProperties* Renderer : EmitterData->GetRenderers())
		{
			UNiagaraMeshRendererProperties* Mesh = Cast<UNiagaraMeshRendererProperties>(Renderer);
			if (!Mesh)
			{
				continue;
			}
			Mesh->Modify();
			Mesh->FacingMode = ENiagaraMeshFacingMode::CameraPlane;
			Mesh->bLockedAxisEnable = true;
			Mesh->LockedAxis = FVector::UpVector;
			Mesh->LockedAxisSpace = ENiagaraMeshLockedAxisSpace::World;
			Mesh->PostEditChange();
			bModifiedMeshRenderer = true;
		}
	}
	if (bModifiedMeshRenderer)
	{
		System->RequestCompile(true);
		System->MarkPackageDirty();
	}
	return bModifiedMeshRenderer;
#else
	return false;
#endif
}

#if WITH_DEV_AUTOMATION_TESTS
int32 UReEchoCombatVfxComponent::GetBossProjectileEffectCountForTests() const
{
	return BossProjectileEffects.Num();
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
                                                         USceneComponent* InHurtVfxRoot,
                                                         USceneComponent* InBossWeaponVfxRoot)
{
	AttackVfxRoot = InAttackVfxRoot;
	HurtVfxRoot = InHurtVfxRoot;
	BossWeaponVfxRoot = InBossWeaponVfxRoot;
}

void UReEchoCombatVfxComponent::ConfigureEchoAuraRoot(USceneComponent* InEchoAuraVfxRoot)
{
	EchoAuraVfxRoot = InEchoAuraVfxRoot;
}

void UReEchoCombatVfxComponent::PlayEchoCardAuraPulse(const bool bPlayWater, const bool bPlayGrass)
{
	USceneComponent* AuraRoot = ResolveEchoAuraVfxRoot();
	auto PlayPulse = [this, AuraRoot](const EReEchoCombatVfxSemantic Semantic)
	{
		if (UNiagaraComponent* Effect =
		        SpawnAttached(static_cast<uint8>(Semantic), FVector::ForwardVector, AuraRoot, true))
		{
			Effect->SetTranslucentSortPriority(ResolveOwnerAuraSortPriority());
		}
	};
	if (bPlayWater)
	{
		PlayPulse(EReEchoCombatVfxSemantic::EchoWaterAura);
	}
	if (bPlayGrass)
	{
		PlayPulse(EReEchoCombatVfxSemantic::EchoGrassAura);
	}
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

void UReEchoCombatVfxComponent::TickComponent(const float DeltaTime,
                                              const ELevelTick TickType,
                                              FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	for (int32 Index = ReverseMeleePlaybacks.Num() - 1; Index >= 0; --Index)
	{
		FReverseMeleePlayback& Playback = ReverseMeleePlaybacks[Index];
		UNiagaraComponent* Effect = Playback.Effect.Get();
		Playback.RemainingSeconds = FMath::Max(0.0f, Playback.RemainingSeconds - DeltaTime);
		if (!Effect || Playback.RemainingSeconds <= 0.0f)
		{
			StopNiagaraEffect(Effect);
			ReverseMeleePlaybacks.RemoveAtSwap(Index);
			continue;
		}
		Effect->SetDesiredAge(Playback.RemainingSeconds);
	}
}

void UReEchoCombatVfxComponent::BindEventSources(UReEchoCombatEventsComponent* InCombatEvents,
                                                 UReEchoEnemyEventsComponent* InEnemyEvents)
{
	CancelConductPropagation();
	if (CombatEvents)
	{
		CombatEvents->OnAttackCommitted.RemoveAll(this);
		CombatEvents->OnHit.RemoveAll(this);
		CombatEvents->OnHurt.RemoveAll(this);
		CombatEvents->OnDeath.RemoveAll(this);
		CombatEvents->OnElementStateChanged.RemoveAll(this);
		CombatEvents->OnElementReactionResolved.RemoveAll(this);
	}
	if (EnemyEvents)
	{
		EnemyEvents->OnProjectile.RemoveAll(this);
		EnemyEvents->OnBossIntent.RemoveAll(this);
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
		CombatEvents->OnHit.AddDynamic(this, &UReEchoCombatVfxComponent::HandleHit);
		CombatEvents->OnHurt.AddDynamic(this, &UReEchoCombatVfxComponent::HandleHurt);
		CombatEvents->OnDeath.AddDynamic(this, &UReEchoCombatVfxComponent::HandleDeath);
		CombatEvents->OnElementStateChanged.AddDynamic(this, &UReEchoCombatVfxComponent::HandleElementStateChanged);
		CombatEvents->OnElementReactionResolved.AddDynamic(this,
		                                                   &UReEchoCombatVfxComponent::HandleElementReactionResolved);
	}
	if (EnemyEvents)
	{
		EnemyEvents->OnProjectile.AddDynamic(this, &UReEchoCombatVfxComponent::HandleProjectile);
		EnemyEvents->OnBossIntent.AddDynamic(this, &UReEchoCombatVfxComponent::HandleBossIntent);
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
	const FReEchoVfxPlacement Placement = FReEchoCombatVfxCatalog::ResolvePlacement(Semantic);
	const FRotator DirectionRotation = FReEchoCombatVfxCatalog::ResolveRotation(Semantic, Direction);
	const FQuat WorldRotation = DirectionRotation.Quaternion() * Placement.LocalRotation.Quaternion();
	const FVector WorldLocation = Location + DirectionRotation.RotateVector(Placement.LocalOffset);
	UNiagaraComponent* Effect = UNiagaraFunctionLibrary::SpawnSystemAtLocation(World,
	                                                                           System,
	                                                                           WorldLocation,
	                                                                           WorldRotation.Rotator(),
	                                                                           Placement.Scale,
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

void UReEchoCombatVfxComponent::ResolveBossBeamWorldEndpoints(
    const FVector& Origin, const FVector&, const float LengthCm, FVector& OutStart, FVector& OutEnd)
{
	OutStart = Origin;
	// Skill04 is authored as an upward world-space column rooted at its locked warning center.
	OutEnd = Origin + FVector::ForwardVector * FMath::Max(0.0f, LengthCm);
}

FVector UReEchoCombatVfxComponent::ResolveAttachedScale(const FVector& DesiredScale,
                                                        const FVector& AttachmentWorldScale,
                                                        const bool bPreserveWorldSize)
{
	if (!bPreserveWorldSize)
	{
		return DesiredScale;
	}
	auto SafeDivide = [](const float Value, const float Divisor)
	{
		return FMath::Abs(Divisor) > UE_SMALL_NUMBER ? Value / Divisor : Value;
	};
	return FVector(SafeDivide(DesiredScale.X, AttachmentWorldScale.X),
	               SafeDivide(DesiredScale.Y, AttachmentWorldScale.Y),
	               SafeDivide(DesiredScale.Z, AttachmentWorldScale.Z));
}

bool UReEchoCombatVfxComponent::ConfigureMeleeNiagaraComponentFacing(UNiagaraSystem* System)
{
#if WITH_EDITOR
	if (!System)
	{
		return false;
	}
	System->Modify();
	bool bHasEnabledEmitter = false;
	bool bHasMeshRenderer = false;
	for (FNiagaraEmitterHandle& EmitterHandle : System->GetEmitterHandles())
	{
		if (!EmitterHandle.GetIsEnabled())
		{
			continue;
		}
		FVersionedNiagaraEmitterData* EmitterData = EmitterHandle.GetEmitterData();
		if (!EmitterData)
		{
			return false;
		}
		bHasEnabledEmitter = true;
		if (UNiagaraEmitterBase* EmitterBase = EmitterHandle.GetEmitterBase())
		{
			EmitterBase->Modify();
		}
		EmitterData->bLocalSpace = true;
		for (UNiagaraRendererProperties* Renderer : EmitterData->GetRenderers())
		{
			if (UNiagaraMeshRendererProperties* MeshRenderer = Cast<UNiagaraMeshRendererProperties>(Renderer))
			{
				MeshRenderer->Modify();
				MeshRenderer->FacingMode = ENiagaraMeshFacingMode::Default;
				bHasMeshRenderer = true;
			}
		}
	}
	System->MarkPackageDirty();
	return bHasEnabledEmitter && bHasMeshRenderer;
#else
	return false;
#endif
}

FRotator UReEchoCombatVfxComponent::ComposeAttachedRotation(const FRotator& DirectionRotation,
                                                            const FRotator& LocalRotation)
{
	return (DirectionRotation.Quaternion() * LocalRotation.Quaternion()).Rotator();
}

FRotator UReEchoCombatVfxComponent::ResolveCameraPlaneDirectionRotation(const FVector& Direction,
                                                                        const FVector& CameraFacingNormal)
{
	const FVector Normal = CameraFacingNormal.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
	FVector PlaneDirection = Direction - FVector::DotProduct(Direction, Normal) * Normal;
	PlaneDirection = PlaneDirection.GetSafeNormal(UE_SMALL_NUMBER, FVector::RightVector);
	return FRotationMatrix::MakeFromZX(Normal, PlaneDirection).Rotator();
}

FRotator UReEchoCombatVfxComponent::ResolveSwordMeshDirectionRotation(const FVector& Direction,
                                                                      const FVector& CameraFacingNormal)
{
	const FVector Normal = CameraFacingNormal.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
	FVector PlaneDirection = Direction - FVector::DotProduct(Direction, Normal) * Normal;
	PlaneDirection = PlaneDirection.GetSafeNormal(UE_SMALL_NUMBER, FVector::RightVector);
	// The delivered 0811_01 Niagara mesh is authored in its local YZ plane: local X is the surface normal and
	// local Y is the in-plane attack axis. The DA Roll correction therefore rotates the slash inside the view plane.
	return FRotationMatrix::MakeFromXY(Normal, PlaneDirection).Rotator();
}

FRotator UReEchoCombatVfxComponent::EnsureSwordFrontFacesCamera(const FRotator& ComposedRotation,
                                                                const FVector& CameraFacingNormal)
{
	const FVector CameraNormal = CameraFacingNormal.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
	const FQuat ComposedQuat = ComposedRotation.Quaternion();
	const FVector SurfaceNormal = ComposedQuat.RotateVector(FVector::ForwardVector);
	if (FVector::DotProduct(SurfaceNormal, CameraNormal) >= 0.0f)
	{
		return ComposedRotation;
	}
	const FVector AttackAxis = ComposedQuat.RotateVector(FVector::RightVector).GetSafeNormal();
	return (FQuat(AttackAxis, PI) * ComposedQuat).Rotator();
}

float UReEchoCombatVfxComponent::ResolveMeleePlayDirection(const FVector& AttackDirection, const FVector& CameraRight)
{
	return FVector::DotProduct(AttackDirection, CameraRight) < 0.0f ? 1.0f : -1.0f;
}

bool UReEchoCombatVfxComponent::HasMeleePlayDirectionParameter(const UNiagaraSystem* System)
{
	if (!System)
	{
		return false;
	}
	TArray<FNiagaraVariable> UserParameters;
	System->GetExposedParameters().GetParameters(UserParameters);
	return UserParameters.ContainsByPredicate(
	    [](const FNiagaraVariable& Variable)
	    {
		    return Variable.GetName() == TEXT("PlayDirection");
	    });
}

UNiagaraComponent* UReEchoCombatVfxComponent::SpawnBossBeam(const FReEchoBossIntent& Intent) const
{
	UNiagaraSystem* System = ResolveSystem(static_cast<uint8>(EReEchoCombatVfxSemantic::GoatSkill04Lighting));
	UWorld* World = GetWorld();
	if (!System || !World)
	{
		return nullptr;
	}
	FVector Start = FVector::ZeroVector;
	FVector End = FVector::ZeroVector;
	ResolveBossBeamWorldEndpoints(Intent.LockedTargetLocation, Intent.LockedDirection, Intent.LengthCm, Start, End);
	UNiagaraComponent* Effect = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
	    World,
	    System,
	    Start,
	    FReEchoCombatVfxCatalog::ResolveRotation(EReEchoCombatVfxSemantic::GoatSkill04Lighting, FVector::ForwardVector),
	    FVector::OneVector,
	    false,
	    false,
	    ENCPoolMethod::None,
	    true);
	if (!Effect)
	{
		return nullptr;
	}
	Effect->SetVariablePosition(TEXT("User.StartPosition"), Start);
	Effect->SetVariablePosition(TEXT("User.EndPosition"), End);
	Effect->SetVariableFloat(TEXT("User.BeamLength"), FVector::Dist(Start, End));
	Effect->SetVariableFloat(TEXT("User.BeamWidth"), FMath::Max(0.0f, Intent.WidthCm));
	Effect->SetTranslucentSortPriority(ResolveOwnerSortPriority());
	Effect->Activate(true);
	return Effect;
}

UNiagaraComponent* UReEchoCombatVfxComponent::SpawnAttached(const uint8 SemanticValue,
                                                            const FVector& Direction,
                                                            USceneComponent* AttachmentRoot,
                                                            const bool bAutoDestroy) const
{
	const EReEchoCombatVfxSemantic Semantic = static_cast<EReEchoCombatVfxSemantic>(SemanticValue);
	UNiagaraSystem* System = ResolveSystem(SemanticValue);
	if (!System || !AttachmentRoot)
	{
		return nullptr;
	}
	const FReEchoVfxPlacement Placement = FReEchoCombatVfxCatalog::ResolvePlacement(Semantic);
	FRotator DirectionRotation = FReEchoCombatVfxCatalog::ResolveRotation(Semantic, Direction);
	FVector SwordCameraFacingNormal = FVector::UpVector;
	if (Semantic == EReEchoCombatVfxSemantic::PlayerMeleeSlash)
	{
		const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0);
		SwordCameraFacingNormal = Camera ? -Camera->GetCameraRotation().Vector() : FVector::UpVector;
		DirectionRotation = ResolveSwordMeshDirectionRotation(Direction, SwordCameraFacingNormal);
	}
	else if (Semantic == EReEchoCombatVfxSemantic::PlayerScytheSlash)
	{
		const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0);
		const FVector CameraFacingNormal = Camera ? -Camera->GetCameraRotation().Vector() : FVector::UpVector;
		DirectionRotation = ResolveCameraPlaneDirectionRotation(Direction, CameraFacingNormal);
	}
	FRotator RelativeRotation = ComposeAttachedRotation(DirectionRotation, Placement.LocalRotation);
	if (Semantic == EReEchoCombatVfxSemantic::PlayerMeleeSlash)
	{
		RelativeRotation = EnsureSwordFrontFacesCamera(RelativeRotation, SwordCameraFacingNormal);
	}
	const FVector RelativeScale =
	    ResolveAttachedScale(Placement.Scale,
	                         AttachmentRoot->GetComponentTransform().GetScale3D(),
	                         Placement.ScalePolicy == EReEchoVfxScalePolicy::PreserveWorldSize);
	const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0);
	const FVector CameraRight =
	    Camera ? FRotationMatrix(Camera->GetCameraRotation()).GetUnitAxis(EAxis::Y) : FVector::RightVector;
	const float PlayDirection = Semantic == EReEchoCombatVfxSemantic::PlayerMeleeSlash
	                                ? ResolveMeleePlayDirection(Direction, CameraRight)
	                                : 1.0f;
	const bool bReverseMelee = Semantic == EReEchoCombatVfxSemantic::PlayerMeleeSlash && PlayDirection < 0.0f;
	const bool bHasPlayDirectionParameter = HasMeleePlayDirectionParameter(System);
	UNiagaraComponent* Effect =
	    UNiagaraFunctionLibrary::SpawnSystemAttached(System,
	                                                 AttachmentRoot,
	                                                 NAME_None,
	                                                 Placement.LocalOffset,
	                                                 RelativeRotation,
	                                                 RelativeScale,
	                                                 EAttachLocation::KeepRelativeOffset,
	                                                 bAutoDestroy && !(bReverseMelee && !bHasPlayDirectionParameter),
	                                                 ENCPoolMethod::None,
	                                                 false);
	if (Effect)
	{
		if (Semantic == EReEchoCombatVfxSemantic::FoxDirection)
		{
			// The authored system fixed bounds are only +/-100, while its live camera-facing sprites are centered
			// at local Z=-150 and grow as large as 800x600. Their 500 cm half-diagonal may rotate onto any camera
			// plane axis, so override only this runtime instance without mutating the shared Niagara asset.
			Effect->SetSystemFixedBounds(ReEchoCombatVfx::FoxDirectionRuntimeBounds);
		}
		if (Placement.bUseWorldDirectionRotation)
		{
			// Match projectile presentation: the attack direction is a world-space fact. The DA correction remains
			// local to the effect, while the attachment root continues to own position and lifetime only.
			Effect->SetAbsolute(false, true, false);
			Effect->SetWorldRotation(RelativeRotation);
		}
		if (Semantic == EReEchoCombatVfxSemantic::PlayerMeleeSlash)
		{
			if (bHasPlayDirectionParameter)
			{
				Effect->SetVariableFloat(TEXT("User.PlayDirection"), PlayDirection);
			}
			else if (!bMissingMeleePlayDirectionWarned)
			{
				bMissingMeleePlayDirectionWarned = true;
				UE_LOG(LogReEcho,
				       Warning,
				       TEXT("[VFX] Sword slash '%s' lacks User.PlayDirection; using right-side DesiredAge fallback"),
				       *System->GetPathName());
			}
		}
		Effect->SetTranslucentSortPriority(ResolveOwnerSortPriority());
		Effect->Activate(true);
		if (bReverseMelee && !bHasPlayDirectionParameter)
		{
			// Exact system duration is an empty one-shot frame, so reverse from the preceding rendered frame.
			const float ReverseStartAge = FMath::Max(Placement.PlaybackDurationSeconds - (1.0f / 60.0f), 0.0f);
			Effect->SetAgeUpdateMode(ENiagaraAgeUpdateMode::DesiredAge);
			Effect->SetDesiredAge(ReverseStartAge);
			ReverseMeleePlaybacks.Add({Effect, ReverseStartAge});
		}
		ReEchoCombatVfx::LogLayerStateNowAndDelayed(
		    GetWorld(), GetOwner(), AttachmentRoot, Effect, Semantic, TEXT("Attached"));
	}
	return Effect;
}

USceneComponent* UReEchoCombatVfxComponent::ResolveAttackVfxRoot() const
{
	AActor* Owner = GetOwner();
	return AttackVfxRoot ? AttackVfxRoot.Get() : (Owner ? Owner->GetRootComponent() : nullptr);
}

USceneComponent* UReEchoCombatVfxComponent::ResolveBossWeaponVfxRoot() const
{
	return BossWeaponVfxRoot ? BossWeaponVfxRoot.Get() : ResolveAttackVfxRoot();
}

USceneComponent* UReEchoCombatVfxComponent::ResolveHurtVfxRoot() const
{
	AActor* Owner = GetOwner();
	return HurtVfxRoot ? HurtVfxRoot.Get() : (Owner ? Owner->GetRootComponent() : nullptr);
}

USceneComponent* UReEchoCombatVfxComponent::ResolveEchoAuraVfxRoot() const
{
	AActor* Owner = GetOwner();
	return EchoAuraVfxRoot ? EchoAuraVfxRoot.Get() : (Owner ? Owner->GetRootComponent() : nullptr);
}

int32 UReEchoCombatVfxComponent::ResolveOwnerSortPriority() const
{
	const AActor* Owner = GetOwner();
	const UReEcho2DAnimationComponent* Animation =
	    Owner ? Owner->FindComponentByClass<UReEcho2DAnimationComponent>() : nullptr;
	const int32 OwnerPriority = Animation ? Animation->TranslucencySortPriority : 0;
	return ResolveCombatEffectSortPriority(OwnerPriority);
}

int32 UReEchoCombatVfxComponent::ResolveOwnerAuraSortPriority() const
{
	const AActor* Owner = GetOwner();
	const UReEcho2DAnimationComponent* Animation =
	    Owner ? Owner->FindComponentByClass<UReEcho2DAnimationComponent>() : nullptr;
	return ResolveEchoAuraSortPriority(Animation ? Animation->TranslucencySortPriority : 0);
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

void UReEchoCombatVfxComponent::StopNiagaraEffect(UNiagaraComponent* Effect) const
{
	if (Effect)
	{
		Effect->Deactivate();
		Effect->DestroyComponent();
	}
}

void UReEchoCombatVfxComponent::StopBossActionEffects()
{
	StopEffect(BossChargingEffect);
	StopEffect(BossTelegraphEffect);
	StopEffect(BossActiveEffect);
}

void UReEchoCombatVfxComponent::RememberBossAbility(const int64 AttackSequence, const FName AbilityId)
{
	if (AttackSequence <= 0 || AbilityId.IsNone())
	{
		return;
	}
	BossAbilityByAttackSequence.Add(AttackSequence, AbilityId);
	constexpr int32 MaximumRememberedBossAttacks = 16;
	if (BossAbilityByAttackSequence.Num() <= MaximumRememberedBossAttacks)
	{
		return;
	}
	int64 OldestSequence = TNumericLimits<int64>::Max();
	for (const TPair<int64, FName>& Pair : BossAbilityByAttackSequence)
	{
		OldestSequence = FMath::Min(OldestSequence, Pair.Key);
	}
	BossAbilityByAttackSequence.Remove(OldestSequence);
}

bool UReEchoCombatVfxComponent::TryResolveBossImpactSemantic(const int64 AttackSequence, uint8& OutSemanticValue) const
{
	const FName* AbilityId = BossAbilityByAttackSequence.Find(AttackSequence);
	if (!AbilityId)
	{
		return false;
	}
	if (*AbilityId == TEXT("M_SHEEP_StationaryVolley") || *AbilityId == TEXT("M_SHEEP_MovingSpread"))
	{
		OutSemanticValue = static_cast<uint8>(EReEchoCombatVfxSemantic::GoatSkill02Impact);
		return true;
	}
	return false;
}

void UReEchoCombatVfxComponent::StopAllEffects()
{
	for (FReverseMeleePlayback& Playback : ReverseMeleePlaybacks)
	{
		StopNiagaraEffect(Playback.Effect.Get());
	}
	ReverseMeleePlaybacks.Reset();
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
	for (TPair<FReEchoProjectileVisualKey, TObjectPtr<UNiagaraComponent>>& Pair : BossProjectileEffects)
	{
		StopNiagaraEffect(Pair.Value);
	}
	BossProjectileEffects.Reset();
	StopBossActionEffects();
	BossAbilityByAttackSequence.Reset();
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

UNiagaraComponent* UReEchoCombatVfxComponent::SpawnElementReactionAt(const uint8 SemanticValue, AActor* Target) const
{
	UNiagaraSystem* System = ResolveElementSystem(SemanticValue, Target);
	const IReEchoCombatTarget* CombatTarget = Target ? Cast<IReEchoCombatTarget>(Target) : nullptr;
	if (!System || !Target || !CombatTarget || !CombatTarget->IsCombatTargetAlive())
	{
		return nullptr;
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
			return Effect;
		}
	}
	return nullptr;
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
	const float DelaySeconds = FReEchoCombatVfxCatalog::ResolveMeleeSlashDelay(Semantic);
	if (UWorld* World = GetWorld(); World && DelaySeconds > 0.0f)
	{
		const TWeakObjectPtr<UReEchoCombatVfxComponent> WeakThis(this);
		// Each committed attack owns its delayed slash. Reusing and clearing one handle starves the effect whenever
		// the attack interval is shorter than the authored weapon-spin delay.
		FTimerHandle MeleeSlashTimer;
		World->GetTimerManager().SetTimer(
		    MeleeSlashTimer,
		    [WeakThis, Semantic, LockedDirection = Event.Direction]()
		    {
			    if (const UReEchoCombatVfxComponent* Component = WeakThis.Get())
			    {
				    Component->SpawnAttached(
				        static_cast<uint8>(Semantic), LockedDirection, Component->ResolveAttackVfxRoot());
			    }
		    },
		    DelaySeconds,
		    false);
		return;
	}
	SpawnAttached(static_cast<uint8>(Semantic), Event.Direction, ResolveAttackVfxRoot());
}

void UReEchoCombatVfxComponent::HandleHit(const FReEchoDamageEvent& Event)
{
	if (Event.AppliedDamage <= 0.0f)
	{
		return;
	}
	EReEchoCombatVfxSemantic Semantic = EReEchoCombatVfxSemantic::PlayerLongSwordImpact;
	if (!FReEchoCombatVfxCatalog::ResolveWeaponDamageSemantic(Event.Attack.WeaponId, Semantic))
	{
		return;
	}
	const FVector ImpactDirection = Event.WorldLocation - Event.SourceWorldLocation;
	SpawnWorld(static_cast<uint8>(Semantic), Event.WorldLocation, ImpactDirection);
}

void UReEchoCombatVfxComponent::HandleHurt(const FReEchoDamageEvent& Event)
{
	if (Event.Target != GetOwner() || Event.AppliedDamage <= 0.0f || Event.bFatal)
	{
		return;
	}
	EReEchoCombatVfxSemantic Semantic = Cast<AReEchoEnemyActor>(GetOwner()) ? EReEchoCombatVfxSemantic::EnemyHurt
	                                                                        : EReEchoCombatVfxSemantic::PlayerHurt;
	if (const AReEchoEnemyActor* SourceEnemy = Cast<AReEchoEnemyActor>(Event.Attack.Source.Get());
	    SourceEnemy && SourceEnemy->GetPresentationId() == TEXT("Enemy.Fox"))
	{
		Semantic = EReEchoCombatVfxSemantic::FoxImpact;
	}
	else if (const AReEchoEnemyActor* SourceBoss = Cast<AReEchoEnemyActor>(Event.Attack.Source.Get());
	         SourceBoss && SourceBoss->GetPresentationId() == TEXT("Enemy.TimeGuard"))
	{
		if (const UReEchoCombatVfxComponent* SourceVfx = SourceBoss->FindComponentByClass<UReEchoCombatVfxComponent>())
		{
			uint8 BossImpactSemantic = 0;
			if (SourceVfx->TryResolveBossImpactSemantic(Event.Attack.Sequence, BossImpactSemantic))
			{
				const FVector ImpactLocation = Event.WorldLocation;
				const FVector ImpactDirection = ImpactLocation - Event.SourceWorldLocation;
				SpawnWorld(BossImpactSemantic, ImpactLocation, ImpactDirection);
				return;
			}
		}
	}
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
	else if (Event.Phase == EReEchoPresentationActionPhase::Recovery ||
	         Event.Phase == EReEchoPresentationActionPhase::Ended ||
	         Event.Phase == EReEchoPresentationActionPhase::Cancelled)
	{
		StopEffect(DashEffect);
	}
}

void UReEchoCombatVfxComponent::HandleBossIntent(const FReEchoBossIntent& Intent)
{
	const bool bSkill01 = Intent.AbilityId == TEXT("M_SHEEP_MeleeSweep");
	const bool bSkill02 =
	    Intent.AbilityId == TEXT("M_SHEEP_StationaryVolley") || Intent.AbilityId == TEXT("M_SHEEP_MovingSpread");
	const bool bSkill03 = Intent.AbilityId == TEXT("M_SHEEP_BlinkSlam");
	const bool bSkill04 = Intent.AbilityId == TEXT("M_SHEEP_PrayerBeam");
	if (!bSkill01 && !bSkill02 && !bSkill03 && !bSkill04)
	{
		return;
	}
	RememberBossAbility(Intent.Attack.Sequence, Intent.AbilityId);
	if (bSkill03 && Intent.Type == EReEchoBossIntentType::ImpactResolved)
	{
		SpawnWorld(static_cast<uint8>(EReEchoCombatVfxSemantic::GoatSkill03Impact),
		           Intent.LockedTargetLocation,
		           Intent.LockedDirection);
		return;
	}
	if (Intent.Type == EReEchoBossIntentType::TelegraphStarted)
	{
		StopBossActionEffects();
		if (bSkill01)
		{
			return;
		}
		const EReEchoCombatVfxSemantic ChargingSemantic = bSkill02   ? EReEchoCombatVfxSemantic::GoatSkill02Charging
		                                                  : bSkill03 ? EReEchoCombatVfxSemantic::GoatSkill03Charging
		                                                             : EReEchoCombatVfxSemantic::GoatSkill04Charging;
		BossChargingEffect = SpawnAttached(static_cast<uint8>(ChargingSemantic),
		                                   Intent.LockedDirection,
		                                   bSkill02 ? ResolveBossWeaponVfxRoot() : ResolveAttackVfxRoot(),
		                                   false);
		if (bSkill03 || bSkill04)
		{
			BossTelegraphEffect = SpawnWorld(static_cast<uint8>(EReEchoCombatVfxSemantic::GoatSkill03Alarming),
			                                 Intent.LockedTargetLocation,
			                                 Intent.LockedDirection,
			                                 false);
		}
		return;
	}
	if (Intent.Type == EReEchoBossIntentType::AttackWindowStarted)
	{
		StopEffect(BossChargingEffect);
		StopEffect(BossTelegraphEffect);
		if (bSkill01)
		{
			if (USceneComponent* WeaponRoot = ResolveBossWeaponVfxRoot())
			{
				SpawnWorld(static_cast<uint8>(EReEchoCombatVfxSemantic::GoatSkill02Impact),
				           WeaponRoot->GetComponentLocation(),
				           Intent.LockedDirection);
			}
		}
		if (bSkill04)
		{
			BossTelegraphEffect = SpawnWorld(static_cast<uint8>(EReEchoCombatVfxSemantic::GoatSkill03Alarming),
			                                 Intent.LockedTargetLocation,
			                                 Intent.LockedDirection,
			                                 false);
			StopEffect(BossActiveEffect);
			BossActiveEffect = SpawnBossBeam(Intent);
		}
		return;
	}
	if (Intent.Type == EReEchoBossIntentType::AbilityEnded)
	{
		StopBossActionEffects();
	}
}

void UReEchoCombatVfxComponent::HandleProjectile(const FReEchoEnemyProjectileEvent& Event)
{
	const bool bRabbitProjectile = Event.AbilityId == TEXT("M_RABBIT_RangedBurst");
	const bool bSheepProjectile = Event.AbilityId == TEXT("M_SHEEP_Projectile");
	if ((!bRabbitProjectile && !bSheepProjectile) || Event.Attack.Sequence <= 0 || Event.VolleyBallIndex < 0)
	{
		return;
	}
	const FReEchoProjectileVisualKey Key = ResolveProjectileVisualKey(Event);
	if (bSheepProjectile)
	{
		if (Event.Type == EReEchoEnemyProjectileEventType::Spawned)
		{
			if (TObjectPtr<UNiagaraComponent>* Existing = BossProjectileEffects.Find(Key))
			{
				StopNiagaraEffect(*Existing);
				BossProjectileEffects.Remove(Key);
			}
			if (UNiagaraComponent* Effect = SpawnWorld(static_cast<uint8>(EReEchoCombatVfxSemantic::GoatSkill02Bullet),
			                                           Event.Location,
			                                           Event.Direction,
			                                           false))
			{
				BossProjectileEffects.Add(Key, Effect);
			}
			return;
		}
		if (TObjectPtr<UNiagaraComponent>* Effect = BossProjectileEffects.Find(Key))
		{
			if (*Effect && Event.Type == EReEchoEnemyProjectileEventType::Moved)
			{
				(*Effect)->SetWorldLocationAndRotation(
				    Event.Location,
				    FReEchoCombatVfxCatalog::ResolveRotation(EReEchoCombatVfxSemantic::GoatSkill02Bullet,
				                                             Event.Direction));
			}
			else if (Event.Type == EReEchoEnemyProjectileEventType::Ended)
			{
				StopNiagaraEffect(*Effect);
				BossProjectileEffects.Remove(Key);
			}
		}
		return;
	}
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
