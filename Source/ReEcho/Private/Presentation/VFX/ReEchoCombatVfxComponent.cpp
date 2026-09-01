#include "Presentation/VFX/ReEchoCombatVfxComponent.h"

#include "Combat/ReEchoCombatantComponent.h"
#include "Combat/ReEchoCombatTarget.h"
#include "Camera/PlayerCameraManager.h"
#include "Data/ReEchoCsvDataRegistry.h"
#include "DrawDebugHelpers.h"
#include "Graybox/ReEchoEnemyActor.h"
#include "HAL/IConsoleManager.h"
#include "Components/MaterialBillboardComponent.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraEmitter.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraMeshRendererProperties.h"
#include "NiagaraRibbonRendererProperties.h"
#include "NiagaraSpriteRendererProperties.h"
#include "NiagaraSystem.h"
#include "PaperFlipbook.h"
#include "Kismet/GameplayStatics.h"
#include "Math/RotationMatrix.h"
#include "Presentation/Animation2D/ReEcho2DAnimationComponent.h"
#include "Presentation/Combat/ReEchoCombatPresentationCoordinator.h"
#include "Presentation/VFX/ReEchoCombatVfxCatalog.h"
#include "Presentation/VFX/ReEchoElementReactionVfxCatalog.h"
#include "Presentation/Weapon/ReEchoWeaponPresentationProfile.h"
#include "ReEcho.h"
#include "ReEchoGameMode.h"
#include "TimerManager.h"
#include "Weapons/ReEchoWeaponVisualCatalog.h"

#if WITH_EDITOR
#include "Materials/MaterialInstanceConstant.h"
#include "NiagaraDataInterfaceColorCurve.h"
#include "NiagaraGraph.h"
#include "NiagaraNodeFunctionCall.h"
#include "NiagaraNodeOutput.h"
#include "NiagaraScript.h"
#include "NiagaraScriptSource.h"
#include "ViewModels/Stack/NiagaraStackGraphUtilities.h"
#include "UObject/UObjectHash.h"
#endif

namespace ReEchoCombatVfx
{
constexpr int32 CombatEffectSortOffset = 1;
constexpr int32 CombatEffectSortPriorityFloor = 1000;
constexpr float DebugElementReactionPreviewSeconds = 2.0f;
constexpr float BoundedElementReactionLifetimeSeconds = 2.0f;
constexpr int32 EchoAuraSortOffset = -1;
constexpr float EchoConnectionBoundsPaddingCm = 200.0f;
const FBox FoxDirectionRuntimeBounds(FVector(-500.0f, -500.0f, -650.0f), FVector(500.0f, 500.0f, 350.0f));
const FName FoxDirectionSpriteRotationParameter(TEXT("User.DirectionSpriteRotationDegrees"));
constexpr float RabbitProjectileGlowDiameterScale = 1.5f;
TAutoConsoleVariable<int32> CVarReEchoDebugConductVfx(TEXT("ReEcho.Debug.ConductVfx"),
                                                      0,
                                                      TEXT("Draw and log Conduct world-space endpoints. 0=off, 1=on."),
                                                      ECVF_Cheat);

float ResolveFoxDirectionSpriteRotationDegrees(const FVector& LockedDirection,
                                               const FVector& ViewRight,
                                               const FVector& ViewUp)
{
	if (LockedDirection.IsNearlyZero())
	{
		return 0.0f;
	}
	const FVector SafeViewRight = ViewRight.GetSafeNormal(UE_SMALL_NUMBER, FVector::RightVector);
	const FVector SafeViewUp = ViewUp.GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
	const FVector SafeDirection = LockedDirection.GetSafeNormal();
	const float ScreenRight = FVector::DotProduct(SafeDirection, SafeViewRight);
	// Niagara FaceCamera uses -ResolvedViewUp as its unaligned sprite Up basis. The renderer binding is
	// documented in degrees and its vertex factory converts degrees to radians immediately before sincos.
	const float ScreenUp = FVector::DotProduct(SafeDirection, -SafeViewUp);
	return FMath::IsNearlyZero(ScreenRight) && FMath::IsNearlyZero(ScreenUp)
	           ? 0.0f
	           : FMath::RadiansToDegrees(FMath::Atan2(ScreenUp, ScreenRight));
}

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
	return ReactionId == TEXT("Y_ER_F_G");
}

