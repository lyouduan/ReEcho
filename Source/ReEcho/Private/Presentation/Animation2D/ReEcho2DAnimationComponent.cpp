#include "Presentation/Animation2D/ReEcho2DAnimationComponent.h"

#include "PaperFlipbook.h"
#include "PaperSprite.h"
#include "DrawDebugHelpers.h"
#include "PhysicsEngine/BodySetup.h"
#include "Presentation/Animation2D/ReEcho2DCollisionDebug.h"

EReEcho2DAnimationState ReEchoResolve2DAnimationState(const bool bMoving, const bool bAttacking)
{
	if (bAttacking)
	{
		return EReEcho2DAnimationState::Attack;
	}
	return bMoving ? EReEcho2DAnimationState::Walk : EReEcho2DAnimationState::Idle;
}

UPaperFlipbook* FReEcho2DAnimationProfile::Resolve(const EReEcho2DAnimationState State) const
{
	if (const TObjectPtr<UPaperFlipbook>* Match = StateFlipbooks.Find(State); Match && Match->Get())
	{
		return Match->Get();
	}
	return DefaultFlipbook;
}

UReEcho2DAnimationComponent::UReEcho2DAnimationComponent()
{
	// UPaperFlipbookComponent advances playback from its component tick.
	PrimaryComponentTick.bCanEverTick = true;
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetCollisionObjectType(ECC_WorldDynamic);
	SetCollisionResponseToAllChannels(ECR_Ignore);
	SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	// Per-frame BodySetup geometry is query data, not an event source. Automatic overlap updates
	// would be rebuilt on every key frame and no gameplay system consumes those callbacks.
	SetGenerateOverlapEvents(false);
	SetCastShadow(false);
	SetLooping(true);
	SetVisibility(false);
	SetHiddenInGame(true);
	SetAbsolute(false, true, false);
	SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
}

bool UReEcho2DAnimationComponent::PlayClip(const FReEcho2DAnimationClip& Clip, const bool bRestart)
{
	if (!Clip.IsValid())
	{
		return false;
	}

	const bool bClipChanged = GetFlipbook() != Clip.Flipbook;
	const bool bPolicyChanged = IsLooping() != Clip.bLooping || !FMath::IsNearlyEqual(GetPlayRate(), Clip.PlayRate);
	ActiveClip = Clip;
	bAnimationActive = true;
	SetFlipbook(Clip.Flipbook);
	SetLooping(Clip.bLooping);
	SetPlayRate(Clip.PlayRate);
	SetRelativeLocation(Clip.LocalOffset);
	SetTranslucentSortPriority(Clip.TranslucentSortPriority);
	SetHiddenInGame(false);
	SetVisibility(true);
	SetComponentTickEnabled(true);
	ApplyDisplayScale();
	ApplyCollisionPolicy();
	if (bRestart || Clip.bRestartOnRequest || bClipChanged || bPolicyChanged || !IsPlaying())
	{
		PlayFromStart();
	}
	return true;
}

EReEcho2DAnimationActivationResult
UReEcho2DAnimationComponent::ActivateProfile(const FReEcho2DAnimationProfile& InProfile)
{
	DeactivateAnimation();
	if (!InProfile.bUseNativeScale && InProfile.WorldHeight <= 0.0f)
	{
		return EReEcho2DAnimationActivationResult::MissingProfile;
	}
	if (!InProfile.DefaultFlipbook)
	{
		return EReEcho2DAnimationActivationResult::MissingFlipbook;
	}

	ActiveProfile = InProfile;
	ActiveState = EReEcho2DAnimationState::Default;
	bAnimationActive = true;
	FReEcho2DAnimationClip Clip;
	Clip.Flipbook = ActiveProfile.DefaultFlipbook;
	Clip.bLooping = true;
	Clip.bUseNativeScale = ActiveProfile.bUseNativeScale;
	Clip.WorldHeight = ActiveProfile.WorldHeight;
	Clip.LocalOffset = ActiveProfile.LocalOffset;
	Clip.TranslucentSortPriority = ActiveProfile.TranslucentSortPriority;
	PlayClip(Clip, true);
	return EReEcho2DAnimationActivationResult::Activated;
}

bool UReEcho2DAnimationComponent::SetAnimationState(const EReEcho2DAnimationState NewState,
                                                    const bool bShouldLoop,
                                                    const bool bRestart)
{
	if (!bAnimationActive)
	{
		return false;
	}
	UPaperFlipbook* ResolvedFlipbook = ActiveProfile.Resolve(NewState);
	if (!bRestart && ActiveState == NewState && GetFlipbook() == ResolvedFlipbook && IsLooping() == bShouldLoop &&
	    IsPlaying())
	{
		return true;
	}
	ActiveState = NewState;
	FReEcho2DAnimationClip Clip = ActiveClip;
	Clip.Flipbook = ResolvedFlipbook;
	Clip.bLooping = bShouldLoop;
	Clip.bRestartOnRequest = bRestart;
	PlayClip(Clip, bRestart);
	return GetFlipbook() != nullptr;
}

void UReEcho2DAnimationComponent::DeactivateAnimation()
{
	Stop();
	SetFlipbook(nullptr);
	SetVisibility(false);
	SetHiddenInGame(true);
	SetComponentTickEnabled(false);
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	bAnimationActive = false;
	ActiveClip = FReEcho2DAnimationClip();
	ActiveState = EReEcho2DAnimationState::Default;
}

void UReEcho2DAnimationComponent::SetFacingSign(const float InFacingSign)
{
	FacingSign = InFacingSign < 0.0f ? -1.0f : 1.0f;
	ApplyDisplayScale();
}

bool UReEcho2DAnimationComponent::IsAnimationActive()
{
	return bAnimationActive && GetFlipbook() != nullptr;
}

void UReEcho2DAnimationComponent::TickComponent(const float DeltaTime, const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	DrawCurrentFrameCollisionDebug();
}

int32 UReEcho2DAnimationComponent::GetCurrentKeyFrameIndex()
{
	const UPaperFlipbook* Flipbook = GetFlipbook();
	if (!bAnimationActive || !Flipbook || Flipbook->GetNumFrames() <= 0)
	{
		return INDEX_NONE;
	}
	return FMath::Clamp(Flipbook->GetKeyFrameIndexAtTime(GetPlaybackPosition(), true), 0, Flipbook->GetNumFrames() - 1);
}

bool UReEcho2DAnimationComponent::IsUsingEachFrameCollision() const
{
	return ActiveClip.Flipbook &&
	       ActiveClip.Flipbook->GetCollisionSource() == EFlipbookCollisionMode::EachFrameCollision;
}

bool UReEcho2DAnimationComponent::RebuildSpriteAsset(UPaperSprite* Sprite)
{
#if WITH_EDITOR
	if (!Sprite)
	{
		return false;
	}
	Sprite->Modify();
	Sprite->RebuildData();
	Sprite->MarkPackageDirty();
	return Sprite->GetRenderBounds().BoxExtent.Z > 0.0f;
#else
	return false;
#endif
}

void UReEcho2DAnimationComponent::ApplyDisplayScale()
{
	const UPaperFlipbook* Flipbook = GetFlipbook();
	const float NativeWorldHeight = Flipbook ? Flipbook->GetRenderBounds().BoxExtent.Z * 2.0f : 0.0f;
	const float UniformScale =
	    ActiveClip.bUseNativeScale || NativeWorldHeight <= 0.0f ? 1.0f : ActiveClip.WorldHeight / NativeWorldHeight;
	SetRelativeScale3D(FVector(UniformScale * FacingSign, UniformScale, UniformScale));
}