bool UReEchoCombatVfxComponent::IsBoundedElementReactionSemantic(const uint8 SemanticValue)
{
	const EReEchoElementReactionVfxSemantic Semantic = static_cast<EReEchoElementReactionVfxSemantic>(SemanticValue);
	return Semantic == EReEchoElementReactionVfxSemantic::Vaporize ||
	       Semantic == EReEchoElementReactionVfxSemantic::Growth ||
	       Semantic == EReEchoElementReactionVfxSemantic::EnhanceGrass ||
	       Semantic == EReEchoElementReactionVfxSemantic::EnhanceWater;
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

bool UReEchoCombatVfxComponent::PlayConductLinkForDebug(AActor* SourceTarget, AActor* TargetTarget) const
{
	FReEchoElementReactionLink Link;
	Link.SourceTarget = SourceTarget;
	Link.TargetTarget = TargetTarget;
	return SpawnConductLink(Link);
}

float UReEchoCombatVfxComponent::ResolveConductLinkScheduledTime(const int32 LinkIndex, const float DelaySeconds)
{
	return FMath::Max(0, LinkIndex) * FMath::Max(0.0f, DelaySeconds);
}

bool UReEchoCombatVfxComponent::TryResolveConductLinkAnchors(AActor* SourceTarget,
                                                             AActor* TargetTarget,
                                                             FVector& OutStartWorld,
                                                             FVector& OutEndWorld)
{
	const UReEchoCombatVfxComponent* SourceVfx =
	    SourceTarget ? SourceTarget->FindComponentByClass<UReEchoCombatVfxComponent>() : nullptr;
	const UReEchoCombatVfxComponent* TargetVfx =
	    TargetTarget ? TargetTarget->FindComponentByClass<UReEchoCombatVfxComponent>() : nullptr;
	const USceneComponent* SourceAnchor = SourceVfx ? SourceVfx->HurtVfxRoot.Get() : nullptr;
	const USceneComponent* TargetAnchor = TargetVfx ? TargetVfx->HurtVfxRoot.Get() : nullptr;
	if (!IsValid(SourceAnchor) || !IsValid(TargetAnchor))
	{
		return false;
	}

	OutStartWorld = SourceAnchor->GetComponentLocation();
	OutEndWorld = TargetAnchor->GetComponentLocation();
	return true;
}

void UReEchoCombatVfxComponent::ResolveConductLinkWorldEndpoints(const FVector& StartWorld,
                                                                 const FVector& EndWorld,
                                                                 FVector& OutStartParameter,
                                                                 FVector& OutEndParameter)
{
	OutStartParameter = StartWorld;
	// NS_Element_Electricity's BeamEmitterSetup consumes Beam Start as an absolute position but Beam End as
	// a displacement from that start. Passing a second absolute position makes the rendered endpoint overshoot
	// or reverse even though both gameplay anchors are correct.
	OutEndParameter = EndWorld - StartWorld;
}

bool UReEchoCombatVfxComponent::TryResolveFlipbookCenter(AActor* Target, FVector& OutCenterWorld)
{
	UReEcho2DAnimationComponent* Animation =
	    Target ? Target->FindComponentByClass<UReEcho2DAnimationComponent>() : nullptr;
	const UPaperFlipbook* Flipbook = Animation ? Animation->GetFlipbook() : nullptr;
	if (!Animation || !Flipbook)
	{
		return false;
	}
	OutCenterWorld = Animation->GetComponentTransform().TransformPosition(Flipbook->GetRenderBounds().Origin);
	return true;
}

bool UReEchoCombatVfxComponent::TryResolveFlipbookWorldDiameter(AActor* Target, float& OutDiameterCm)
{
	UReEcho2DAnimationComponent* Animation =
	    Target ? Target->FindComponentByClass<UReEcho2DAnimationComponent>() : nullptr;
	const UPaperFlipbook* Flipbook = Animation ? Animation->GetFlipbook() : nullptr;
	if (!Animation || !Flipbook)
	{
		return false;
	}
	const FBoxSphereBounds WorldBounds = Flipbook->GetRenderBounds().TransformBy(Animation->GetComponentTransform());
	OutDiameterCm = (WorldBounds.BoxExtent * 2.0f).GetMax();
	return OutDiameterCm > UE_SMALL_NUMBER;
}

bool UReEchoCombatVfxComponent::ShouldPlayTargetHurtEffect(const FReEchoDamageEvent& Event, const AActor* Owner)
{
	const bool bTargetIsEnemy = Cast<AReEchoEnemyActor>(Owner) != nullptr;
	return Event.Target == Owner && Event.AppliedDamage > 0.0f &&
	       (!Event.bFatal || bTargetIsEnemy || Event.DamageSource == EReEchoDamageSource::Path);
}

FVector UReEchoCombatVfxComponent::ResolveBossHurtEffectLocation(const FVector& HurtRootWorld,
                                                                 const FVector& FlipbookCenterWorld)
{
	FVector Result = HurtRootWorld;
	Result.Z = FMath::Lerp(HurtRootWorld.Z, FlipbookCenterWorld.Z, 0.5f);
	return Result;
}

FVector UReEchoCombatVfxComponent::ResolveTargetMatchedReactionWorldScale(const UNiagaraSystem* ReactionSystem,
                                                                          const float TargetDiameterCm,
                                                                          const float CoverageRatio,
                                                                          const FVector& FallbackWorldScale)
{
	if (!ReactionSystem || TargetDiameterCm <= UE_SMALL_NUMBER || CoverageRatio <= UE_SMALL_NUMBER)
	{
		return FallbackWorldScale;
	}
	const FBox ReactionBounds = ReactionSystem->GetFixedBounds();
	if (!ReactionBounds.IsValid)
	{
		return FallbackWorldScale;
	}
	const float ReactionDiameter = ReactionBounds.GetSize().GetMax();
	if (ReactionDiameter <= UE_SMALL_NUMBER)
	{
		return FallbackWorldScale;
	}
	return FVector(TargetDiameterCm * CoverageRatio / ReactionDiameter);
}

FBox UReEchoCombatVfxComponent::ResolveConnectionLinkLocalBounds(const FTransform& EffectTransform,
                                                                 const FVector& StartWorld,
                                                                 const FVector& EndWorld,
                                                                 const float PaddingCm)
{
	FBox LocalBounds(EForceInit::ForceInit);
	LocalBounds += EffectTransform.InverseTransformPosition(StartWorld);
	LocalBounds += EffectTransform.InverseTransformPosition(EndWorld);
	return LocalBounds.ExpandBy(FMath::Max(0.0f, PaddingCm));
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
	const FNiagaraVariable GroundTangentParameter(FNiagaraTypeDefinition::GetVec3Def(), TEXT("User.GroundTangent"));
	System->GetExposedParameters().SetParameterValue(FVector3f::UpVector, GroundNormalParameter, true);
	System->GetExposedParameters().SetParameterValue(FVector3f::ForwardVector, GroundTangentParameter, true);
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
			Sprite->Alignment = ENiagaraSpriteAlignment::CustomAlignment;
			Sprite->SpriteFacingBinding.SetValue(TEXT("User.GroundNormal"), VersionedEmitter, Sprite->SourceMode);
			Sprite->SpriteAlignmentBinding.SetValue(TEXT("User.GroundTangent"), VersionedEmitter, Sprite->SourceMode);
			if (!Sprite->SpriteFacingBinding.DoesBindingExistOnSource())
			{
				UE_LOG(LogReEcho,
				       Error,
				       TEXT("[VFX] User.GroundNormal is not a valid renderer source for emitter '%s'"),
				       *EmitterHandle.GetName().ToString());
				return false;
			}
			if (!Sprite->SpriteAlignmentBinding.DoesBindingExistOnSource())
			{
				UE_LOG(LogReEcho,
				       Error,
				       TEXT("[VFX] User.GroundTangent is not a valid renderer source for emitter '%s'"),
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

bool UReEchoCombatVfxComponent::SetEchoBornRendererMaterials(UNiagaraSystem* System)
{
#if WITH_EDITOR
	if (!System)
	{
		return false;
	}
	static const TMap<FString, FString> MaterialReplacements = {
	    {TEXT("/Game/VFX/Monster/Goat/MI/BaseVFX003_Inst15.BaseVFX003_Inst15"),
	     TEXT("/Game/VFX/Echo/MI/MI_Echo_Born_Inst15.MI_Echo_Born_Inst15")},
	    {TEXT("/Game/VFX/Monster/Goat/MI/BaseVFX003_Inst21.BaseVFX003_Inst21"),
	     TEXT("/Game/VFX/Echo/MI/MI_Echo_Born_Inst21.MI_Echo_Born_Inst21")},
	    {TEXT("/Game/VFX/Monster/Goat/MI/BaseVFX003_Inst22.BaseVFX003_Inst22"),
	     TEXT("/Game/VFX/Echo/MI/MI_Echo_Born_Inst22.MI_Echo_Born_Inst22")},
	    {TEXT("/Game/VFX/Monster/Goat/MI/BaseVFX003_Inst23.BaseVFX003_Inst23"),
	     TEXT("/Game/VFX/Echo/MI/MI_Echo_Born_Inst23.MI_Echo_Born_Inst23")},
	};
	auto ResolveReplacement = [](UMaterialInterface* Material) -> UMaterialInterface*
	{
		if (!Material)
		{
			return nullptr;
		}
		if (Material->GetPathName().StartsWith(TEXT("/Game/VFX/Echo/MI/MI_Echo_Born_")))
		{
			return Material;
		}
		const FString* ReplacementPath = MaterialReplacements.Find(Material->GetPathName());
		return ReplacementPath ? LoadObject<UMaterialInterface>(nullptr, **ReplacementPath) : nullptr;
	};
	bool bModified = false;
	for (FNiagaraEmitterHandle& EmitterHandle : System->GetEmitterHandles())
	{
		FVersionedNiagaraEmitterData* EmitterData = EmitterHandle.GetEmitterData();
		UNiagaraEmitterBase* EmitterBase = EmitterHandle.GetEmitterBase();
		if (!EmitterData || !EmitterBase)
		{
			continue;
		}
		for (UNiagaraRendererProperties* Renderer : EmitterData->GetRenderers())
		{
			if (UNiagaraSpriteRendererProperties* Sprite = Cast<UNiagaraSpriteRendererProperties>(Renderer))
			{
				if (UMaterialInterface* Replacement = ResolveReplacement(Sprite->Material))
				{
					EmitterBase->Modify();
					Sprite->Modify();
					Sprite->Material = Replacement;
					Sprite->PostEditChange();
					bModified = true;
				}
				continue;
			}
			if (UNiagaraRibbonRendererProperties* Ribbon = Cast<UNiagaraRibbonRendererProperties>(Renderer))
			{
				if (UMaterialInterface* Replacement = ResolveReplacement(Ribbon->Material))
				{
					EmitterBase->Modify();
					Ribbon->Modify();
					Ribbon->Material = Replacement;
					Ribbon->PostEditChange();
					bModified = true;
				}
				continue;
			}
			UNiagaraMeshRendererProperties* Mesh = Cast<UNiagaraMeshRendererProperties>(Renderer);
			if (!Mesh)
			{
				continue;
			}
			bool bMeshModified = false;
			for (FNiagaraMeshMaterialOverride& Override : Mesh->OverrideMaterials)
			{
				if (UMaterialInterface* Replacement = ResolveReplacement(Override.ExplicitMat))
				{
					Override.ExplicitMat = Replacement;
					bMeshModified = true;
				}
			}
			for (FNiagaraMeshMICOverride& Override : Mesh->MICOverrideMaterials)
			{
				if (UMaterialInterface* Replacement = ResolveReplacement(Override.ReplacementMaterial))
				{
					Override.ReplacementMaterial = CastChecked<UMaterialInstanceConstant>(Replacement);
					bMeshModified = true;
				}
			}
			if (bMeshModified)
			{
				EmitterBase->Modify();
				Mesh->Modify();
				Mesh->PostEditChange();
				bModified = true;
			}
		}
	}
	if (bModified)
	{
		System->Modify();
		System->RequestCompile(true);
		System->MarkPackageDirty();
	}
	return bModified;
#else
	return false;
#endif
}

bool UReEchoCombatVfxComponent::CompileNiagaraSystemAndWait(UNiagaraSystem* System)
{
#if WITH_EDITOR
	if (!System)
	{
		return false;
	}
	System->RequestCompile(true);
	System->WaitForCompilationComplete(true, false);
	return !System->HasOutstandingCompilationRequests(true);
#else
	return false;
#endif
}

bool UReEchoCombatVfxComponent::SetEchoBornParticleColors(UNiagaraSystem* System)
{
#if WITH_EDITOR
	if (!System)
	{
		return false;
	}
	// Preserve the selected ice-blue hue while allowing HDR values to make the short-lived circle and particles read.
	constexpr float IceRed = 0.75f;
	constexpr float IceGreen = 1.17f;
	constexpr float IceBlue = 1.50f;
	auto RecolorCurve = [](FRichCurve& Curve,
	                       const FRichCurve& Red,
	                       const FRichCurve& Green,
	                       const FRichCurve& Blue,
	                       const float ChannelScale)
	{
		for (auto Iterator = Curve.GetKeyHandleIterator(); Iterator; ++Iterator)
		{
			const FKeyHandle Handle = *Iterator;
			const float Time = Curve.GetKeyTime(Handle);
			const float Intensity = FMath::Max3(Red.Eval(Time), Green.Eval(Time), Blue.Eval(Time));
			Curve.SetKeyValue(Handle, Intensity * ChannelScale);
		}
	};

	bool bModified = false;
	TArray<UObject*> NestedObjects;
	GetObjectsWithOuter(System, NestedObjects, EGetObjectsFlags::IncludeNestedObjects);
	for (UObject* Object : NestedObjects)
	{
		UNiagaraDataInterfaceColorCurve* ColorCurve = Cast<UNiagaraDataInterfaceColorCurve>(Object);
		if (!ColorCurve)
		{
			continue;
		}
		ColorCurve->Modify();
		const FRichCurve OriginalRed = ColorCurve->RedCurve;
		const FRichCurve OriginalGreen = ColorCurve->GreenCurve;
		const FRichCurve OriginalBlue = ColorCurve->BlueCurve;
		RecolorCurve(ColorCurve->RedCurve, OriginalRed, OriginalGreen, OriginalBlue, IceRed);
		RecolorCurve(ColorCurve->GreenCurve, OriginalRed, OriginalGreen, OriginalBlue, IceGreen);
		RecolorCurve(ColorCurve->BlueCurve, OriginalRed, OriginalGreen, OriginalBlue, IceBlue);
		ColorCurve->UpdateTimeRanges();
		ColorCurve->PostEditChange();
		bModified = true;
	}

	for (FNiagaraEmitterHandle& EmitterHandle : System->GetEmitterHandles())
	{
		FVersionedNiagaraEmitterData* EmitterData = EmitterHandle.GetEmitterData();
		if (!EmitterData)
		{
			continue;
		}
		TArray<UNiagaraScript*> Scripts;
		EmitterData->GetScripts(Scripts, false, false);
		for (UNiagaraScript* Script : Scripts)
		{
			if (!Script)
			{
				continue;
			}
			for (const FNiagaraVariableWithOffset& Parameter :
			     Script->RapidIterationParameters.ReadParameterVariables())
			{
				if (Parameter.GetType() != FNiagaraTypeDefinition::GetColorDef() ||
				    !Parameter.GetName().ToString().Contains(TEXT("Color")))
				{
					continue;
				}
				const FLinearColor Current =
				    Script->RapidIterationParameters.GetParameterValue<FLinearColor>(Parameter);
				const float Intensity = FMath::Max3(Current.R, Current.G, Current.B);
				Script->Modify();
				Script->RapidIterationParameters.SetParameterValue(
				    FLinearColor(Intensity * IceRed, Intensity * IceGreen, Intensity * IceBlue, Current.A), Parameter);
				bModified = true;
			}
		}
	}
	if (bModified)
	{
		System->Modify();
		System->RequestCompile(true);
		System->MarkPackageDirty();
	}
	return bModified;
#else
	return false;
#endif
}

bool UReEchoCombatVfxComponent::SetEchoBornMeshHeightScale(UNiagaraSystem* System)
{
#if WITH_EDITOR
	if (!System)
	{
		return false;
	}
	bool bModified = false;
	for (FNiagaraEmitterHandle& EmitterHandle : System->GetEmitterHandles())
	{
		FVersionedNiagaraEmitterData* EmitterData = EmitterHandle.GetEmitterData();
		if (!EmitterData)
		{
			continue;
		}
		TArray<UNiagaraScript*> Scripts;
		EmitterData->GetScripts(Scripts, false, false);
		for (UNiagaraScript* Script : Scripts)
		{
			if (!Script)
			{
				continue;
			}
			for (const FNiagaraVariableWithOffset& Parameter :
			     Script->RapidIterationParameters.ReadParameterVariables())
			{
				if (Parameter.GetType() != FNiagaraTypeDefinition::GetVec3Def() ||
				    !Parameter.GetName().ToString().EndsWith(TEXT("InitializeParticle.Mesh Scale")))
				{
					continue;
				}
				Script->Modify();
				// Fountain004 retains the source asset's complete authored size. Its relative layer size is owned by
				// Niagara; only the runtime component applies the common Echo Born world-scale normalization.
				Script->RapidIterationParameters.SetParameterValue(FVector3f(25.0f, 25.0f, 40.0f), Parameter);
				bModified = true;
			}
		}
	}
	if (bModified)
	{
		System->Modify();
		System->RequestCompile(true);
		System->MarkPackageDirty();
	}
	return bModified;
#else
	return false;
#endif
}

bool UReEchoCombatVfxComponent::RestoreEchoBornSpriteFacing(UNiagaraSystem* System, UNiagaraSystem* SourceSystem)
{
#if WITH_EDITOR
	if (!System || !SourceSystem || System->GetEmitterHandles().Num() != SourceSystem->GetEmitterHandles().Num())
	{
		return false;
	}
	bool bModified = false;
	for (int32 EmitterIndex = 0; EmitterIndex < System->GetEmitterHandles().Num(); ++EmitterIndex)
	{
		FNiagaraEmitterHandle& DestinationHandle = System->GetEmitterHandles()[EmitterIndex];
		const FNiagaraEmitterHandle& SourceHandle = SourceSystem->GetEmitterHandles()[EmitterIndex];
		FVersionedNiagaraEmitterData* DestinationData = DestinationHandle.GetEmitterData();
		const FVersionedNiagaraEmitterData* SourceData = SourceHandle.GetEmitterData();
		UNiagaraEmitterBase* DestinationBase = DestinationHandle.GetEmitterBase();
		if (!DestinationData || !SourceData || !DestinationBase)
		{
			return false;
		}
		const TArray<UNiagaraRendererProperties*>& DestinationRenderers = DestinationData->GetRenderers();
		const TArray<UNiagaraRendererProperties*>& SourceRenderers = SourceData->GetRenderers();
		if (DestinationRenderers.Num() != SourceRenderers.Num())
		{
			return false;
		}
		for (int32 RendererIndex = 0; RendererIndex < DestinationRenderers.Num(); ++RendererIndex)
		{
			UNiagaraSpriteRendererProperties* DestinationSprite =
			    Cast<UNiagaraSpriteRendererProperties>(DestinationRenderers[RendererIndex]);
			const UNiagaraSpriteRendererProperties* SourceSprite =
			    Cast<UNiagaraSpriteRendererProperties>(SourceRenderers[RendererIndex]);
			if (!DestinationSprite && !SourceSprite)
			{
				continue;
			}
			if (!DestinationSprite || !SourceSprite)
			{
				return false;
			}
			DestinationBase->Modify();
			DestinationSprite->Modify();
			DestinationSprite->FacingMode = SourceSprite->FacingMode;
			DestinationSprite->Alignment = SourceSprite->Alignment;
			DestinationSprite->SpriteFacingBinding = SourceSprite->SpriteFacingBinding;
			DestinationSprite->SpriteAlignmentBinding = SourceSprite->SpriteAlignmentBinding;
			DestinationSprite->PostEditChange();
			bModified = true;
		}
	}
	if (bModified)
	{
		System->Modify();
		System->RequestCompile(true);
		System->MarkPackageDirty();
	}
	return bModified;
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

void UReEchoCombatVfxComponent::ConfigureWeaponAttackVfxRoot(USceneComponent* InWeaponAttackVfxRoot)
{
	WeaponAttackVfxRoot = InWeaponAttackVfxRoot;
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
	if (!EchoConnectionEffects.IsEmpty())
	{
		const IReEchoCombatTarget* OwnerCombatTarget = Cast<IReEchoCombatTarget>(GetOwner());
		if (!OwnerCombatTarget || !OwnerCombatTarget->IsCombatTargetAlive())
		{
			ClearEchoConnectionLinks();
		}
	}
	if (BossActiveEffectRemainingSeconds > 0.0f)
	{
		BossActiveEffectRemainingSeconds = FMath::Max(0.0f, BossActiveEffectRemainingSeconds - DeltaTime);
		if (BossActiveEffectRemainingSeconds <= 0.0f)
		{
			StopEffect(BossTelegraphEffect);
			StopEffect(BossActiveEffect);
		}
	}
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
	for (auto It = EchoConnectionEffects.CreateIterator(); It; ++It)
	{
		AActor* EchoActor = It.Key();
		UNiagaraComponent* Effect = It.Value();
		const IReEchoCombatTarget* EchoCombatTarget =
		    IsValid(EchoActor) ? Cast<IReEchoCombatTarget>(EchoActor) : nullptr;
		if (!EchoCombatTarget || !EchoCombatTarget->IsCombatTargetAlive() || !IsValid(Effect))
		{
			StopNiagaraEffect(Effect);
			It.RemoveCurrent();
			continue;
		}
		UpdateEchoConnectionEffect(EchoActor, Effect);
		if (!Effect->IsActive())
		{
			// The card owns this lifetime. Authored Niagara completion must not remove the link while both endpoints
			// live.
			Effect->Activate(true);
		}
	}
}

bool UReEchoCombatVfxComponent::PlayEchoBornAtWorldLocation(const FVector& GroundWorldLocation) const
{
	const EReEchoCombatVfxSemantic Semantic = EReEchoCombatVfxSemantic::EchoBorn;
	UNiagaraComponent* Effect =
	    SpawnWorld(static_cast<uint8>(Semantic), GroundWorldLocation, FVector::ForwardVector, false, false);
	UWorld* World = GetWorld();
	if (!Effect || !World)
	{
		return false;
	}
	// The caller supplies stable actor XY plus the shadow's ground Z. Niagara stays independent and cannot inherit the
	// hidden Echo actor's visibility or its camera-dependent presentation offset.
	Effect->PrimaryComponentTick.bTickEvenWhenPaused = true;
	Effect->SetTranslucentSortPriority(ResolveOwnerAuraSortPriority());
	Effect->SetVariableVec3(TEXT("User.GroundNormal"), FVector::UpVector);
	Effect->SetVariableVec3(TEXT("User.GroundTangent"), FVector::ForwardVector);
	EchoBornEffect = Effect;
	Effect->Activate(true);
	const float DurationSeconds = FReEchoCombatVfxCatalog::ResolvePlacement(Semantic).PlaybackDurationSeconds;
	const TWeakObjectPtr<UNiagaraComponent> WeakEffect(Effect);
	FTimerHandle LifetimeTimer;
	World->GetTimerManager().SetTimer(
	    LifetimeTimer,
	    [WeakEffect]()
	    {
		    if (UNiagaraComponent* ActiveEffect = WeakEffect.Get())
		    {
			    ActiveEffect->Deactivate();
			    ActiveEffect->DestroyComponent();
		    }
	    },
	    FMath::Max(DurationSeconds, 0.01f),
	    false);
	return true;
}

bool UReEchoCombatVfxComponent::IsEchoBornEffectActive() const
{
	return EchoBornEffect.IsValid() && EchoBornEffect->IsActive();
}

bool UReEchoCombatVfxComponent::EnsureNiagaraUpdateBeamModule(UNiagaraSystem* System)
{
#if WITH_EDITOR
	if (!System)
	{
		return false;
	}
	UNiagaraScript* UpdateBeamScript =
	    LoadObject<UNiagaraScript>(nullptr, TEXT("/Niagara/Modules/Beams/UpdateBeam.UpdateBeam"));
	if (!UpdateBeamScript)
	{
		return false;
	}
	System->Modify();
	bool bAllEmittersReady = true;
	for (FNiagaraEmitterHandle& EmitterHandle : System->GetEmitterHandles())
	{
		if (!EmitterHandle.GetIsEnabled())
		{
			continue;
		}
		FVersionedNiagaraEmitterData* EmitterData = EmitterHandle.GetEmitterData();
		UNiagaraEmitterBase* EmitterBase = EmitterHandle.GetEmitterBase();
		UNiagaraScript* ParticleUpdateScript = EmitterData ? EmitterData->UpdateScriptProps.Script : nullptr;
		UNiagaraScriptSource* ScriptSource =
		    ParticleUpdateScript ? Cast<UNiagaraScriptSource>(ParticleUpdateScript->GetLatestSource()) : nullptr;
		UNiagaraGraph* Graph = ScriptSource ? ScriptSource->NodeGraph : nullptr;
		TArray<UNiagaraNodeOutput*> OutputNodes;
		if (Graph)
		{
			Graph->GetNodesOfClass(OutputNodes);
		}
		UNiagaraNodeOutput** OutputNodeEntry = OutputNodes.FindByPredicate(
		    [](const UNiagaraNodeOutput* Node)
		    {
			    return Node && Node->GetUsage() == ENiagaraScriptUsage::ParticleUpdateScript;
		    });
		UNiagaraNodeOutput* OutputNode = OutputNodeEntry ? *OutputNodeEntry : nullptr;
		if (!EmitterData || !EmitterBase || !Graph || !OutputNode)
		{
			bAllEmittersReady = false;
			continue;
		}
		TArray<UNiagaraNodeFunctionCall*> FunctionCalls;
		Graph->GetNodesOfClass(FunctionCalls);
		const bool bAlreadyPresent = FunctionCalls.ContainsByPredicate(
		    [UpdateBeamScript](const UNiagaraNodeFunctionCall* Node)
		    {
			    return Node && Node->FunctionScript == UpdateBeamScript;
		    });
		if (!bAlreadyPresent)
		{
			EmitterBase->Modify();
			if (!FNiagaraStackGraphUtilities::AddScriptModuleToStack(UpdateBeamScript, *OutputNode))
			{
				bAllEmittersReady = false;
			}
		}
	}
	if (bAllEmittersReady)
	{
		System->RequestCompile(false);
		System->MarkPackageDirty();
	}
	return bAllEmittersReady;
#else
	return false;
#endif
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
                                                         const bool bAutoDestroy,
                                                         const bool bActivateImmediately,
                                                         const bool bUseAutoReleasePool) const
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
	                                                                           bUseAutoReleasePool
	                                                                               ? ENCPoolMethod::AutoRelease
	                                                                               : ENCPoolMethod::None,
	                                                                           bActivateImmediately);
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

FVector UReEchoCombatVfxComponent::ResolveBossBeamGroundOrigin(const FVector& LockedWarningCenter,
                                                               const float GameplayPlaneWorldZ)
{
	FVector GroundOrigin = LockedWarningCenter;
	GroundOrigin.Z = GameplayPlaneWorldZ;
	return GroundOrigin;
}

FVector UReEchoCombatVfxComponent::ResolveGroundAlignedEffectOrigin(const FVector& GroundCenter,
                                                                    const FBox& AuthoredBounds,
                                                                    const FVector& WorldScale)
{
	if (!AuthoredBounds.IsValid)
	{
		return GroundCenter;
	}
	FVector Origin = GroundCenter;
	Origin.Z -= AuthoredBounds.Min.Z * FMath::Abs(WorldScale.Z);
	return Origin;
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

USceneComponent* UReEchoCombatVfxComponent::ResolveBossChargingAttachmentRoot(const bool bSkill03,
                                                                              const int32 BossPhaseIndex,
                                                                              USceneComponent* AttackRoot,
                                                                              USceneComponent* WeaponRoot)
{
	return bSkill03 && BossPhaseIndex >= 3 ? AttackRoot : WeaponRoot;
}

FVector
UReEchoCombatVfxComponent::ResolveBossPhase3ChargingTopWorldLocation(const FBoxSphereBounds& AnimationWorldBounds)
{
	return AnimationWorldBounds.Origin + FVector::UpVector * AnimationWorldBounds.BoxExtent.Z;
}

FVector UReEchoCombatVfxComponent::ResolveAttackRangeScale(const FVector& AuthoredScale,
                                                           const FVector& ScaleMask,
                                                           const float RangeMultiplier,
                                                           const float MinMultiplier,
                                                           const float MaxMultiplier)
{
	const float SafeMin = FMath::Max(0.01f, FMath::Min(MinMultiplier, MaxMultiplier));
	const float SafeMax = FMath::Max(SafeMin, FMath::Max(MinMultiplier, MaxMultiplier));
	const float ClampedMultiplier = FMath::Clamp(RangeMultiplier, SafeMin, SafeMax);
	const FVector SafeMask(FMath::Clamp(ScaleMask.X, 0.0f, 1.0f),
	                       FMath::Clamp(ScaleMask.Y, 0.0f, 1.0f),
	                       FMath::Clamp(ScaleMask.Z, 0.0f, 1.0f));
	return AuthoredScale * (FVector::OneVector + SafeMask * (ClampedMultiplier - 1.0f));
}

FVector UReEchoCombatVfxComponent::ResolveGunMuzzleHorizontalDirection(const FVector& AimDirection,
                                                                       const FVector& CameraRight)
{
	const FVector SafeCameraRight = CameraRight.GetSafeNormal(UE_SMALL_NUMBER, FVector::RightVector);
	return FVector::DotProduct(AimDirection, SafeCameraRight) < 0.0f ? -SafeCameraRight : SafeCameraRight;
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

bool UReEchoCombatVfxComponent::BindNiagaraSpriteRotationToDirectionParameter(UNiagaraSystem* System)
{
#if WITH_EDITOR
	if (!System)
	{
		return false;
	}
	System->Modify();
	const FNiagaraVariable RotationParameter(FNiagaraTypeDefinition::GetFloatDef(),
	                                         ReEchoCombatVfx::FoxDirectionSpriteRotationParameter);
	System->GetExposedParameters().SetParameterValue(0.0f, RotationParameter, true);
	int32 ModifiedSpriteRendererCount = 0;
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
			if (!Sprite || !Sprite->GetIsEnabled())
			{
				continue;
			}
			Sprite->Modify();
			Sprite->SpriteRotationBinding.SetValue(
			    ReEchoCombatVfx::FoxDirectionSpriteRotationParameter, VersionedEmitter, Sprite->SourceMode);
			if (!Sprite->SpriteRotationBinding.DoesBindingExistOnSource() ||
			    Sprite->SpriteRotationBinding.GetParamMapBindableVariable() != RotationParameter)
			{
				UE_LOG(LogReEcho,
				       Error,
				       TEXT("[VFX] %s is not a valid SpriteRotation source for emitter '%s'"),
				       *ReEchoCombatVfx::FoxDirectionSpriteRotationParameter.ToString(),
				       *EmitterHandle.GetName().ToString());
				return false;
			}
			Sprite->PostEditChange();
			++ModifiedSpriteRendererCount;
		}
	}
	if (ModifiedSpriteRendererCount > 0)
	{
		System->RequestCompile(true);
		System->MarkPackageDirty();
	}
	return ModifiedSpriteRendererCount > 0;
#else
	return false;
#endif
}

bool UReEchoCombatVfxComponent::AuditFoxDirectionSpritePivots(UNiagaraSystem* System)
{
#if WITH_EDITOR
	if (!System)
	{
		return false;
	}
	int32 EnabledSpriteRendererCount = 0;
	for (const FNiagaraEmitterHandle& EmitterHandle : System->GetEmitterHandles())
	{
		if (!EmitterHandle.GetIsEnabled())
		{
			continue;
		}
		const FVersionedNiagaraEmitterData* EmitterData = EmitterHandle.GetEmitterData();
		if (!EmitterData)
		{
			return false;
		}
		for (const UNiagaraRendererProperties* Renderer : EmitterData->GetRenderers())
		{
			const UNiagaraSpriteRendererProperties* Sprite = Cast<UNiagaraSpriteRendererProperties>(Renderer);
			if (!Sprite || !Sprite->GetIsEnabled())
			{
				continue;
			}
			const FNiagaraVariable PivotBindingVariable = Sprite->PivotOffsetBinding.GetParamMapBindableVariable();
			UE_LOG(LogReEcho,
			       Display,
			       TEXT("PLAN117_FOX_DIRECTION_PIVOT emitter=%s pivot=(%.9f,%.9f) binding_exists=%d binding=%s"),
			       *EmitterHandle.GetName().ToString(),
			       Sprite->PivotInUVSpace.X,
			       Sprite->PivotInUVSpace.Y,
			       Sprite->PivotOffsetBinding.DoesBindingExistOnSource() ? 1 : 0,
			       *PivotBindingVariable.GetName().ToString());
			++EnabledSpriteRendererCount;
		}
	}
	return EnabledSpriteRendererCount == 2;
#else
	return false;
#endif
}

bool UReEchoCombatVfxComponent::SetFoxDirectionSpritePivots(UNiagaraSystem* System,
                                                            const FVector2D KuangPivotInUvSpace,
                                                            const FVector2D Kuang002PivotInUvSpace)
{
#if WITH_EDITOR
	if (!System)
	{
		return false;
	}
	System->Modify();
	int32 ModifiedSpriteRendererCount = 0;
	for (FNiagaraEmitterHandle& EmitterHandle : System->GetEmitterHandles())
	{
		if (!EmitterHandle.GetIsEnabled())
		{
			continue;
		}
		const FName EmitterName = EmitterHandle.GetName();
		const FVector2D* TargetPivot = nullptr;
		if (EmitterName == TEXT("Kuang"))
		{
			TargetPivot = &KuangPivotInUvSpace;
		}
		else if (EmitterName == TEXT("Kuang002"))
		{
			TargetPivot = &Kuang002PivotInUvSpace;
		}
		else
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
			UNiagaraSpriteRendererProperties* Sprite = Cast<UNiagaraSpriteRendererProperties>(Renderer);
			if (!Sprite || !Sprite->GetIsEnabled())
			{
				continue;
			}
			if (Sprite->PivotOffsetBinding.DoesBindingExistOnSource())
			{
				UE_LOG(LogReEcho,
				       Error,
				       TEXT("[VFX] Fox Direction emitter '%s' has an authored PivotOffset binding; refusing to "
				            "overwrite it"),
				       *EmitterName.ToString());
				return false;
			}
			Sprite->Modify();
			Sprite->PivotInUVSpace = *TargetPivot;
			Sprite->PostEditChange();
			UE_LOG(LogReEcho,
			       Display,
			       TEXT("PLAN117_FOX_DIRECTION_PIVOT_AUTHORED emitter=%s pivot=(%.9f,%.9f)"),
			       *EmitterName.ToString(),
			       TargetPivot->X,
			       TargetPivot->Y);
			++ModifiedSpriteRendererCount;
		}
	}
	if (ModifiedSpriteRendererCount == 2)
	{
		System->RequestCompile(true);
		System->MarkPackageDirty();
	}
	return ModifiedSpriteRendererCount == 2;
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

FRotator UReEchoCombatVfxComponent::ResolveGroundPlaneDirectionRotation(const FVector& Direction)
{
	FVector GroundDirection(Direction.X, Direction.Y, 0.0f);
	GroundDirection = GroundDirection.GetSafeNormal(UE_SMALL_NUMBER, FVector::RightVector);
	// The delivered 0811_01 slash mesh is authored in its local YZ plane. Map its local X surface normal to
	// world-up and its local Y attack axis to the committed direction so the full ring stays parallel to ground.
	return FRotationMatrix::MakeFromXY(FVector::UpVector, GroundDirection).Rotator();
}

FRotator UReEchoCombatVfxComponent::ResolveSwordMeshDirectionRotation(const FVector& Direction)
{
	return ResolveGroundPlaneDirectionRotation(Direction);
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

UNiagaraComponent* UReEchoCombatVfxComponent::SpawnBossBeam(const FReEchoBossIntent& Intent,
                                                            const FVector& GroundOrigin) const
{
	UNiagaraSystem* System = ResolveSystem(static_cast<uint8>(EReEchoCombatVfxSemantic::GoatSkill04Lighting));
	UWorld* World = GetWorld();
	if (!System || !World)
	{
		return nullptr;
	}
	FVector Start = FVector::ZeroVector;
	FVector End = FVector::ZeroVector;
	ResolveBossBeamWorldEndpoints(GroundOrigin, Intent.LockedDirection, Intent.LengthCm, Start, End);
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

FVector UReEchoCombatVfxComponent::ResolveBossTargetGroundLocation(const FReEchoBossIntent& Intent) const
{
	if (const AActor* Target = (Intent.bPhaseOpening || Intent.bGroundedSlam) ? GetOwner() : Intent.Target.Get())
	{
		TArray<USceneComponent*> SceneComponents;
		Target->GetComponents(SceneComponents);
		for (const FName PreferredGroundRoot : {FName(TEXT("GroundRoot")), FName(TEXT("FootRoot"))})
		{
			for (const USceneComponent* Component : SceneComponents)
			{
				if (Component && Component->GetFName() == PreferredGroundRoot)
				{
					return ResolveBossBeamGroundOrigin(Intent.LockedTargetLocation,
					                                   Component->GetComponentLocation().Z);
				}
			}
		}
	}
	FVector GroundLocation = Intent.LockedTargetLocation;
	if (const AReEchoGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AReEchoGameMode>() : nullptr)
	{
		float GameplayPlaneWorldZ = GroundLocation.Z;
		if (GameMode->TryGetActiveArenaGameplayPlaneZ(GameplayPlaneWorldZ))
		{
			GroundLocation = ResolveBossBeamGroundOrigin(GroundLocation, GameplayPlaneWorldZ);
		}
	}
	return GroundLocation;
}

UNiagaraComponent* UReEchoCombatVfxComponent::SpawnAttached(const uint8 SemanticValue,
                                                            const FVector& Direction,
                                                            USceneComponent* AttachmentRoot,
                                                            const bool bAutoDestroy,
                                                            const float AttackRangeMultiplier,
                                                            const bool bUseAutoReleasePool) const
{
	const EReEchoCombatVfxSemantic Semantic = static_cast<EReEchoCombatVfxSemantic>(SemanticValue);
	UNiagaraSystem* System = ResolveSystem(SemanticValue);
	if (!System || !AttachmentRoot)
	{
		return nullptr;
	}
	const FReEchoVfxPlacement Placement = FReEchoCombatVfxCatalog::ResolvePlacement(Semantic);
	const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0);
	const FVector CameraRight =
	    Camera ? FRotationMatrix(Camera->GetCameraRotation()).GetUnitAxis(EAxis::Y) : FVector::RightVector;
	const FVector CameraUp =
	    Camera ? FRotationMatrix(Camera->GetCameraRotation()).GetUnitAxis(EAxis::Z) : FVector::ForwardVector;
	const FVector VisualDirection = Semantic == EReEchoCombatVfxSemantic::PlayerGunMuzzle
	                                    ? ResolveGunMuzzleHorizontalDirection(Direction, CameraRight)
	                                    : Direction;
	FRotator DirectionRotation = FReEchoCombatVfxCatalog::ResolveRotation(Semantic, VisualDirection);
	if (FReEchoCombatVfxCatalog::IsLongSwordSlashSemantic(Semantic))
	{
		DirectionRotation = ResolveSwordMeshDirectionRotation(VisualDirection);
	}
	else if (FReEchoCombatVfxCatalog::IsScytheSlashSemantic(Semantic))
	{
		// The scythe owns a full 360-degree ground sweep. Keep its authored local YZ effect plane parallel to the arena
		// so the slash cannot stand vertically and intersect the floor as the camera pitch changes.
		DirectionRotation = ResolveGroundPlaneDirectionRotation(VisualDirection);
	}
	const FRotator LocalRotationCorrection = FReEchoCombatVfxCatalog::IsLongSwordSlashSemantic(Semantic)
	                                             ? FRotator(0.0f, 0.0f, Placement.LocalRotation.Roll)
	                                             : Placement.LocalRotation;
	const FRotator RelativeRotation = ComposeAttachedRotation(DirectionRotation, LocalRotationCorrection);
	const FVector DesiredScale = Placement.bScaleWithAttackRange
	                                 ? ResolveAttackRangeScale(Placement.Scale,
	                                                           Placement.AttackRangeScaleMask,
	                                                           AttackRangeMultiplier,
	                                                           Placement.MinAttackRangeMultiplier,
	                                                           Placement.MaxAttackRangeMultiplier)
	                                 : Placement.Scale;
	const FVector RelativeScale =
	    ResolveAttachedScale(DesiredScale,
	                         AttachmentRoot->GetComponentTransform().GetScale3D(),
	                         Placement.ScalePolicy == EReEchoVfxScalePolicy::PreserveWorldSize);
	const float PlayDirection = FReEchoCombatVfxCatalog::IsLongSwordSlashSemantic(Semantic)
	                                ? ResolveMeleePlayDirection(VisualDirection, CameraRight)
	                                : 1.0f;
	const bool bReverseMelee = FReEchoCombatVfxCatalog::IsLongSwordSlashSemantic(Semantic) && PlayDirection < 0.0f;
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
	                                                 bUseAutoReleasePool ? ENCPoolMethod::AutoRelease
	                                                                     : ENCPoolMethod::None,
	                                                 false);
	if (Effect)
	{
		if (Semantic == EReEchoCombatVfxSemantic::FoxDirection || Semantic == EReEchoCombatVfxSemantic::PlayerGunMuzzle)
		{
			if (Semantic == EReEchoCombatVfxSemantic::FoxDirection)
			{
				// The authored system fixed bounds are only +/-100, while its live camera-facing sprites grow as large
				// as 800x600 from the local origin. Their 500 cm half-diagonal may rotate onto any camera
				// plane axis, so override only this runtime instance without mutating the shared Niagara asset.
				Effect->SetSystemFixedBounds(ReEchoCombatVfx::FoxDirectionRuntimeBounds);
			}
			// FaceCamera sprites ignore component rotation when orienting their image. Write the attack direction
			// into the renderer-bound screen-space rotation before activation; this also gives gun muzzle sprites
			// an exact 180-degree reversal when the weapon faces left.
			Effect->SetVariableFloat(
			    ReEchoCombatVfx::FoxDirectionSpriteRotationParameter,
			    ReEchoCombatVfx::ResolveFoxDirectionSpriteRotationDegrees(VisualDirection, CameraRight, CameraUp));
		}
		if (Placement.bUseWorldDirectionRotation)
		{
			// Match projectile presentation: the attack direction is a world-space fact. The DA correction remains
			// local to the effect, while the attachment root continues to own position and lifetime only.
			Effect->SetAbsolute(false, true, false);
			Effect->SetWorldRotation(RelativeRotation);
		}
		if (FReEchoCombatVfxCatalog::IsLongSwordSlashSemantic(Semantic))
		{
			Effect->SetVariableVec3(TEXT("User.GroundNormal"), FVector::UpVector);
			const FVector GroundTangent = FVector(VisualDirection.X, VisualDirection.Y, 0.0f)
			                                  .GetSafeNormal(UE_SMALL_NUMBER, FVector::RightVector);
			Effect->SetVariableVec3(TEXT("User.GroundTangent"), GroundTangent);
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
		else if (FReEchoCombatVfxCatalog::IsScytheSlashSemantic(Semantic))
		{
			// Sprite renderers consume the same world-space ground basis as the component-oriented mesh renderers.
			// This applies to the default slash and every element-specific scythe system.
			Effect->SetVariableVec3(TEXT("User.GroundNormal"), FVector::UpVector);
			const FVector GroundTangent = FVector(VisualDirection.X, VisualDirection.Y, 0.0f)
			                                  .GetSafeNormal(UE_SMALL_NUMBER, FVector::RightVector);
			Effect->SetVariableVec3(TEXT("User.GroundTangent"), GroundTangent);
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

USceneComponent* UReEchoCombatVfxComponent::ResolveWeaponAttackVfxRoot() const
{
	return IsValid(WeaponAttackVfxRoot) ? WeaponAttackVfxRoot.Get() : ResolveAttackVfxRoot();
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
	BossActiveEffectRemainingSeconds = 0.0f;
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
	EndBossTransformationEffects();
	ClearEchoConnectionLinks();
	for (FReverseMeleePlayback& Playback : ReverseMeleePlaybacks)
	{
		StopNiagaraEffect(Playback.Effect.Get());
	}
	ReverseMeleePlaybacks.Reset();
	StopEffect(ChargingEffect);
	StopEffect(DirectionEffect);
	StopEffect(DashEffect);
	StopEffect(BurnStatusEffect);
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

void UReEchoCombatVfxComponent::ClearEchoConnectionLinks()
{
	for (TPair<TObjectPtr<AActor>, TObjectPtr<UNiagaraComponent>>& Pair : EchoConnectionEffects)
	{
		StopNiagaraEffect(Pair.Value);
	}
	EchoConnectionEffects.Reset();
}

void UReEchoCombatVfxComponent::UpdateEchoConnectionEffect(AActor* EchoActor, UNiagaraComponent* Effect)
{
	FVector StartWorld = FVector::ZeroVector;
	FVector EndWorld = FVector::ZeroVector;
	if (!Effect || !TryResolveFlipbookCenter(GetOwner(), StartWorld) || !TryResolveFlipbookCenter(EchoActor, EndWorld))
	{
		return;
	}
	FVector StartParameter = FVector::ZeroVector;
	FVector EndParameter = FVector::ZeroVector;
	ResolveConductLinkWorldEndpoints(StartWorld, EndWorld, StartParameter, EndParameter);
	Effect->SetVariablePosition(TEXT("User.StartPosition"), StartParameter);
	Effect->SetVariablePosition(TEXT("User.EndPosition"), EndParameter);
	// The world-spawned component can be far from its rendered ribbon. Cover the current endpoints with a
	// finite runtime bounds so a valid long link is not culled by the asset's authored static bounds.
	Effect->SetSystemFixedBounds(ResolveConnectionLinkLocalBounds(
	    Effect->GetComponentTransform(), StartWorld, EndWorld, ReEchoCombatVfx::EchoConnectionBoundsPaddingCm));
}

void UReEchoCombatVfxComponent::SyncEchoConnectionLinks(const bool bEnabled, const TArray<AActor*>& EchoActors)
{
	const IReEchoCombatTarget* OwnerCombatTarget = Cast<IReEchoCombatTarget>(GetOwner());
	if (!bEnabled || !OwnerCombatTarget || !OwnerCombatTarget->IsCombatTargetAlive() || !GetWorld())
	{
		ClearEchoConnectionLinks();
		return;
	}

	TSet<AActor*> DesiredEchoes;
	for (AActor* EchoActor : EchoActors)
	{
		const IReEchoCombatTarget* EchoCombatTarget = EchoActor ? Cast<IReEchoCombatTarget>(EchoActor) : nullptr;
		FVector StartWorld = FVector::ZeroVector;
		FVector EndWorld = FVector::ZeroVector;
		if (EchoCombatTarget && EchoCombatTarget->IsCombatTargetAlive() &&
		    TryResolveFlipbookCenter(GetOwner(), StartWorld) && TryResolveFlipbookCenter(EchoActor, EndWorld))
		{
			DesiredEchoes.Add(EchoActor);
		}
	}

	for (auto It = EchoConnectionEffects.CreateIterator(); It; ++It)
	{
		if (!IsValid(It.Key()) || !DesiredEchoes.Contains(It.Key()))
		{
			StopNiagaraEffect(It.Value());
			It.RemoveCurrent();
		}
	}

	UNiagaraSystem* System = nullptr;
	for (AActor* EchoActor : DesiredEchoes)
	{
		UNiagaraComponent* Effect = EchoConnectionEffects.FindRef(EchoActor);
		if (!IsValid(Effect))
		{
			EchoConnectionEffects.Remove(EchoActor);
			System = System ? System : ResolveSystem(static_cast<uint8>(EReEchoCombatVfxSemantic::EchoConnectionLine));
			if (!System)
			{
				return;
			}
			Effect = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(),
			                                                        System,
			                                                        FVector::ZeroVector,
			                                                        FRotator::ZeroRotator,
			                                                        FVector::OneVector,
			                                                        false,
			                                                        false,
			                                                        ENCPoolMethod::None,
			                                                        false);
			if (!Effect)
			{
				continue;
			}
			Effect->SetTranslucentSortPriority(ResolveOwnerSortPriority());
			// Parameter endpoints are written by this adapter; make Niagara consume them later in the same frame.
			Effect->AddTickPrerequisiteComponent(this);
			EchoConnectionEffects.Add(EchoActor, Effect);
			UpdateEchoConnectionEffect(EchoActor, Effect);
			Effect->Activate(true);
		}
		else
		{
			UpdateEchoConnectionEffect(EchoActor, Effect);
		}
	}
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
	RefreshBurnStatus(State.bBurnActive);
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
		const FReEchoElementReactionVfxPlacement Placement =
		    FReEchoElementReactionVfxCatalog::ResolvePlacement(EReEchoElementReactionVfxSemantic::Burn);
		FVector ReactionWorldScale = Placement.WorldScale;
		float FlipbookDiameterCm = 0.0f;
		if (Placement.bMatchTargetFlipbookSize && TryResolveFlipbookWorldDiameter(GetOwner(), FlipbookDiameterCm))
		{
			ReactionWorldScale = ResolveTargetMatchedReactionWorldScale(
			    System, FlipbookDiameterCm, Placement.TargetCoverageRatio, Placement.WorldScale);
		}
		const FVector RelativeScale = ResolveAttachedScale(ReactionWorldScale,
		                                                   ResolveHurtVfxRoot()->GetComponentTransform().GetScale3D(),
		                                                   Placement.bPreserveWorldSize);
		BurnStatusEffect = UNiagaraFunctionLibrary::SpawnSystemAttached(System,
		                                                                ResolveHurtVfxRoot(),
		                                                                NAME_None,
		                                                                FVector::ZeroVector,
		                                                                FRotator::ZeroRotator,
		                                                                RelativeScale,
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
		FVector RelativeLocation = FVector::ZeroVector;
		if (const AReEchoEnemyActor* TargetEnemy = Cast<AReEchoEnemyActor>(Target);
		    TargetEnemy && TargetEnemy->GetPresentationId() == TEXT("Enemy.TimeGuard"))
		{
			FVector FlipbookCenterWorld = FVector::ZeroVector;
			if (TryResolveFlipbookCenter(Target, FlipbookCenterWorld))
			{
				const FVector BossReactionWorld =
				    ResolveBossHurtEffectLocation(AttachmentRoot->GetComponentLocation(), FlipbookCenterWorld);
				RelativeLocation = AttachmentRoot->GetComponentTransform().InverseTransformPosition(BossReactionWorld);
			}
		}
		const EReEchoElementReactionVfxSemantic Semantic =
		    static_cast<EReEchoElementReactionVfxSemantic>(SemanticValue);
		const FReEchoElementReactionVfxPlacement Placement =
		    FReEchoElementReactionVfxCatalog::ResolvePlacement(Semantic);
		FVector ReactionWorldScale = Placement.WorldScale;
		float FlipbookDiameterCm = 0.0f;
		if (Placement.bMatchTargetFlipbookSize && TryResolveFlipbookWorldDiameter(Target, FlipbookDiameterCm))
		{
			ReactionWorldScale = ResolveTargetMatchedReactionWorldScale(
			    System, FlipbookDiameterCm, Placement.TargetCoverageRatio, Placement.WorldScale);
		}
		const FVector RelativeScale = ResolveAttachedScale(
		    ReactionWorldScale, AttachmentRoot->GetComponentTransform().GetScale3D(), Placement.bPreserveWorldSize);
		if (UNiagaraComponent* Effect =
		        UNiagaraFunctionLibrary::SpawnSystemAttached(System,
		                                                     AttachmentRoot,
		                                                     NAME_None,
		                                                     RelativeLocation,
		                                                     FRotator::ZeroRotator,
		                                                     RelativeScale,
		                                                     EAttachLocation::KeepRelativeOffset,
		                                                     true,
		                                                     ENCPoolMethod::None,
		                                                     true))
		{
			Effect->SetTranslucentSortPriority(TargetVfx ? TargetVfx->ResolveOwnerSortPriority()
			                                             : ResolveOwnerSortPriority());
			if (UWorld* World = GetWorld(); World && IsBoundedElementReactionSemantic(SemanticValue))
			{
				TWeakObjectPtr<UNiagaraComponent> WeakEffect = Effect;
				FTimerHandle StopTimer;
				World->GetTimerManager().SetTimer(
				    StopTimer,
				    FTimerDelegate::CreateWeakLambda(this,
				                                     [WeakEffect]()
				                                     {
					                                     if (UNiagaraComponent* ActiveEffect = WeakEffect.Get())
					                                     {
						                                     ActiveEffect->DeactivateImmediate();
						                                     ActiveEffect->DestroyComponent();
					                                     }
				                                     }),
				    ReEchoCombatVfx::BoundedElementReactionLifetimeSeconds,
				    false);
			}
			return Effect;
		}
	}
	return nullptr;
}

bool UReEchoCombatVfxComponent::SpawnConductLink(const FReEchoElementReactionLink& Link) const
{
	AActor* SourceTarget = Link.SourceTarget;
	AActor* TargetTarget = Link.TargetTarget;
	const IReEchoCombatTarget* SourceCombatTarget = SourceTarget ? Cast<IReEchoCombatTarget>(SourceTarget) : nullptr;
	const IReEchoCombatTarget* TargetCombatTarget = TargetTarget ? Cast<IReEchoCombatTarget>(TargetTarget) : nullptr;
	if (!SourceCombatTarget || !TargetCombatTarget || !SourceCombatTarget->IsCombatTargetAlive() ||
	    !TargetCombatTarget->IsCombatTargetAlive() || !GetWorld())
	{
		return false;
	}
	UNiagaraSystem* System =
	    ResolveElementSystem(static_cast<uint8>(EReEchoElementReactionVfxSemantic::Conduct), TargetTarget);
	if (!System)
	{
		return false;
	}
	FVector Start = FVector::ZeroVector;
	FVector End = FVector::ZeroVector;
	if (!TryResolveConductLinkAnchors(SourceTarget, TargetTarget, Start, End))
	{
		UE_LOG(LogReEcho,
		       VeryVerbose,
		       TEXT("Conduct suppressed because an explicit HurtVfxRoot is missing: Source=%s Target=%s"),
		       *GetNameSafe(SourceTarget),
		       *GetNameSafe(TargetTarget));
		return false;
	}
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
		return false;
	}
	Effect->SetVariablePosition(TEXT("User.StartPosition"), StartParameter);
	Effect->SetVariablePosition(TEXT("User.EndPosition"), EndParameter);
	if (ReEchoCombatVfx::CVarReEchoDebugConductVfx.GetValueOnGameThread() != 0)
	{
		const UReEchoCombatVfxComponent* SourceVfx = SourceTarget->FindComponentByClass<UReEchoCombatVfxComponent>();
		const UReEchoCombatVfxComponent* TargetVfx = TargetTarget->FindComponentByClass<UReEchoCombatVfxComponent>();
		const USceneComponent* SourceAnchor = SourceVfx ? SourceVfx->HurtVfxRoot.Get() : nullptr;
		const USceneComponent* TargetAnchor = TargetVfx ? TargetVfx->HurtVfxRoot.Get() : nullptr;
		constexpr float DebugSeconds = 5.0f;
		DrawDebugSphere(GetWorld(), StartParameter, 18.0f, 12, FColor::Green, false, DebugSeconds, 0, 3.0f);
		DrawDebugSphere(GetWorld(), End, 18.0f, 12, FColor::Red, false, DebugSeconds, 0, 3.0f);
		DrawDebugLine(GetWorld(), Start, End, FColor::Blue, false, DebugSeconds, 0, 3.0f);
		DrawDebugString(GetWorld(), StartParameter, TEXT("Conduct Source"), nullptr, FColor::Green, DebugSeconds);
		DrawDebugString(GetWorld(), End, TEXT("Conduct Target"), nullptr, FColor::Red, DebugSeconds);
		UE_LOG(LogReEcho,
		       Warning,
		       TEXT("[ConductSpaceDebug] Source=%s ActorWorld=%s Anchor=%s AnchorRelative=%s AnchorWorld=%s "
		            "Target=%s ActorWorld=%s Anchor=%s AnchorRelative=%s AnchorWorld=%s "
		            "UserStartAbsolute=%s UserEndRelative=%s ExpectedTargetWorld=%s Distance=%.2f NiagaraWorld=%s "
		            "UserStartInNiagaraLocal=%s UserEndInNiagaraLocal=%s"),
		       *GetNameSafe(SourceTarget),
		       *SourceTarget->GetActorLocation().ToCompactString(),
		       *GetNameSafe(SourceAnchor),
		       SourceAnchor ? *SourceAnchor->GetRelativeLocation().ToCompactString() : TEXT("<none>"),
		       SourceAnchor ? *SourceAnchor->GetComponentLocation().ToCompactString() : TEXT("<none>"),
		       *GetNameSafe(TargetTarget),
		       *TargetTarget->GetActorLocation().ToCompactString(),
		       *GetNameSafe(TargetAnchor),
		       TargetAnchor ? *TargetAnchor->GetRelativeLocation().ToCompactString() : TEXT("<none>"),
		       TargetAnchor ? *TargetAnchor->GetComponentLocation().ToCompactString() : TEXT("<none>"),
		       *StartParameter.ToCompactString(),
		       *EndParameter.ToCompactString(),
		       *End.ToCompactString(),
		       FVector::Distance(Start, End),
		       *Effect->GetComponentLocation().ToCompactString(),
		       *Effect->GetComponentTransform().InverseTransformPosition(StartParameter).ToCompactString(),
		       *Effect->GetComponentTransform().InverseTransformPosition(EndParameter).ToCompactString());
	}
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
	return true;
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
	if (!FReEchoCombatVfxCatalog::ResolveAttackCommittedSemantic(Event.AttackPatternId, Semantic))
	{
		return;
	}
	const EReEchoCombatVfxSemantic DefaultSemantic = Semantic;
	if (FReEchoCombatVfxCatalog::IsLongSwordSlashSemantic(Semantic))
	{
		FReEchoCombatVfxCatalog::ResolveLongSwordSlashSemantic(Event.Element, Semantic);
	}
	else if (FReEchoCombatVfxCatalog::IsScytheSlashSemantic(Semantic))
	{
		FReEchoCombatVfxCatalog::ResolveScytheSlashSemantic(Event.Element, Semantic);
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
		    [WeakThis,
		     Semantic,
		     DefaultSemantic,
		     LockedDirection = Event.Direction,
		     LockedRangeMultiplier = Event.RangeMultiplierFromBase]()
		    {
			    if (const UReEchoCombatVfxComponent* Component = WeakThis.Get())
			    {
				    if (!Component->SpawnAttached(static_cast<uint8>(Semantic),
				                                  LockedDirection,
				                                  Component->ResolveWeaponAttackVfxRoot(),
				                                  true,
				                                  LockedRangeMultiplier) &&
				        Semantic != DefaultSemantic)
				    {
					    Component->SpawnAttached(static_cast<uint8>(DefaultSemantic),
					                             LockedDirection,
					                             Component->ResolveWeaponAttackVfxRoot(),
					                             true,
					                             LockedRangeMultiplier);
				    }
			    }
		    },
		    DelaySeconds,
		    false);
		return;
	}
	if (!SpawnAttached(static_cast<uint8>(Semantic),
	                   Event.Direction,
	                   ResolveWeaponAttackVfxRoot(),
	                   true,
	                   Event.RangeMultiplierFromBase) &&
	    Semantic != DefaultSemantic)
	{
		SpawnAttached(static_cast<uint8>(DefaultSemantic),
		              Event.Direction,
		              ResolveWeaponAttackVfxRoot(),
		              true,
		              Event.RangeMultiplierFromBase);
	}
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
	if (!ShouldPlayTargetHurtEffect(Event, GetOwner()))
	{
		return;
	}
	EReEchoCombatVfxSemantic Semantic = Cast<AReEchoEnemyActor>(GetOwner()) ? EReEchoCombatVfxSemantic::EnemyHurt
	                                                                        : EReEchoCombatVfxSemantic::PlayerHurt;
	const AReEchoEnemyActor* TargetEnemy = Cast<AReEchoEnemyActor>(GetOwner());
	const bool bTargetIsBoss = TargetEnemy && TargetEnemy->GetPresentationId() == TEXT("Enemy.TimeGuard");
	FVector BossHurtLocation = Event.WorldLocation;
	if (bTargetIsBoss)
	{
		FVector FlipbookCenterWorld = FVector::ZeroVector;
		if (const USceneComponent* HurtRoot = ResolveHurtVfxRoot();
		    HurtRoot && TryResolveFlipbookCenter(GetOwner(), FlipbookCenterWorld))
		{
			BossHurtLocation = ResolveBossHurtEffectLocation(HurtRoot->GetComponentLocation(), FlipbookCenterWorld);
		}
	}
	if (TargetEnemy && (Event.bFatal || Event.DamageSource == EReEchoDamageSource::Path))
	{
		// Fatal enemy hits cannot remain attached to presentation that death tears down. Connection-line damage
		// has no weapon impact semantic either, so both paths use a world instance that survives target cleanup.
		const FVector ImpactLocation = bTargetIsBoss ? BossHurtLocation : Event.WorldLocation;
		const FVector ImpactDirection = ImpactLocation - Event.SourceWorldLocation;
		SpawnWorld(static_cast<uint8>(EReEchoCombatVfxSemantic::EnemyHurt),
		           ImpactLocation,
		           ImpactDirection,
		           true,
		           true,
		           true);
		return;
	}
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
	if (bTargetIsBoss)
	{
		SpawnWorld(static_cast<uint8>(Semantic), BossHurtLocation, FVector::ForwardVector, true, true, true);
		return;
	}
	SpawnAttached(static_cast<uint8>(Semantic), FVector::ForwardVector, ResolveHurtVfxRoot(), true, 1.0f, true);
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
		// Burn is driven by the authoritative timed reaction status.
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
	else if (Event.ReactionId == TEXT("Y_ER_L_G"))
	{
		Semantic = EReEchoElementReactionVfxSemantic::Growth;
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
			AActor* Owner = GetOwner();
			DirectionEffect = SpawnAttached(static_cast<uint8>(EReEchoCombatVfxSemantic::FoxDirection),
			                                Event.LockedDirection,
			                                Owner ? Owner->GetRootComponent() : nullptr,
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

void UReEchoCombatVfxComponent::BeginBossTransformationEffects(const float Scale)
{
	EndBossTransformationEffects();
	StopBossActionEffects();
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}
	FReEchoBossIntent GroundIntent;
	GroundIntent.Target = Owner;
	GroundIntent.LockedTargetLocation = Owner->GetActorLocation();
	const FVector Ground = ResolveBossTargetGroundLocation(GroundIntent);
	BossTransformationGround = SpawnWorld(
	    static_cast<uint8>(EReEchoCombatVfxSemantic::GoatSkill03Alarming), Ground, FVector::ForwardVector, false);
	FVector Top = Owner->GetActorLocation();
	if (const UReEcho2DAnimationComponent* Animation = Owner->FindComponentByClass<UReEcho2DAnimationComponent>())
	{
		Top = ResolveBossPhase3ChargingTopWorldLocation(Animation->CalcBounds(Animation->GetComponentTransform()));
	}
	BossTransformationCharge = SpawnWorld(
	    static_cast<uint8>(EReEchoCombatVfxSemantic::GoatSkill04Charging), Top, FVector::ForwardVector, false);
	for (UNiagaraComponent* Effect : {BossTransformationGround.Get(), BossTransformationCharge.Get()})
	{
		if (Effect)
		{
			Effect->SetWorldScale3D(Effect->GetComponentScale() * FMath::Max(0.1f, Scale));
		}
	}
}

void UReEchoCombatVfxComponent::BurstBossTransformationEffects(const float Scale)
{
	StopEffect(BossTransformationCharge);
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}
	FReEchoBossIntent Intent;
	Intent.Target = Owner;
	Intent.LockedTargetLocation = Owner->GetActorLocation();
	const FVector Ground = ResolveBossTargetGroundLocation(Intent);
	BossTransformationBurst = SpawnWorld(
	    static_cast<uint8>(EReEchoCombatVfxSemantic::GoatSkill03Impact), Ground, FVector::ForwardVector, false, false);
	if (BossTransformationBurst)
	{
		BossTransformationBurst->SetWorldScale3D(BossTransformationBurst->GetComponentScale() *
		                                         FMath::Max(0.1f, Scale));
		const UNiagaraSystem* System = BossTransformationBurst->GetAsset();
		BossTransformationBurst->SetWorldLocation(ResolveGroundAlignedEffectOrigin(
		    Ground, System ? System->GetFixedBounds() : FBox(ForceInit), BossTransformationBurst->GetComponentScale()));
		BossTransformationBurst->Activate(true);
	}
}

void UReEchoCombatVfxComponent::EndBossTransformationEffects()
{
	EndSacrificeEffect();
	StopEffect(BossTransformationCharge);
	StopEffect(BossTransformationGround);
	StopEffect(BossTransformationBurst);
}

void UReEchoCombatVfxComponent::BeginSacrificeEffect(const FVector& WorldLocation)
{
	EndSacrificeEffect();
	SacrificeEffect = SpawnWorld(static_cast<uint8>(EReEchoCombatVfxSemantic::GoatSkill03Charging),
	                             WorldLocation,
	                             FVector::ForwardVector,
	                             false);
	if (SacrificeEffect)
	{
		const UReEcho2DAnimationComponent* Animation = GetOwner()->FindComponentByClass<UReEcho2DAnimationComponent>();
		const float BodyHeight = Animation ? Animation->Bounds.BoxExtent.Z * 2.0f : 180.0f;
		SacrificeEffect->SetWorldScale3D(SacrificeEffect->GetComponentScale() *
		                                 FMath::Clamp(BodyHeight / 180.0f, 0.4f, 1.5f));
	}
}

void UReEchoCombatVfxComponent::UpdateSacrificeEffect(const FVector& WorldLocation)
{
	if (SacrificeEffect)
	{
		SacrificeEffect->SetWorldLocation(WorldLocation);
		if (SacrificeEffect->IsComplete())
		{
			SacrificeEffect->Activate(true);
		}
	}
}

void UReEchoCombatVfxComponent::EndSacrificeEffect()
{
	StopEffect(SacrificeEffect);
}

FString UReEchoCombatVfxComponent::MakeBossSkill03ImpactKey(const FReEchoBossIntent& Intent)
{
	return FString::Printf(TEXT("%lld:%d"), Intent.Attack.Sequence, Intent.ComboStrikeIndex);
}

void UReEchoCombatVfxComponent::SpawnBossSkill03Impact(const FReEchoBossIntent& Intent)
{
#if WITH_DEV_AUTOMATION_TESTS
	++BossSkill03ImpactIntentCountForTests;
#endif
	const FVector* LockedGroundLocation = BossGroundLocationByAttackSequence.Find(Intent.Attack.Sequence);
	const FVector ImpactLocation =
	    LockedGroundLocation ? *LockedGroundLocation : ResolveBossTargetGroundLocation(Intent);
	const EReEchoCombatVfxSemantic ImpactSemantic = EReEchoCombatVfxSemantic::GoatSkill03Impact;
	LastBossSkill03ImpactEffect =
	    SpawnWorld(static_cast<uint8>(ImpactSemantic), ImpactLocation, Intent.LockedDirection, false, false);
	if (UNiagaraComponent* ImpactEffect = LastBossSkill03ImpactEffect.Get())
	{
		const UNiagaraSystem* ImpactSystem = ImpactEffect->GetAsset();
		const FBox AuthoredBounds = ImpactSystem ? ImpactSystem->GetFixedBounds() : FBox(EForceInit::ForceInit);
		ImpactEffect->SetWorldLocation(
		    ResolveGroundAlignedEffectOrigin(ImpactLocation, AuthoredBounds, ImpactEffect->GetComponentScale()));
		ImpactEffect->Activate(true);
		if (UWorld* World = GetWorld())
		{
			const TWeakObjectPtr<UNiagaraComponent> WeakImpactEffect(ImpactEffect);
			FTimerHandle LifetimeTimer;
			World->GetTimerManager().SetTimer(
			    LifetimeTimer,
			    [WeakImpactEffect]()
			    {
				    if (UNiagaraComponent* ActiveEffect = WeakImpactEffect.Get())
				    {
					    ActiveEffect->Deactivate();
					    ActiveEffect->DestroyComponent();
				    }
			    },
			    FMath::Max(FReEchoCombatVfxCatalog::ResolvePlacement(ImpactSemantic).PlaybackDurationSeconds, 0.01f),
			    false);
		}
	}
#if WITH_DEV_AUTOMATION_TESTS
	BossSkill03ImpactSpawnCountForTests += LastBossSkill03ImpactEffect.IsValid() ? 1 : 0;
#endif
}

void UReEchoCombatVfxComponent::HandleBossIntent(const FReEchoBossIntent& Intent)
{
	if (Intent.Type == EReEchoBossIntentType::EncounterPhase)
	{
		CurrentBossPhaseIndex = FMath::Max(1, Intent.PhaseDefinition.PhaseIndex);
		EarlyBossSkill03ImpactKeys.Reset();
		StopBossActionEffects();
		return;
	}
	const bool bSkill01 = Intent.AbilityId == TEXT("M_SHEEP_MeleeSweep");
	const bool bSkill02 =
	    Intent.AbilityId == TEXT("M_SHEEP_StationaryVolley") || Intent.AbilityId == TEXT("M_SHEEP_MovingSpread");
	const bool bSkill03 = Intent.AbilityKind == EReEchoBossAbilityKind::BlinkSlam ||
	                      Intent.AbilityKind == EReEchoBossAbilityKind::BlinkSlamMoving;
	const bool bSkill04 = Intent.AbilityId == TEXT("M_SHEEP_PrayerBeam");
	if (!bSkill01 && !bSkill02 && !bSkill03 && !bSkill04)
	{
		return;
	}
	RememberBossAbility(Intent.Attack.Sequence, Intent.AbilityId);
	if (bSkill03 && Intent.Type == EReEchoBossIntentType::ImpactResolved)
	{
		if (CurrentBossPhaseIndex >= 3 && EarlyBossSkill03ImpactKeys.Remove(MakeBossSkill03ImpactKey(Intent)) > 0)
		{
			return;
		}
		SpawnBossSkill03Impact(Intent);
		return;
	}
	if (Intent.Type == EReEchoBossIntentType::TelegraphStarted)
	{
		StopBossActionEffects();
		if (bSkill03 && (Intent.bPhaseOpening || Intent.bGroundedSlam))
		{
			// Grounded opening starts the attack animation directly, without charging or a target telegraph.
			BossGroundLocationByAttackSequence.Add(Intent.Attack.Sequence, ResolveBossTargetGroundLocation(Intent));
			return;
		}
		if (bSkill01)
		{
			return;
		}
		const EReEchoCombatVfxSemantic ChargingSemantic = bSkill02   ? EReEchoCombatVfxSemantic::GoatSkill02Charging
		                                                  : bSkill03 ? EReEchoCombatVfxSemantic::GoatSkill03Charging
		                                                             : EReEchoCombatVfxSemantic::GoatSkill04Charging;
		USceneComponent* ChargingRoot = ResolveBossChargingAttachmentRoot(
		    bSkill03, CurrentBossPhaseIndex, ResolveAttackVfxRoot(), ResolveBossWeaponVfxRoot());
		BossChargingEffect =
		    SpawnAttached(static_cast<uint8>(ChargingSemantic), Intent.LockedDirection, ChargingRoot, false);
		if (BossChargingEffect && bSkill03 && CurrentBossPhaseIndex >= 3)
		{
			AActor* Owner = GetOwner();
			UReEcho2DAnimationComponent* Animation =
			    Owner ? Owner->FindComponentByClass<UReEcho2DAnimationComponent>() : nullptr;
			if (Animation && Animation->GetFlipbook())
			{
				const FBoxSphereBounds AnimationWorldBounds = Animation->CalcBounds(Animation->GetComponentTransform());
				BossChargingEffect->SetWorldLocation(ResolveBossPhase3ChargingTopWorldLocation(AnimationWorldBounds));
			}
		}
		if (bSkill03 || bSkill04)
		{
			const FVector GroundEffectLocation = ResolveBossTargetGroundLocation(Intent);
			BossGroundLocationByAttackSequence.Add(Intent.Attack.Sequence, GroundEffectLocation);
			BossTelegraphEffect = SpawnWorld(static_cast<uint8>(EReEchoCombatVfxSemantic::GoatSkill03Alarming),
			                                 GroundEffectLocation,
			                                 Intent.LockedDirection,
			                                 false);
		}
		return;
	}
	if (Intent.Type == EReEchoBossIntentType::AttackWindowStarted)
	{
		StopEffect(BossChargingEffect);
		StopEffect(BossTelegraphEffect);
		if (bSkill03 && CurrentBossPhaseIndex >= 3 && !Intent.bPhaseOpening && !Intent.bGroundedSlam)
		{
			SpawnBossSkill03Impact(Intent);
			EarlyBossSkill03ImpactKeys.Add(MakeBossSkill03ImpactKey(Intent));
		}
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
			const FVector* LockedGroundLocation = BossGroundLocationByAttackSequence.Find(Intent.Attack.Sequence);
			const FVector GroundLocation =
			    LockedGroundLocation ? *LockedGroundLocation : ResolveBossTargetGroundLocation(Intent);
			BossTelegraphEffect = SpawnWorld(static_cast<uint8>(EReEchoCombatVfxSemantic::GoatSkill03Alarming),
			                                 GroundLocation,
			                                 Intent.LockedDirection,
			                                 false);
			StopEffect(BossActiveEffect);
			BossActiveEffect = SpawnBossBeam(Intent, GroundLocation);
			BossActiveEffectRemainingSeconds = BossActiveEffect ? FMath::Max(0.0f, Intent.ActiveSeconds) : 0.0f;
		}
		return;
	}
	if (Intent.Type == EReEchoBossIntentType::AbilityEnded)
	{
		EarlyBossSkill03ImpactKeys.Remove(MakeBossSkill03ImpactKey(Intent));
		StopBossActionEffects();
		BossGroundLocationByAttackSequence.Remove(Intent.Attack.Sequence);
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