void UReEcho2DAnimationComponent::ApplyCollisionPolicy()
{
	// Paper2D recreates its physics state on every key-frame change. Keep it Query-only so the
	// Actor root Capsule remains the sole movement/blocking authority.
	SetCollisionEnabled(IsUsingEachFrameCollision() ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
}

void UReEcho2DAnimationComponent::DrawCurrentFrameCollisionDebug()
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (ReEcho2DCollisionDebug::GetLevel() < 2 || !IsUsingEachFrameCollision() || !GetWorld())
	{
		return;
	}
	UBodySetup* BodySetup = GetBodySetup();
	if (!BodySetup)
	{
		DrawDebugString(GetWorld(), GetComponentLocation(), TEXT("EachFrameCollision: no BodySetup"),
		                nullptr, FColor::Orange, 0.0f, true, 0.9f);
		return;
	}
	const FTransform ComponentTransform = GetComponentTransform();
	const FVector ComponentScale = ComponentTransform.GetScale3D().GetAbs();
	const FColor ShapeColor = FColor::Yellow;
	for (const FKBoxElem& Elem : BodySetup->AggGeom.BoxElems)
	{
		const FTransform WorldTransform = Elem.GetTransform() * ComponentTransform;
		DrawDebugBox(GetWorld(), WorldTransform.GetLocation(),
		             FVector(Elem.X, Elem.Y, Elem.Z) * ComponentScale * 0.5f,
		             WorldTransform.GetRotation(), ShapeColor, false, 0.0f, 1, 2.0f);
	}
	for (const FKSphereElem& Elem : BodySetup->AggGeom.SphereElems)
	{
		const FVector WorldCenter = ComponentTransform.TransformPosition(Elem.Center);
		DrawDebugSphere(GetWorld(), WorldCenter, Elem.Radius * ComponentScale.GetMax(),
		                16, ShapeColor, false, 0.0f, 1, 2.0f);
	}
	for (const FKSphylElem& Elem : BodySetup->AggGeom.SphylElems)
	{
		const FTransform WorldTransform = Elem.GetTransform() * ComponentTransform;
		DrawDebugCapsule(GetWorld(), WorldTransform.GetLocation(), Elem.GetScaledHalfLength(ComponentScale),
		                 Elem.GetScaledRadius(ComponentScale), WorldTransform.GetRotation(),
		                 ShapeColor, false, 0.0f, 1, 2.0f);
	}
	for (const FKConvexElem& Elem : BodySetup->AggGeom.ConvexElems)
	{
		const FTransform WorldTransform = Elem.GetTransform() * ComponentTransform;
		for (int32 Triangle = 0; Triangle + 2 < Elem.IndexData.Num(); Triangle += 3)
		{
			const int32 Indices[3] = {Elem.IndexData[Triangle], Elem.IndexData[Triangle + 1], Elem.IndexData[Triangle + 2]};
			for (int32 Edge = 0; Edge < 3; ++Edge)
			{
				if (Elem.VertexData.IsValidIndex(Indices[Edge]) && Elem.VertexData.IsValidIndex(Indices[(Edge + 1) % 3]))
				{
					DrawDebugLine(GetWorld(), WorldTransform.TransformPosition(Elem.VertexData[Indices[Edge]]),
					              WorldTransform.TransformPosition(Elem.VertexData[Indices[(Edge + 1) % 3]]),
					              ShapeColor, false, 0.0f, 1, 2.0f);
				}
			}
		}
	}
	const FBox DebugBounds = BodySetup->AggGeom.CalcAABB(ComponentTransform);
	const FString Label = FString::Printf(TEXT("Paper2D frame=%d boxes=%d spheres=%d capsules=%d convex=%d"),
	                                      GetCurrentKeyFrameIndex(),
	                                      BodySetup->AggGeom.BoxElems.Num(),
	                                      BodySetup->AggGeom.SphereElems.Num(),
	                                      BodySetup->AggGeom.SphylElems.Num(),
	                                      BodySetup->AggGeom.ConvexElems.Num());
	DrawDebugString(GetWorld(), DebugBounds.IsValid ? DebugBounds.Max : GetComponentLocation(), Label,
	                nullptr, FColor::Yellow, 0.0f, true, 0.75f);
#endif
}
