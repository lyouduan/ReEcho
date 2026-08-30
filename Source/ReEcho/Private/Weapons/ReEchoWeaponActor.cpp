#include "Weapons/ReEchoWeaponActor.h"

#include "ReEcho.h"
#include "ReEchoGameMode.h"
#include "Camera/PlayerCameraManager.h"

#include "Combat/ReEchoCombatantComponent.h"
#include "Combat/ReEchoAttackControllerComponent.h"
#include "Combat/ReEchoCombatContracts.h"
#include "Combat/ReEchoCombatTarget.h"
#include "Combat/ReEchoElementReaction.h"
#include "Combat/ReEchoHitResolver.h"
#include "Diagnostics/ReEchoBuildTrace.h"
#include "Components/BillboardComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Math/RotationMatrix.h"
#include "Graybox/ReEchoStaffLightWaveActor.h"
#include "Graybox/ReEchoEchoActor.h"
#include "Graybox/ReEchoEnemyActor.h"
#include "Graybox/ReEchoProjectileActor.h"
#include "Graybox/ReEchoTimeShardPickupActor.h"
#include "Kismet/GameplayStatics.h"
#include "Player/ReEchoPlayerPawn.h"
#include "Presentation/Animation2D/ReEcho2DCharacterPresentationProfile.h"
#include "Presentation/Weapon/ReEchoWeaponPresentationCatalog.h"
#include "Presentation/Weapon/ReEchoWeaponPresentationProfile.h"
#include "Weapons/ReEchoWeaponGeometry.h"
#include "Weapons/ReEchoWeaponVisualCatalog.h"

namespace ReEchoWeaponVisual
{
const FVector StaffLocation(-16.0f, 30.0f, 6.0f);
const FVector SwordLocation(8.0f, 0.0f, 0.0f);
constexpr float SwordRestAngleRadians = -PI / 4.0f;
constexpr float TripleSwingAmplitudeRadians = PI / 3.0f;
constexpr float TripleSwingHalfCycles = 3.0f;
const FVector CameraFacingNormal(-0.573576f, 0.0f, 0.819152f);
const FVector DefaultWeaponAnchorRatio(-0.16f, 0.30f, 0.06f);
const FVector DefaultLeftWeaponAnchorRatio(-0.16f, -0.30f, 0.06f);
constexpr float WeaponPlaneSizeCm = 100.0f;

FVector2D ResolveDefaultAttackVfxAnchorRatio(const FName WeaponVisualKey)
{
	if (WeaponVisualKey == TEXT("CrescentBlade"))
	{
		return FVector2D(0.0f, -0.5f);
	}
	if (WeaponVisualKey == TEXT("Bow") || WeaponVisualKey == TEXT("Gun"))
	{
		return FVector2D(0.5f, 0.0f);
	}
	return FVector2D::ZeroVector;
}

FVector2D ResolveAttackVfxAnchorRatio(const UReEchoWeaponPresentationProfile& WeaponProfile, const float FacingSign)
{
	FVector2D AnchorRatio = WeaponProfile.bOverrideAttackVfxAnchor
	                            ? WeaponProfile.AttackVfxAnchorRatio
	                            : ResolveDefaultAttackVfxAnchorRatio(WeaponProfile.WeaponVisualKey);
	if (FacingSign < 0.0f)
	{
		AnchorRatio.X *= -1.0f;
	}
	return AnchorRatio;
}

FVector2D ResolveAttackVfxAnchorComponentRatio(const UReEchoWeaponPresentationProfile& WeaponProfile,
                                               const float FacingSign,
                                               const bool bVisualHorizontallyMirrored)
{
	FVector2D AnchorRatio = ResolveAttackVfxAnchorRatio(WeaponProfile, FacingSign);
	if (bVisualHorizontallyMirrored)
	{
		// The desired anchor is expressed in final visual space. A negative parent X scale would otherwise mirror it
		// a second time, putting a right-facing gun muzzle on the character-facing side of the texture.
		AnchorRatio.X *= -1.0f;
	}
	return AnchorRatio;
}

FVector ResolveGunMuzzleLocalPoint(const FBox& LocalBounds,
                                   const float FacingSign,
                                   const bool bVisualHorizontallyMirrored,
                                   const float VerticalRatio)
{
	if (!LocalBounds.IsValid)
	{
		return FVector::ZeroVector;
	}
	float LocalDirectionSign = FacingSign < 0.0f ? -1.0f : 1.0f;
	if (bVisualHorizontallyMirrored)
	{
		// The parent plane's negative X scale reverses its local barrel axis in final visual space.
		LocalDirectionSign *= -1.0f;
	}
	const FVector Center = LocalBounds.GetCenter();
	const FVector Extent = LocalBounds.GetExtent();
	return FVector(
	    Center.X + LocalDirectionSign * Extent.X, Center.Y + VerticalRatio * LocalBounds.GetSize().Y, Center.Z);
}

FVector ResolveProjectileSpawnLocation(const FVector& WeaponAnchorLocation,
                                       const FVector& LegacyOwnerLocation,
                                       const FVector& Direction,
                                       const bool bHasWeaponAnchor)
{
	return bHasWeaponAnchor ? WeaponAnchorLocation
	                        : LegacyOwnerLocation + FVector(0.0f, 0.0f, 35.0f) + Direction * 45.0f;
}

EReEchoElement ResolveProjectileElement(const bool bUsesDeterministicRandomElement,
                                        const EReEchoElement AttackElement,
                                        const int64 AttackSequence,
                                        const int32 ProjectileIndex,
                                        const EReEchoElement DebugOverride)
{
	const EReEchoElement ResolvedElement = ReEchoWeaponRuntime::ResolveProjectileElement(
	    bUsesDeterministicRandomElement, AttackElement, AttackSequence, ProjectileIndex);
#if !UE_BUILD_SHIPPING
	return ReEchoElementReaction::IsCombatElement(DebugOverride) ? DebugOverride : ResolvedElement;
#else
	return ResolvedElement;
#endif
}

FQuat GetSwordRotation(const float SpinRadians = 0.0f)
{
	const FQuat CameraFacingRotation = FRotationMatrix::MakeFromZX(CameraFacingNormal, FVector::RightVector).ToQuat();
	return FQuat(CameraFacingNormal, SpinRadians) * CameraFacingRotation;
}

float ResolveHeldLength(const UReEchoWeaponPresentationProfile& WeaponProfile, const float CharacterWorldHeight)
{
	return WeaponProfile.bOverrideHeldLength ? FMath::Max(WeaponProfile.HeldLengthOverrideCm, 1.0f)
	                                         : CharacterWorldHeight * FMath::Max(WeaponProfile.HeldLengthRatio, 0.01f);
}

FVector2D ResolveHeldDimensions(const UTexture2D& Texture,
                                const UReEchoWeaponPresentationProfile& WeaponProfile,
                                const float CharacterWorldHeight)
{
	const float TextureWidth = FMath::Max(Texture.GetSizeX(), 1);
	const float TextureHeight = FMath::Max(Texture.GetSizeY(), 1);
	const float HeldLength = ResolveHeldLength(WeaponProfile, CharacterWorldHeight);
	if (WeaponProfile.HeldSizeAxis == EReEchoHeldWeaponSizeAxis::Width)
	{
		return FVector2D(HeldLength, HeldLength * TextureHeight / TextureWidth);
	}
	return FVector2D(HeldLength * TextureWidth / TextureHeight, HeldLength);
}

FVector ResolveHeldVisualAttackRangeScale(const FVector& AuthoredScale,
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

FVector ResolveFacingPointAroundCenter(const FVector& Point,
                                       const FVector& CharacterCenter,
                                       const float FacingSign,
                                       FVector CameraRight)
{
	if (FacingSign >= 0.0f)
	{
		return Point;
	}
	CameraRight.Z = 0.0f;
	CameraRight = CameraRight.GetSafeNormal(UE_SMALL_NUMBER, FVector::RightVector);
	const FVector CenterToPoint = Point - CharacterCenter;
	const float HorizontalDistance = FVector::DotProduct(CenterToPoint, CameraRight);
	return Point - 2.0f * HorizontalDistance * CameraRight;
}

FVector
ResolveSelfCenteredSpinRootLocation(const FVector& HandAnchor, const FVector& VisualOffset, const FQuat& SpinRotation)
{
	return HandAnchor + VisualOffset - SpinRotation.RotateVector(VisualOffset);
}

float ResolveTripleSwingAngle(const float Progress, const float DirectionSign)
{
	const float NormalizedProgress = FMath::Clamp(Progress, 0.0f, 1.0f);
	const float Direction = DirectionSign < 0.0f ? -1.0f : 1.0f;
	return TripleSwingAmplitudeRadians * FMath::Cos(NormalizedProgress * TripleSwingHalfCycles * PI) * Direction;
}

bool CanCutRabbitProjectiles(const FName AttackPatternId)
{
	return AttackPatternId == TEXT("Pattern.LongSwordCombo") || AttackPatternId == TEXT("Pattern.ScytheSweep");
}

int32 ResolveConfiguredMaxStacks(const float ConfiguredMaxStacks)
{
	const int32 RoundedMaxStacks = FMath::RoundToInt(ConfiguredMaxStacks);
	return RoundedMaxStacks > 0 ? RoundedMaxStacks : TNumericLimits<int32>::Max();
}
}

#if !UE_BUILD_SHIPPING
namespace ReEchoSwordDamageTrace
{
const FName LongSwordWeaponId = TEXT("W_J_01");
const FName DamageCoefficientRule = TEXT("Weapon.DamageCoefficient");

FString DescribeEquippedParts(const TArray<FReEchoEquippedPartSnapshot>& EquippedParts)
{
	TArray<FString> PartIds;
	PartIds.Reserve(EquippedParts.Num());
	for (const FReEchoEquippedPartSnapshot& Part : EquippedParts)
	{
		PartIds.Add(FString::Printf(TEXT("%s:%s"), *Part.SlotTypeId.ToString(), *Part.PartId.ToString()));
	}
	return PartIds.IsEmpty() ? TEXT("None") : FString::Join(PartIds, TEXT("|"));
}

FString DescribeAttackSteps(const TArray<FReEchoCsvAttackStepRow>& AttackSteps)
{
	TArray<FString> Steps;
	Steps.Reserve(AttackSteps.Num());
	for (const FReEchoCsvAttackStepRow& Step : AttackSteps)
	{
		Steps.Add(FString::Printf(TEXT("%s[%d]=%.3f"), *Step.Id.ToString(), Step.StepIndex, Step.DamageCoefficient));
	}
	return Steps.IsEmpty() ? TEXT("None") : FString::Join(Steps, TEXT("|"));
}
} // namespace ReEchoSwordDamageTrace

namespace ReEchoRangedCritTrace
{
const FName BowWeaponId = TEXT("W_J_08");
const FName GunWeaponId = TEXT("W_J_09");

bool IsCriticalRuneBehavior(const FName BehaviorId)
{
	return BehaviorId == TEXT("Part.ProjectilePierceOnCritical") || BehaviorId == TEXT("Part.ApplyBleedOnCritical");
}

bool HasCriticalRune(const TArray<FReEchoWeaponRuneEffectSpec>& Effects)
{
	return Effects.ContainsByPredicate(
	    [](const FReEchoWeaponRuneEffectSpec& Effect)
	    {
		    return IsCriticalRuneBehavior(Effect.BehaviorId);
	    });
}

bool ShouldTrace(const FName WeaponId, const TArray<FReEchoWeaponRuneEffectSpec>& Effects)
{
	return (WeaponId == BowWeaponId || WeaponId == GunWeaponId) && HasCriticalRune(Effects);
}

FString DescribeEffects(const TArray<FReEchoWeaponRuneEffectSpec>& Effects)
{
	TArray<FString> Descriptions;
	for (const FReEchoWeaponRuneEffectSpec& Effect : Effects)
	{
		if (IsCriticalRuneBehavior(Effect.BehaviorId))
		{
			Descriptions.Add(FString::Printf(TEXT("%s:%s(trigger=%s,value=%.3f,param=%.3f,duration=%.3f)"),
			                                 *Effect.PartId.ToString(),
			                                 *Effect.BehaviorId.ToString(),
			                                 *Effect.Trigger.ToString(),
			                                 Effect.Value,
			                                 Effect.ParamValue,
			                                 Effect.DurationSeconds));
		}
	}
	return Descriptions.IsEmpty() ? TEXT("None") : FString::Join(Descriptions, TEXT("|"));
}
} // namespace ReEchoRangedCritTrace
#endif

AReEchoWeaponActor::AReEchoWeaponActor()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	// 武器只继承持有者位置，不继承鼠标瞄准产生的角色旋转。
	Root->SetAbsolute(false, true, false);
	SwordSpriteRestRotation = ReEchoWeaponVisual::GetSwordRotation(ReEchoWeaponVisual::SwordRestAngleRadians);
	WeaponAttackVfxRoot = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponAttackVfxRoot"));
	WeaponAttackVfxRoot->SetupAttachment(Root);

	ElementIndicator = CreateDefaultSubobject<UTextRenderComponent>(TEXT("ElementIndicator"));
	ElementIndicator->SetupAttachment(Root);
	ElementIndicator->SetHorizontalAlignment(EHTA_Center);
	ElementIndicator->SetVerticalAlignment(EVRTA_TextCenter);
	ElementIndicator->SetWorldSize(28.0f);
	ElementIndicator->SetRelativeLocation(FVector(0.0f, 0.0f, 145.0f));
	ElementIndicator->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ElementIndicator->SetCastShadow(false);
	ElementIndicator->SetTranslucentSortPriority(25);
	ElementIndicator->SetVisibility(false);
	if (UMaterialInterface* UnlitTextMaterial =
	        LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineMaterials/UnlitText.UnlitText")))
	{
		ElementIndicator->SetTextMaterial(UnlitTextMaterial);
	}

	StaffSprite = CreateDefaultSubobject<UBillboardComponent>(TEXT("StaffSprite"));
	StaffSprite->SetupAttachment(Root);
	StaffSprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StaffSprite->SetCastShadow(false);
	StaffSprite->SetTranslucentSortPriority(6);
	StaffSprite->SetRelativeLocation(ReEchoWeaponVisual::StaffLocation);
	StaffSprite->SetHiddenInGame(false);
	StaffSprite->bIsScreenSizeScaled = false;
	const FString StaffTexturePath = FReEchoWeaponVisualCatalog::ResolveHeldTexturePath(TEXT("MoonStaff"));
	if (UTexture2D* StaffTexture = LoadObject<UTexture2D>(nullptr, *StaffTexturePath))
	{
		StaffSprite->SetSprite(StaffTexture);
		constexpr float StaffWorldHeight = 250.0f;
		StaffSprite->SetRelativeScale3D(FVector(StaffWorldHeight / FMath::Max(1, StaffTexture->GetSizeY())));
	}

	// Billboard 会在渲染阶段覆盖组件旋转；使用透明 Plane 才能稳定显示武器自身的 360 度旋转。
	UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
	UMaterialInterface* SpriteMaterial = LoadObject<UMaterialInterface>(
	    nullptr, TEXT("/Paper2D/TranslucentUnlitSpriteMaterial.TranslucentUnlitSpriteMaterial"));
	auto CreateWeaponPlane = [this, PlaneMesh, SpriteMaterial](const TCHAR* Name, const FString& TexturePath)
	{
		UStaticMeshComponent* Plane = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Plane->SetupAttachment(Root);
		Plane->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Plane->SetCastShadow(false);
		Plane->SetTranslucentSortPriority(6);
		Plane->SetHiddenInGame(false);
		Plane->SetStaticMesh(PlaneMesh);
		if (UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *TexturePath); SpriteMaterial && Texture)
		{
			UMaterialInstanceDynamic* MaterialInstance = UMaterialInstanceDynamic::Create(SpriteMaterial, this);
			MaterialInstance->SetTextureParameterValue(TEXT("SpriteTexture"), Texture);
			Plane->SetMaterial(0, MaterialInstance);
		}
		return Plane;
	};
	ScytheSprite =
	    CreateWeaponPlane(TEXT("ScytheSprite"), FReEchoWeaponVisualCatalog::ResolveHeldTexturePath(TEXT("Scythe")));
	BowSprite = CreateWeaponPlane(TEXT("BowSprite"), FReEchoWeaponVisualCatalog::ResolveHeldTexturePath(TEXT("Bow")));
	GunSprite = CreateWeaponPlane(TEXT("GunSprite"), FReEchoWeaponVisualCatalog::ResolveHeldTexturePath(TEXT("Gun")));

	SwordSprite = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SwordSprite"));
	SwordSprite->SetupAttachment(Root);
	SwordSprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SwordSprite->SetCastShadow(false);
	SwordSprite->SetTranslucentSortPriority(-1);
	SwordSprite->SetRelativeLocation(ReEchoWeaponVisual::SwordLocation);
	SwordSprite->SetRelativeRotation(ReEchoWeaponVisual::GetSwordRotation(ReEchoWeaponVisual::SwordRestAngleRadians));
	SwordSprite->SetHiddenInGame(false);
	SwordSprite->SetStaticMesh(PlaneMesh);
	const FString WeaponTexturePath = FReEchoWeaponVisualCatalog::ResolveHeldTexturePath(TEXT("CrescentBlade"));
	UTexture2D* WeaponTexture = LoadObject<UTexture2D>(nullptr, *WeaponTexturePath);
	if (SpriteMaterial && WeaponTexture)
	{
		UMaterialInstanceDynamic* MaterialInstance = UMaterialInstanceDynamic::Create(SpriteMaterial, this);
		MaterialInstance->SetTextureParameterValue(TEXT("SpriteTexture"), WeaponTexture);
		SwordSprite->SetMaterial(0, MaterialInstance);
		constexpr float WeaponWorldWidth = 250.0f;
		const float AspectRatio =
		    static_cast<float>(WeaponTexture->GetSizeY()) / FMath::Max(1, WeaponTexture->GetSizeX());
		SwordSprite->SetRelativeScale3D(
		    FVector(WeaponWorldWidth / 100.0f, WeaponWorldWidth * AspectRatio / 100.0f, 1.0f));
	}
	SetActorEnableCollision(false);
}

void AReEchoWeaponActor::ConfigureHeldPresentation(const UReEcho2DCharacterPresentationProfile* CharacterProfile)
{
	HeldCharacterProfile = CharacterProfile;
	RefreshHeldPresentation();
}

void AReEchoWeaponActor::InitializeWeapon(const FReEchoBuildSnapshot* InBuildSnapshot,
                                          TSharedPtr<const FReEchoCsvDataSnapshot> InSnapshot)
{
	SwordSpriteRestLocation = ReEchoWeaponVisual::SwordLocation;
	WeaponLogic.Reset();
	LastAttackCommit = {};
	LastRuneAttackContext.Reset();
	PersistentRuneHitCounters.Reset();
	PersistentTargetHitCounters.Reset();
	TimedRangeStacks.Reset();
	bScytheThrown = false;
	ScytheRuneContext.Reset();
	LastCommittedAttackStepId = NAME_None;
	LastCommittedAttackStepIndex = INDEX_NONE;
	Definitions.Reset();
	DataSnapshot = InSnapshot.IsValid() ? InSnapshot : FReEchoCsvDataRegistry::GetSnapshot();
	if (!DataSnapshot.IsValid())
	{
		UE_LOG(LogReEcho, Fatal, TEXT("Cannot initialize weapon actor: CSV snapshot is unavailable"));
	}
	for (const FName WeaponId : DataSnapshot->WeaponOrder)
	{
		const FReEchoCsvWeaponRow* Definition = DataSnapshot->FindEnabledWeapon(WeaponId);
		if (Definition)
		{
			Definitions.Add(Definition->Id, *Definition);
		}
	}
	if (Definitions.IsEmpty())
	{
		UE_LOG(LogReEcho, Fatal, TEXT("Cannot initialize weapon actor: no enabled CSV weapons"));
	}
	if (InBuildSnapshot)
	{
		TArray<FName> PartIds;
		for (const FReEchoEquippedPartSnapshot& Part : InBuildSnapshot->EquippedParts)
		{
			PartIds.Add(Part.PartId);
		}
		FString Error;
		if (!ReEchoWeaponRuntime::TryEquipParts(*DataSnapshot, *InBuildSnapshot, PartIds, BuildSnapshot, Error))
		{
			UE_LOG(LogReEcho, Fatal, TEXT("Cannot initialize weapon actor: %s"), *Error);
		}
		EquippedRunes = BuildSnapshot.EquippedParts;
	}
	else
	{
		BuildSnapshot = {};
		BuildSnapshot.WeaponDomainRevision = DataSnapshot->WeaponDomainRevision;
		EquippedRunes.Reset();
	}
	const FReEchoCsvWeaponRow* InitialWeapon = InBuildSnapshot
	                                               ? DataSnapshot->FindEnabledWeapon(BuildSnapshot.WeaponId)
	                                               : DataSnapshot->FindWeaponByInputSlot(EReEchoInputSlot::Slot1);
	EquippedWeaponId = InitialWeapon ? InitialWeapon->Id : Definitions.CreateConstIterator().Key();
	if (const FReEchoCsvWeaponRow* Equipped = Definitions.Find(EquippedWeaponId))
	{
		BuildSnapshot.WeaponId = Equipped->Id;
		BuildSnapshot.WeaponDataRevision = Equipped->DataRevision;
		BuildSnapshot.WeaponDomainRevision = DataSnapshot->WeaponDomainRevision;
	}
	LastAttackCommit = {};
	LastCommittedAttackStepId = NAME_None;
	LastCommittedAttackStepIndex = INDEX_NONE;
	SwordAnimationTime = 0.0f;
	if (!RebuildEffectiveDefinition())
	{
		UE_LOG(LogReEcho,
		       Fatal,
		       TEXT("Cannot initialize effective weapon definition for WeaponId '%s'"),
		       *EquippedWeaponId.ToString());
	}
	if (!WeaponLogic.Initialize(ReEchoWeaponRuntime::CompileLogicDefinition(EffectiveDefinition)))
	{
		UE_LOG(
		    LogReEcho, Fatal, TEXT("Cannot initialize weapon logic for WeaponId '%s'"), *EquippedWeaponId.ToString());
	}
	RefreshVisualState();
	RefreshHeldPresentation();
	UpdateElementIndicator();
	SwordSprite->SetRelativeLocation(SwordSpriteRestLocation);
	SwordSprite->SetRelativeRotation(SwordSpriteRestRotation);
}

bool AReEchoWeaponActor::SelectWeaponById(const FName WeaponId)
{
	if (GetEquippedWeaponId() == WeaponId)
	{
		return true;
	}
	if (!DataSnapshot.IsValid() || !Definitions.Contains(WeaponId))
	{
		return false;
	}

	FString Error;
	FReEchoBuildSnapshot CandidateBuild;
	if (!ReEchoWeaponRuntime::TrySelectWeapon(*DataSnapshot, BuildSnapshot, WeaponId, CandidateBuild, Error))
	{
		UE_LOG(LogReEcho, Error, TEXT("Cannot select weapon: %s"), *Error);
		return false;
	}
	FReEchoEffectiveWeaponDefinition CandidateDefinition;
	if (!ReEchoWeaponRuntime::BuildEffectiveWeaponDefinition(*DataSnapshot, CandidateBuild, CandidateDefinition, Error))
	{
		UE_LOG(LogReEcho, Error, TEXT("Cannot select weapon: %s"), *Error);
		return false;
	}

	BuildSnapshot = CandidateBuild;
	EffectiveDefinition = CandidateDefinition;
	bHasEffectiveDefinition = true;
	EquippedWeaponId = WeaponId;
	EquippedRunes = BuildSnapshot.EquippedParts;
	LastAttackCommit = {};
	LastRuneAttackContext.Reset();
	TimedRangeStacks.Reset();
	RecallScythe();
	LastCommittedAttackStepId = NAME_None;
	LastCommittedAttackStepIndex = INDEX_NONE;
	SwordAnimationTime = 0.0f;
	if (!WeaponLogic.Initialize(ReEchoWeaponRuntime::CompileLogicDefinition(EffectiveDefinition)))
	{
		return false;
	}
	RefreshVisualState();
	RefreshHeldPresentation();
	UpdateElementIndicator();
	SwordSprite->SetRelativeLocation(SwordSpriteRestLocation);
	SwordSprite->SetRelativeRotation(SwordSpriteRestRotation);
	return true;
}

bool AReEchoWeaponActor::EquipRune(FName PartId, FString& OutError)
{
	if (!DataSnapshot.IsValid())
	{
		OutError = TEXT("Cannot equip rune: CSV snapshot is unavailable");
		return false;
	}
	const FReEchoCsvWeaponRow* Weapon = Definitions.Find(EquippedWeaponId);
	if (!Weapon)
	{
		OutError = FString::Printf(TEXT("Cannot equip rune '%s': no equipped weapon"), *PartId.ToString());
		return false;
	}
	// Candidate part list = currently equipped runes + the new one. Reuse TryEquipParts so the
	// exact same compatibility / capacity / enabled / effect validation and effect rebuild apply.
	TArray<FName> PartIds;
	PartIds.Reserve(EquippedRunes.Num() + 1);
	for (const FReEchoEquippedPartSnapshot& Equipped : EquippedRunes)
	{
		PartIds.Add(Equipped.PartId);
	}
	PartIds.Add(PartId);

	FReEchoBuildSnapshot CandidateBuild;
	if (!ReEchoWeaponRuntime::TryEquipParts(*DataSnapshot, BuildSnapshot, PartIds, CandidateBuild, OutError))
	{
		// State unchanged: validation failed (incompatible / over-capacity / duplicate / disabled).
		return false;
	}
	BuildSnapshot = CandidateBuild;
	EquippedRunes = CandidateBuild.EquippedParts;
	if (!RebuildEffectiveDefinition())
	{
		OutError = TEXT("Cannot equip rune: failed to rebuild effective weapon definition");
		return false;
	}
	if (!WeaponLogic.Initialize(ReEchoWeaponRuntime::CompileLogicDefinition(EffectiveDefinition)))
	{
		OutError = TEXT("Cannot equip rune: failed to recompile weapon logic");
		return false;
	}
	return true;
}

bool AReEchoWeaponActor::UnequipRune(FName SlotTypeId, FString& OutError)
{
	if (!DataSnapshot.IsValid())
	{
		OutError = TEXT("Cannot unequip rune: CSV snapshot is unavailable");
		return false;
	}
	if (!Definitions.Find(EquippedWeaponId))
	{
		OutError = TEXT("Cannot unequip rune: no equipped weapon");
		return false;
	}
	// Candidate part list = every currently equipped rune except those in the requested slot.
	TArray<FName> PartIds;
	for (const FReEchoEquippedPartSnapshot& Equipped : EquippedRunes)
	{
		if (Equipped.SlotTypeId != SlotTypeId)
		{
			PartIds.Add(Equipped.PartId);
		}
	}

	FReEchoBuildSnapshot CandidateBuild;
	if (!ReEchoWeaponRuntime::TryEquipParts(*DataSnapshot, BuildSnapshot, PartIds, CandidateBuild, OutError))
	{
		return false;
	}
	BuildSnapshot = CandidateBuild;
	EquippedRunes = CandidateBuild.EquippedParts;
	if (!RebuildEffectiveDefinition())
	{
		OutError = TEXT("Cannot unequip rune: failed to rebuild effective weapon definition");
		return false;
	}
	if (!WeaponLogic.Initialize(ReEchoWeaponRuntime::CompileLogicDefinition(EffectiveDefinition)))
	{
		OutError = TEXT("Cannot unequip rune: failed to recompile weapon logic");
		return false;
	}
	return true;
}

bool AReEchoWeaponActor::TryBasicAttack(UReEchoCombatantComponent* Combatant)
{
	if (!Combatant || !WeaponLogic.TryCommitBasicAttack(GetOwner(), Combatant->Stats, LastAttackCommit))
	{
		return false;
	}
	LastCommittedAttackStepId = LastAttackCommit.AttackStepId;
	LastCommittedAttackStepIndex = LastAttackCommit.StepIndex;
#if !UE_BUILD_SHIPPING
	if (ReEchoRangedCritTrace::ShouldTrace(LastAttackCommit.WeaponId, EffectiveDefinition.RuneEffects))
	{
		UE_LOG(LogReEcho,
		       Warning,
		       TEXT("[RangedCritTrace] Commit owner=%s weapon=%s sequence=%lld parts=%s effects=%s role=%s "
		            "physical=%.3f elemental=%.3f critRate=%.3f critEffect=%.3f step=%s[%d] element=%d "
		            "critical=%d rawDamage=%.3f projectiles=%d range=%.3f"),
		       *GetNameSafe(GetOwner()),
		       *LastAttackCommit.WeaponId.ToString(),
		       static_cast<long long>(LastAttackCommit.Attack.Sequence),
		       *ReEchoSwordDamageTrace::DescribeEquippedParts(EquippedRunes),
		       *ReEchoRangedCritTrace::DescribeEffects(EffectiveDefinition.RuneEffects),
		       *Combatant->Stats.RoleId.ToString(),
		       Combatant->Stats.PhysicalAttack,
		       Combatant->Stats.ElementalAttack,
		       Combatant->Stats.CriticalRate,
		       Combatant->Stats.CriticalEffect,
		       *LastAttackCommit.AttackStepId.ToString(),
		       LastAttackCommit.StepIndex,
		       static_cast<int32>(LastAttackCommit.Element),
		       LastAttackCommit.bCritical ? 1 : 0,
		       LastAttackCommit.RawDamage,
		       LastAttackCommit.ProjectileCount,
		       LastAttackCommit.RangeCm);
	}
	if (LastAttackCommit.WeaponId == ReEchoSwordDamageTrace::LongSwordWeaponId)
	{
		UE_LOG(LogReEcho,
		       Warning,
		       TEXT("[SwordDamageTrace] Commit owner=%s parts=%s role=%s physical=%.3f elemental=%.3f attackSpeed=%.3f "
		            "step=%s[%d] element=%d critical=%d rawDamage=%.3f"),
		       *GetNameSafe(GetOwner()),
		       *ReEchoSwordDamageTrace::DescribeEquippedParts(EquippedRunes),
		       *Combatant->Stats.RoleId.ToString(),
		       Combatant->Stats.PhysicalAttack,
		       Combatant->Stats.ElementalAttack,
		       Combatant->Stats.AttackSpeed,
		       *LastAttackCommit.AttackStepId.ToString(),
		       LastAttackCommit.StepIndex,
		       static_cast<int32>(LastAttackCommit.Element),
		       LastAttackCommit.bCritical ? 1 : 0,
		       LastAttackCommit.RawDamage);
	}
#endif
	const FReEchoWeaponAttackCommit EffectiveCommit = BuildEffectiveAttackCommit(LastAttackCommit);
	if (!ExecuteAttack(Combatant, EffectiveCommit))
	{
		WeaponLogic.RollbackLastCommit();
		LastCommittedAttackStepId = NAME_None;
		LastCommittedAttackStepIndex = INDEX_NONE;
		return false;
	}
	WeaponLogic.ConfirmLastCommit();
	ProcessOnAttackRuneEffects(LastRuneAttackContext);
	UpdateElementIndicator();
	PublishAttackCommittedEvent(EffectiveCommit);
	return true;
}

void AReEchoWeaponActor::PublishAttackCommittedEvent(const FReEchoWeaponAttackCommit& Commit) const
{
	AActor* WeaponOwner = GetOwner();
	if (!WeaponOwner)
	{
		ReEchoBuildTrace::LogAttackCommit(
		    nullptr, nullptr, FVector::ZeroVector, FVector::ZeroVector, BuildSnapshot, Commit);
		return;
	}
	AActor* CommittedTarget = nullptr;
	if (const UReEchoAttackControllerComponent* Controller =
	        WeaponOwner->FindComponentByClass<UReEchoAttackControllerComponent>())
	{
		CommittedTarget = Controller->GetSnapshot().CurrentTarget;
	}
	const FVector Origin = WeaponOwner->GetActorLocation();
	const FVector ToCommittedTarget =
	    CommittedTarget ? CommittedTarget->GetActorLocation() - Origin : FVector::ZeroVector;
	const FVector Direction =
	    ToCommittedTarget.IsNearlyZero() ? ResolveOwnerAimDirection() : ToCommittedTarget.GetSafeNormal2D();
	ReEchoBuildTrace::LogAttackCommit(WeaponOwner, CommittedTarget, Origin, Direction, BuildSnapshot, Commit);

	UReEchoCombatEventsComponent* Events = WeaponOwner->FindComponentByClass<UReEchoCombatEventsComponent>();
	if (!Events)
	{
		return;
	}
	FReEchoAttackCommittedEvent Event;
	Event.Attack = Commit.Attack;
	Event.Target = CommittedTarget;
	Event.WeaponId = Commit.WeaponId;
	Event.AttackPatternId = Commit.AttackPatternId;
	Event.AttackStepId = Commit.AttackStepId;
	Event.StepIndex = Commit.StepIndex;
	Event.Element = Commit.Element;
#if !UE_BUILD_SHIPPING
	if (const AReEchoPlayerPawn* PlayerOwner = Cast<AReEchoPlayerPawn>(WeaponOwner);
	    PlayerOwner && ReEchoElementReaction::IsCombatElement(PlayerOwner->GetDebugOutgoingElementOverride()))
	{
		Event.Element = PlayerOwner->GetDebugOutgoingElementOverride();
	}
#endif
	Event.Origin = Origin;
	Event.Direction = Direction;
	Event.EffectiveRangeCm = Commit.RangeCm;
	Event.BaseRangeCm = ResolveBaseAttackRangeCm(Commit.AttackStepId, Commit.RangeCm);
	Event.RangeMultiplierFromBase =
	    Event.BaseRangeCm > UE_SMALL_NUMBER ? FMath::Max(0.01f, Event.EffectiveRangeCm / Event.BaseRangeCm) : 1.0f;
	Event.EffectiveArcDegrees = Commit.ArcDegrees;
	Events->PublishAttackCommitted(Event);
}

FReEchoWeaponAttackCommit AReEchoWeaponActor::BuildEffectiveAttackCommit(const FReEchoWeaponAttackCommit& Commit) const
{
	FReEchoWeaponAttackCommit EffectiveCommit = Commit;
	EffectiveCommit.RangeCm *= GetTimedRangeMultiplier();
	return EffectiveCommit;
}

float AReEchoWeaponActor::ResolveBaseAttackRangeCm(const FName AttackStepId, const float FallbackRangeCm) const
{
	if (DataSnapshot.IsValid())
	{
		if (const FReEchoCsvAttackStepRow* Step = DataSnapshot->AttackSteps.Find(AttackStepId))
		{
			return Step->RangeCm > UE_SMALL_NUMBER ? Step->RangeCm : FallbackRangeCm;
		}
	}
	return FallbackRangeCm;
}

bool AReEchoWeaponActor::TryActiveAttack(UReEchoCombatantComponent* Combatant)
{
	if (bScytheThrown)
	{
		RecallScythe();
		return true;
	}
	if (!Combatant || !WeaponLogic.TryCommitActiveAttack(GetOwner(), Combatant->Stats, LastAttackCommit))
	{
		return false;
	}
	const FReEchoWeaponAttackCommit EffectiveCommit = BuildEffectiveAttackCommit(LastAttackCommit);
	if (HasRuneBehavior(TEXT("Part.ScytheThrowRecall")))
	{
		LastRuneAttackContext = BuildRuneAttackContext(EffectiveCommit, Combatant);
		BeginScytheThrow(LastRuneAttackContext);
	}
	else if (!ExecuteAttack(Combatant, EffectiveCommit))
	{
		WeaponLogic.RollbackLastCommit();
		return false;
	}
	WeaponLogic.ConfirmLastCommit();
	ProcessOnAttackRuneEffects(LastRuneAttackContext);
	UpdateElementIndicator();
	PublishAttackCommittedEvent(EffectiveCommit);
	return true;
}

bool AReEchoWeaponActor::ExecuteBasicAttack(UReEchoCombatantComponent* Combatant)
{
	return TryBasicAttack(Combatant);
}

float AReEchoWeaponActor::GetAttackInterval(UReEchoCombatantComponent* Combatant) const
{
	return bHasEffectiveDefinition && Combatant ? WeaponLogic.GetAttackInterval(Combatant->Stats) : 0.55f;
}

float AReEchoWeaponActor::GetCurrentAttackRangeCm() const
{
	if (!bHasEffectiveDefinition)
	{
		return 0.0f;
	}
	return WeaponLogic.GetCurrentRangeCm() * GetTimedRangeMultiplier();
}

float AReEchoWeaponActor::GetAttackCooldownRemaining() const
{
	return WeaponLogic.GetSnapshot().ReadinessRemainingSeconds;
}

FName AReEchoWeaponActor::GetCurrentAttackStepId() const
{
	return WeaponLogic.GetSnapshot().NextAttackStepId;
}

FName AReEchoWeaponActor::GetAttackPatternId() const
{
	return bHasEffectiveDefinition ? EffectiveDefinition.Weapon.AttackPatternId : NAME_None;
}

bool AReEchoWeaponActor::ExecuteAttack(UReEchoCombatantComponent* Combatant, const FReEchoWeaponAttackCommit& Commit)
{
	if (!Combatant || !bHasEffectiveDefinition)
	{
		return false;
	}
	if (Commit.MovementCm > 0.0f)
	{
		AActor* WeaponOwner = GetOwner();
		const FVector Direction = ResolveOwnerAimDirection();
		if (WeaponOwner)
		{
			WeaponOwner->SetActorLocation(WeaponOwner->GetActorLocation() +
			                                  (Direction.IsNearlyZero() ? FVector::ForwardVector : Direction) *
			                                      Commit.MovementCm,
			                              false,
			                              nullptr,
			                              ETeleportType::TeleportPhysics);
		}
	}
	LastRuneAttackContext = BuildRuneAttackContext(Commit, Combatant);
	LastRuneAttackContext->Commit = Commit;
	switch (Commit.Carrier)
	{
		case EReEchoWeaponAttackCarrier::Wave:
			return FireStaffLightWave(Commit, Combatant);
		case EReEchoWeaponAttackCarrier::Projectile:
			return FireProjectile(Commit, Combatant, LastRuneAttackContext);
		default:
			return SwingMelee(Commit, Combatant, LastRuneAttackContext);
	}
}

FName AReEchoWeaponActor::GetEquippedWeaponId() const
{
	return EquippedWeaponId;
}

FName AReEchoWeaponActor::GetEquippedWeaponVisualKey() const
{
	const FReEchoCsvWeaponRow* Definition = FindEquippedDefinition();
	return Definition ? Definition->VisualKey : NAME_None;
}

FString AReEchoWeaponActor::GetEquippedWeaponLabel() const
{
	if (const FReEchoCsvWeaponRow* Definition = FindEquippedDefinition())
	{
		return Definition->DisplayName.IsEmpty() ? Definition->Id.ToString() : Definition->DisplayName;
	}
	return TEXT("Unknown");
}

const FReEchoBuildSnapshot& AReEchoWeaponActor::GetBuildSnapshot() const
{
	return BuildSnapshot;
}

FString AReEchoWeaponActor::GetPinnedWeaponDomainRevision() const
{
	return DataSnapshot.IsValid() ? DataSnapshot->WeaponDomainRevision : FString();
}

const FReEchoCsvWeaponRow* AReEchoWeaponActor::FindEquippedDefinition() const
{
	return Definitions.Find(EquippedWeaponId);
}

bool AReEchoWeaponActor::IsInvulnerableWindowActive() const
{
	return WeaponLogic.GetSnapshot().bInvulnerable;
}

FVector AReEchoWeaponActor::ResolveOwnerAimDirection() const
{
	const AActor* WeaponOwner = GetOwner();
	if (!WeaponOwner)
	{
		return FVector::ForwardVector;
	}
	if (const AReEchoPlayerPawn* Player = Cast<AReEchoPlayerPawn>(WeaponOwner))
	{
		const FVector PlayerAim = Player->GetAttackAimDirection().GetSafeNormal2D();
		if (!PlayerAim.IsNearlyZero())
		{
			return PlayerAim;
		}
	}
	if (const AReEchoEchoActor* Echo = Cast<AReEchoEchoActor>(WeaponOwner))
	{
		const FVector EchoAim = Echo->GetAttackAimDirection().GetSafeNormal2D();
		if (!EchoAim.IsNearlyZero())
		{
			return EchoAim;
		}
	}
	const FVector OwnerForward = WeaponOwner->GetActorForwardVector().GetSafeNormal2D();
	return OwnerForward.IsNearlyZero() ? FVector::ForwardVector : OwnerForward;
}

FVector AReEchoWeaponActor::ResolveAutomaticAimDirectionToTarget(const FVector& TargetLocation) const
{
	const AActor* WeaponOwner = GetOwner();
	if (!WeaponOwner)
	{
		return FVector::ForwardVector;
	}

	const FVector OwnerLocation = WeaponOwner->GetActorLocation();
	FVector OwnerToTarget = TargetLocation - OwnerLocation;
	OwnerToTarget.Z = 0.0f;
	const FVector OwnerDirection = OwnerToTarget.GetSafeNormal2D();
	if (OwnerDirection.IsNearlyZero())
	{
		return ResolveOwnerAimDirection();
	}

	const FName VisualKey = GetEquippedWeaponVisualKey();
	if (VisualKey != TEXT("Bow") && VisualKey != TEXT("Gun"))
	{
		return OwnerDirection;
	}

	const bool bHasWeaponAnchor = IsValid(WeaponAttackVfxRoot);
	const FVector WeaponAnchorLocation =
	    bHasWeaponAnchor ? WeaponAttackVfxRoot->GetComponentLocation() : FVector::ZeroVector;
	const FVector SpawnLocation = ReEchoWeaponVisual::ResolveProjectileSpawnLocation(
	    WeaponAnchorLocation, OwnerLocation, OwnerDirection, bHasWeaponAnchor);
	FVector SpawnToTarget = TargetLocation - SpawnLocation;
	SpawnToTarget.Z = 0.0f;
	const FVector ProjectileDirection = SpawnToTarget.GetSafeNormal2D();
	return ProjectileDirection.IsNearlyZero() ? OwnerDirection : ProjectileDirection;
}

EReEchoDamageSource AReEchoWeaponActor::ResolveOwnerDamageSource() const
{
	return ReEchoCombatRelations::ResolveActorDamageSource(GetOwner(), EReEchoDamageSource::Player);
}

bool AReEchoWeaponActor::RebuildEffectiveDefinition()
{
	if (!DataSnapshot.IsValid())
	{
		return false;
	}
	FString Error;
	bHasEffectiveDefinition =
	    ReEchoWeaponRuntime::BuildEffectiveWeaponDefinition(*DataSnapshot, BuildSnapshot, EffectiveDefinition, Error);
	if (!bHasEffectiveDefinition)
	{
		UE_LOG(LogReEcho, Error, TEXT("Cannot build effective weapon definition: %s"), *Error);
	}
#if !UE_BUILD_SHIPPING
	else if (EquippedWeaponId == ReEchoSwordDamageTrace::LongSwordWeaponId)
	{
		UE_LOG(LogReEcho,
		       Warning,
		       TEXT("[SwordDamageTrace] Definition owner=%s parts=%s role=%s equipmentBasePhysical=%.3f "
		            "buildPhysical=%.3f equipmentBaseElemental=%.3f buildElemental=%.3f equipmentBaseAttackSpeed=%.3f "
		            "buildAttackSpeed=%.3f weaponCoefficient=%.3f ruleCoefficient=%s damageChannel=%s steps=%s"),
		       *GetNameSafe(GetOwner()),
		       *ReEchoSwordDamageTrace::DescribeEquippedParts(EquippedRunes),
		       *BuildSnapshot.Stats.RoleId.ToString(),
		       BuildSnapshot.EquipmentBaseStats.PhysicalAttack,
		       BuildSnapshot.Stats.PhysicalAttack,
		       BuildSnapshot.EquipmentBaseStats.ElementalAttack,
		       BuildSnapshot.Stats.ElementalAttack,
		       BuildSnapshot.EquipmentBaseStats.AttackSpeed,
		       BuildSnapshot.Stats.AttackSpeed,
		       EffectiveDefinition.Weapon.DamageCoefficient,
		       *BuildSnapshot.RuleFlags.FindRef(ReEchoSwordDamageTrace::DamageCoefficientRule),
		       *EffectiveDefinition.DamageChannelId.ToString(),
		       *ReEchoSwordDamageTrace::DescribeAttackSteps(EffectiveDefinition.AttackSteps));
	}
	else if (ReEchoRangedCritTrace::ShouldTrace(EquippedWeaponId, EffectiveDefinition.RuneEffects))
	{
		UE_LOG(LogReEcho,
		       Warning,
		       TEXT("[RangedCritTrace] Definition owner=%s weapon=%s parts=%s effects=%s role=%s "
		            "equipmentBasePhysical=%.3f buildPhysical=%.3f equipmentBaseElemental=%.3f buildElemental=%.3f "
		            "critRate=%.3f critEffect=%.3f coefficient=%.3f ruleCoefficient=%s channel=%s steps=%s"),
		       *GetNameSafe(GetOwner()),
		       *EquippedWeaponId.ToString(),
		       *ReEchoSwordDamageTrace::DescribeEquippedParts(EquippedRunes),
		       *ReEchoRangedCritTrace::DescribeEffects(EffectiveDefinition.RuneEffects),
		       *BuildSnapshot.Stats.RoleId.ToString(),
		       BuildSnapshot.EquipmentBaseStats.PhysicalAttack,
		       BuildSnapshot.Stats.PhysicalAttack,
		       BuildSnapshot.EquipmentBaseStats.ElementalAttack,
		       BuildSnapshot.Stats.ElementalAttack,
		       BuildSnapshot.Stats.CriticalRate,
		       BuildSnapshot.Stats.CriticalEffect,
		       EffectiveDefinition.Weapon.DamageCoefficient,
		       *BuildSnapshot.RuleFlags.FindRef(ReEchoSwordDamageTrace::DamageCoefficientRule),
		       *EffectiveDefinition.DamageChannelId.ToString(),
		       *ReEchoSwordDamageTrace::DescribeAttackSteps(EffectiveDefinition.AttackSteps));
	}
#endif
	return bHasEffectiveDefinition;
}

#if WITH_DEV_AUTOMATION_TESTS
TSharedPtr<FReEchoWeaponRuneAttackContext>
AReEchoWeaponActor::BuildRuneAttackContextForTests(const FReEchoWeaponAttackCommit& Commit,
                                                   UReEchoCombatantComponent* Combatant) const
{
	return BuildRuneAttackContext(Commit, Combatant);
}

void AReEchoWeaponActor::ProcessRuneHitForTests(const TSharedPtr<FReEchoWeaponRuneAttackContext>& Context,
                                                const FReEchoHitResolved& Result)
{
	ProcessResolvedHit(Context, Result);
}

void AReEchoWeaponActor::ProcessRuneAttackForTests(const TSharedPtr<FReEchoWeaponRuneAttackContext>& Context)
{
	ProcessAttackResolved(Context);
}

void AReEchoWeaponActor::ProcessRuneOnAttackForTests(const TSharedPtr<FReEchoWeaponRuneAttackContext>& Context)
{
	LastRuneAttackContext = Context;
	ProcessOnAttackRuneEffects(Context);
}

FReEchoHitResolved
AReEchoWeaponActor::ApplyRuneDamageForTests(AActor& Target,
                                            const FReEchoWeaponAttackCommit& Commit,
                                            const FVector& DamageSource,
                                            UReEchoCombatantComponent* Combatant,
                                            const TSharedPtr<FReEchoWeaponRuneAttackContext>& Context)
{
	return ApplyDamageToTarget(Target, Commit, DamageSource, Combatant, Context);
}

float AReEchoWeaponActor::GetTimedRangeMultiplierForTests() const
{
	return GetTimedRangeMultiplier();
}

bool AReEchoWeaponActor::IsScytheThrownForTests() const
{
	return bScytheThrown;
}

void AReEchoWeaponActor::AdvanceScytheThrowForTests(const float DeltaSeconds)
{
	AdvanceScytheThrow(DeltaSeconds);
}
#endif

TSharedPtr<FReEchoWeaponRuneAttackContext>
AReEchoWeaponActor::BuildRuneAttackContext(const FReEchoWeaponAttackCommit& Commit,
                                           UReEchoCombatantComponent* Combatant) const
{
	TSharedPtr<FReEchoWeaponRuneAttackContext> Context = MakeShared<FReEchoWeaponRuneAttackContext>();
	Context->Commit = Commit;
	Context->Effects = EffectiveDefinition.RuneEffects;
	Context->SourceCombatant = Combatant;
	Context->DamageSource = ResolveOwnerDamageSource();
	Context->WeaponVisualKey = GetEquippedWeaponVisualKey();
	Context->ReactionEfficiency = Combatant ? Combatant->Stats.ReactionEfficiency : 1.0f;
	Context->OnKillHealPercent = WeaponLogic.GetOnKillHealPercent();
	return Context;
}

bool AReEchoWeaponActor::HasRuneBehavior(const FName BehaviorId) const
{
	return EffectiveDefinition.RuneEffects.ContainsByPredicate(
	    [BehaviorId](const FReEchoWeaponRuneEffectSpec& Effect)
	    {
		    return Effect.BehaviorId == BehaviorId;
	    });
}

float AReEchoWeaponActor::GetRuneParam(const FName BehaviorId, const FName ParamName, const float DefaultValue) const
{
	for (const FReEchoWeaponRuneEffectSpec& Effect : EffectiveDefinition.RuneEffects)
	{
		if (Effect.BehaviorId == BehaviorId && Effect.ParamName == ParamName)
		{
			return Effect.ParamValue;
		}
	}
	return DefaultValue;
}

float AReEchoWeaponActor::GetRuneEffectValue(const FName BehaviorId,
                                             const FName ParamName,
                                             const float DefaultValue) const
{
	for (const FReEchoWeaponRuneEffectSpec& Effect : EffectiveDefinition.RuneEffects)
	{
		if (Effect.BehaviorId == BehaviorId && Effect.ParamName == ParamName)
		{
			return Effect.Value;
		}
	}
	return DefaultValue;
}

float AReEchoWeaponActor::GetTimedRangeMultiplier() const
{
	float BonusFraction = 0.0f;
	for (const FTimedRangeStack& Stack : TimedRangeStacks)
	{
		BonusFraction += Stack.BonusFraction;
	}
	return FMath::Max(0.01f, 1.0f + BonusFraction);
}

float AReEchoWeaponActor::ResolveCurrentAttackRangeMultiplier() const
{
	const float EffectiveRangeCm = GetCurrentAttackRangeCm();
	const float BaseRangeCm = ResolveBaseAttackRangeCm(GetCurrentAttackStepId(), EffectiveRangeCm);
	return BaseRangeCm > UE_SMALL_NUMBER ? FMath::Max(0.01f, EffectiveRangeCm / BaseRangeCm) : 1.0f;
}

void AReEchoWeaponActor::ProcessOnAttackRuneEffects(const TSharedPtr<FReEchoWeaponRuneAttackContext>& Context)
{
	if (!Context.IsValid())
	{
		return;
	}
	const float WorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	LastRuneAttackWorldTime = WorldTime;
	NextComboDecayWorldTime = WorldTime + 1.0f;
	UReEchoCombatantComponent* SourceCombatant = Context->SourceCombatant.Get();
	if (!SourceCombatant)
	{
		return;
	}
	for (const FReEchoWeaponRuneEffectSpec& Effect : Context->Effects)
	{
		if (Effect.Trigger != TEXT("OnAttack"))
		{
			continue;
		}
		const int32 MaxStacks = ReEchoWeaponVisual::ResolveConfiguredMaxStacks(Effect.ParamValue);
		if (Effect.BehaviorId == TEXT("Part.AttackMoveSpeedOnAttack"))
		{
			SourceCombatant->AddTransientStatModifier(
			    Effect.PartId, Effect.Value, Effect.Value, Effect.DurationSeconds, MaxStacks);
		}
		else if (Effect.BehaviorId == TEXT("Part.AttackSpeedOnAttack"))
		{
			SourceCombatant->AddTransientStatModifier(
			    Effect.PartId, Effect.Value, 0.0f, Effect.DurationSeconds, MaxStacks);
		}
		else if (Effect.BehaviorId == TEXT("Part.MoveSpeedOnAttack"))
		{
			SourceCombatant->AddTransientStatModifier(
			    Effect.PartId, 0.0f, Effect.Value, Effect.DurationSeconds, MaxStacks);
		}
	}
}

void AReEchoWeaponActor::ProcessResolvedHit(const TSharedPtr<FReEchoWeaponRuneAttackContext>& Context,
                                            const FReEchoHitResolved& Result)
{
	if (!Context.IsValid() || Result.AppliedDamage <= 0.0f || !Result.Target)
	{
		return;
	}
	UReEchoCombatantComponent* SourceCombatant = Context->SourceCombatant.Get();
	const IReEchoCombatTarget* CombatTarget = Cast<IReEchoCombatTarget>(Result.Target);
	UReEchoCombatantComponent* TargetCombatant = CombatTarget ? CombatTarget->GetCombatTargetCombatant() : nullptr;
	if (!TargetCombatant)
	{
		return;
	}
	++Context->EffectiveHitCount;
	Context->EffectiveHitTargets.AddUnique(Result.Target);
	const int32 HitOrdinal = ++Context->TargetHitOrdinals.FindOrAdd(Result.Target);
	const float WorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	auto DeterministicRoll = [&](const FReEchoWeaponRuneEffectSpec& Effect, const float Chance)
	{
		uint32 Hash = HashCombine(GetTypeHash(Context->Commit.Attack.Sequence), GetTypeHash(Effect.PartId));
		Hash = HashCombine(Hash, GetTypeHash(CombatTarget->GetCombatTargetTieBreakIndex()));
		Hash = HashCombine(Hash, GetTypeHash(HitOrdinal));
		return static_cast<float>(Hash % 100000u) / 100000.0f < Chance;
	};
	auto ApplyBleeding = [&](const FReEchoWeaponRuneEffectSpec& Effect)
	{
		FReEchoTimedStatusCommand Command;
		Command.StatusId = TEXT("Z_Bleeding");
		Command.CurrentTimeSeconds = WorldTime;
		Command.DurationSeconds = Effect.DurationSeconds;
		Command.DamagePerTickMaxHealthFraction = Effect.Value;
		Command.Attack = Result.Attack;
		Command.DamageSource = Result.DamageSource;
		TargetCombatant->ApplyTimedStatus(Command);
	};
	auto ApplyStun = [&](const FReEchoWeaponRuneEffectSpec& Effect)
	{
		FReEchoTimedStatusCommand Command;
		Command.StatusId = TEXT("Z_Vertigo");
		Command.CurrentTimeSeconds = WorldTime;
		Command.DurationSeconds = Effect.DurationSeconds;
		Command.Attack = Result.Attack;
		Command.DamageSource = Result.DamageSource;
		TargetCombatant->ApplyTimedStatus(Command);
	};

	for (const FReEchoWeaponRuneEffectSpec& Effect : Context->Effects)
	{
		if (Effect.BehaviorId == TEXT("Part.HealOnHit") && SourceCombatant)
		{
			SourceCombatant->ApplyHealing(SourceCombatant->Stats.HpMax * Effect.Value);
		}
		else if (Effect.BehaviorId == TEXT("Part.ApplyBleedOnCritical") && Result.bCritical)
		{
			ApplyBleeding(Effect);
		}
		else if (Effect.BehaviorId == TEXT("Part.ApplyBleedOnHitChance") &&
		         DeterministicRoll(Effect, Effect.ParamValue))
		{
			ApplyBleeding(Effect);
		}
		else if (Effect.BehaviorId == TEXT("Part.StunOnHit") && DeterministicRoll(Effect, Effect.Value))
		{
			ApplyStun(Effect);
		}
		else if (Effect.BehaviorId == TEXT("Part.BleedEveryTargetHits"))
		{
			int32& Count = PersistentTargetHitCounters.FindOrAdd(Effect.PartId).FindOrAdd(Result.Target);
			++Count;
			const int32 Threshold = FMath::Max(1, FMath::RoundToInt(Effect.ParamValue));
			if (Count >= Threshold)
			{
				Count -= Threshold;
				ApplyBleeding(Effect);
			}
		}
		else if (Effect.BehaviorId == TEXT("Part.MoveSpeedPerHit") && SourceCombatant)
		{
			SourceCombatant->AddTransientStatModifier(
			    Effect.PartId,
			    0.0f,
			    Effect.Value,
			    Effect.DurationSeconds,
			    ReEchoWeaponVisual::ResolveConfiguredMaxStacks(Effect.ParamValue));
		}
		else if (Effect.BehaviorId == TEXT("Part.AttackSpeedPerHit") && SourceCombatant)
		{
			SourceCombatant->AddTransientStatModifier(
			    Effect.PartId,
			    Effect.Value,
			    0.0f,
			    Effect.DurationSeconds,
			    ReEchoWeaponVisual::ResolveConfiguredMaxStacks(Effect.ParamValue));
		}
		else if (Effect.BehaviorId == TEXT("Part.DropShardEveryHits") &&
		         Result.DamageSource == EReEchoDamageSource::Player)
		{
			int32& Count = PersistentRuneHitCounters.FindOrAdd(Effect.PartId);
			++Count;
			const int32 Threshold = FMath::Max(1, FMath::RoundToInt(Effect.ParamValue));
			if (Count >= Threshold)
			{
				Count -= Threshold;
				SpawnTimeShardPickup(Result.HitLocation, FMath::Max(1, FMath::RoundToInt(Effect.Value)));
			}
		}
		if (Result.bKilled && Effect.BehaviorId == TEXT("Part.DropShardOnKill") &&
		    Result.DamageSource == EReEchoDamageSource::Player)
		{
			SpawnTimeShardPickup(Result.HitLocation, FMath::Max(1, FMath::RoundToInt(Effect.Value)));
		}
		else if (Result.bKilled && Effect.BehaviorId == TEXT("Part.MoveSpeedOnKill") && SourceCombatant)
		{
			const int32 MaxStacks = Effect.ParamName == TEXT("MaxStacks")
			                            ? ReEchoWeaponVisual::ResolveConfiguredMaxStacks(Effect.ParamValue)
			                            : TNumericLimits<int32>::Max();
			SourceCombatant->AddTransientStatModifier(
			    Effect.PartId, 0.0f, Effect.Value, Effect.DurationSeconds, MaxStacks);
		}
	}
}

void AReEchoWeaponActor::ProcessAttackResolved(const TSharedPtr<FReEchoWeaponRuneAttackContext>& Context)
{
	if (!Context.IsValid() || Context->bAttackResolved)
	{
		return;
	}
	Context->bAttackResolved = true;
	const float WorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	bool bHeldVisualRangeChanged = false;
	for (const FReEchoWeaponRuneEffectSpec& Effect : Context->Effects)
	{
		if (Effect.ParamName != TEXT("GroupThreshold") ||
		    Context->EffectiveHitTargets.Num() < FMath::RoundToInt(Effect.ParamValue))
		{
			continue;
		}
		if (Effect.BehaviorId == TEXT("Part.RangeOnGroupHit"))
		{
			FTimedRangeStack& Stack = TimedRangeStacks.AddDefaulted_GetRef();
			Stack.SourceId = Effect.PartId;
			Stack.BonusFraction = Effect.Value;
			Stack.ExpiresAt = WorldTime + Effect.DurationSeconds;
			bHeldVisualRangeChanged = true;
		}
		else if (Effect.BehaviorId == TEXT("Part.InvulnerableOnGroupHit"))
		{
			if (UReEchoCombatantComponent* SourceCombatant = Context->SourceCombatant.Get())
			{
				SourceCombatant->GrantTimedInvulnerability(WorldTime, Effect.DurationSeconds);
			}
		}
		else if (Effect.BehaviorId == TEXT("Part.MeteorOnGroupHit"))
		{
			AActor* WeaponOwner = GetOwner();
			AActor* CenterTarget = nullptr;
			float BestDistanceSquared = TNumericLimits<float>::Max();
			for (const TWeakObjectPtr<AActor>& Target : Context->EffectiveHitTargets)
			{
				if (Target.IsValid() && WeaponOwner)
				{
					const float DistanceSquared =
					    FVector::DistSquared2D(WeaponOwner->GetActorLocation(), Target->GetActorLocation());
					if (DistanceSquared < BestDistanceSquared)
					{
						BestDistanceSquared = DistanceSquared;
						CenterTarget = Target.Get();
					}
				}
			}
			if (!CenterTarget)
			{
				continue;
			}
			const float Radius = GetRuneParam(Effect.BehaviorId, TEXT("ExplosionRadiusCm"), 150.0f);
			for (TActorIterator<AActor> It(GetWorld()); It; ++It)
			{
				const IReEchoCombatTarget* Target = Cast<IReEchoCombatTarget>(*It);
				if (Target &&
				    FVector::Dist2D(CenterTarget->GetActorLocation(), Target->GetCombatTargetLocation()) <= Radius)
				{
					ApplyDamageToTarget(**It,
					                    Context->Commit,
					                    CenterTarget->GetActorLocation(),
					                    Context->SourceCombatant.Get(),
					                    Context,
					                    Effect.Value,
					                    false);
				}
			}
		}
	}
	if (bHeldVisualRangeChanged)
	{
		RefreshHeldPresentation();
	}
}

void AReEchoWeaponActor::SpawnTimeShardPickup(const FVector& Location, const int32 Amount) const
{
	if (!GetWorld() || Amount <= 0)
	{
		return;
	}
	if (AReEchoGameMode* GameMode = GetWorld()->GetAuthGameMode<AReEchoGameMode>())
	{
		GameMode->SpawnTimeShardPickup(Location, Amount);
		return;
	}
	// Headless component tests intentionally have no GameMode; preserve the native fallback there.
	if (AReEchoTimeShardPickupActor* Pickup =
	        GetWorld()->SpawnActor<AReEchoTimeShardPickupActor>(Location, FRotator::ZeroRotator))
	{
		Pickup->InitializePickup(Amount);
	}
}

FReEchoHitResolved AReEchoWeaponActor::ApplyDamageToTarget(AActor& Target,
                                                           const FReEchoWeaponAttackCommit& Commit,
                                                           const FVector& DamageSource,
                                                           UReEchoCombatantComponent* Combatant,
                                                           const TSharedPtr<FReEchoWeaponRuneAttackContext>& Context,
                                                           const float DamageMultiplier,
                                                           const bool bProcessRuneEffects)
{
	const IReEchoCombatTarget* CombatTarget = Cast<IReEchoCombatTarget>(&Target);
	UReEchoCombatantComponent* TargetCombatant = CombatTarget ? CombatTarget->GetCombatTargetCombatant() : nullptr;
	if (!TargetCombatant || !TargetCombatant->IsAlive())
	{
		return {};
	}
	FReEchoHitIntent Intent;
	Intent.Attack = Commit.Attack;
	Intent.Target = &Target;
	const FVector TargetLocation = CombatTarget->GetCombatTargetLocation();
	const float DistanceToTarget = FVector::Dist2D(DamageSource, TargetLocation);
	float ResolvedDamageMultiplier = DamageMultiplier;
	if (Commit.RangeCm > 0.0f && Commit.OuterRingStartFraction > 0.0f && Commit.OuterRingBonusMultiplier > 0.0f)
	{
		const float StartFraction = FMath::Clamp(Commit.OuterRingStartFraction, 0.0f, 1.0f);
		if (DistanceToTarget >= Commit.RangeCm * StartFraction)
		{
			ResolvedDamageMultiplier *= 1.0f + Commit.OuterRingBonusMultiplier;
		}
	}
	if (Context.IsValid())
	{
		for (const FReEchoWeaponRuneEffectSpec& Effect : Context->Effects)
		{
			if (Effect.BehaviorId == TEXT("Part.OuterRingDamage") && Commit.RangeCm > 0.0f)
			{
				const float OuterFraction = FMath::Clamp(Effect.ParamValue, 0.0f, 1.0f);
				if (DistanceToTarget >= Commit.RangeCm * (1.0f - OuterFraction))
				{
					ResolvedDamageMultiplier *= 1.0f + Effect.Value;
				}
			}
		}
	}
	Intent.RawDamage = Commit.RawDamage * ResolvedDamageMultiplier;
	Intent.DamageSource = ResolveOwnerDamageSource();
	Intent.Element = Commit.Element;
	Intent.ReactionEfficiency = Combatant ? Combatant->Stats.ReactionEfficiency : 1.0f;
	Intent.bCritical = Commit.bCritical;
	Intent.CriticalMultiplier = Commit.CriticalMultiplier;
	Intent.SourceLocation = DamageSource;
	Intent.HitLocation = Target.GetActorLocation();
	const float TargetHealthBefore = TargetCombatant->GetSnapshot().CurrentHealth;
	const FReEchoHitResolved Result = ReEchoHitResolver::ResolveHit(Intent);
#if !UE_BUILD_SHIPPING
	if (Commit.WeaponId == ReEchoSwordDamageTrace::LongSwordWeaponId)
	{
		UE_LOG(
		    LogReEcho,
		    Warning,
		    TEXT(
		        "[SwordDamageTrace] Hit owner=%s target=%s step=%s[%d] commitRaw=%.3f localMultiplier=%.3f "
		        "intentRaw=%.3f resolvedRaw=%.3f applied=%.3f healthBefore=%.3f healthAfter=%.3f blocked=%d killed=%d"),
		    *GetNameSafe(GetOwner()),
		    *GetNameSafe(&Target),
		    *Commit.AttackStepId.ToString(),
		    Commit.StepIndex,
		    Commit.RawDamage,
		    ResolvedDamageMultiplier,
		    Intent.RawDamage,
		    Result.RawDamage,
		    Result.AppliedDamage,
		    TargetHealthBefore,
		    TargetCombatant->GetSnapshot().CurrentHealth,
		    Result.bBlocked ? 1 : 0,
		    Result.bKilled ? 1 : 0);
	}
#endif
	if (Result.bKilled && Combatant && WeaponLogic.GetOnKillHealPercent() > 0.0f)
	{
		Combatant->ApplyHealing(Combatant->Stats.HpMax * WeaponLogic.GetOnKillHealPercent());
	}
	if (bProcessRuneEffects)
	{
		ProcessResolvedHit(Context, Result);
	}
	return Result;
}

bool AReEchoWeaponActor::FireStaffLightWave(const FReEchoWeaponAttackCommit& Commit,
                                            UReEchoCombatantComponent* Combatant)
{
	AActor* WeaponOwner = GetOwner();
	if (!WeaponOwner)
	{
		return false;
	}
	const FVector OwnerLocation = WeaponOwner->GetActorLocation();
	const FVector AimDirection = ResolveOwnerAimDirection();
	// 固定相机的屏幕上方向；从法杖 Billboard 中心偏移到月牙水晶杖头。
	const FVector CameraUp(0.8192f, 0.0f, 0.5736f);
	const FVector StaffHeadLocation = StaffSprite->GetComponentLocation() + CameraUp * 45.0f;
	const FVector SpawnLocation = StaffHeadLocation + AimDirection * 18.0f;
	AReEchoStaffLightWaveActor* Wave =
	    GetWorld()->SpawnActor<AReEchoStaffLightWaveActor>(SpawnLocation, FRotator::ZeroRotator);
	if (!Wave)
	{
		return false;
	}
	Wave->SetOwner(WeaponOwner);
	Wave->InitializeWave(
	    AimDirection, Commit.RawDamage, OwnerLocation, Commit.RangeCm, Commit.Attack, ResolveOwnerDamageSource());
	return true;
}

bool AReEchoWeaponActor::FireProjectile(const FReEchoWeaponAttackCommit& Commit,
                                        UReEchoCombatantComponent* Combatant,
                                        const TSharedPtr<FReEchoWeaponRuneAttackContext>& Context)
{
	AActor* WeaponOwner = GetOwner();
	if (!WeaponOwner)
	{
		return false;
	}
	const FVector OwnerLocation = WeaponOwner->GetActorLocation();
	const FVector AimDirection = ResolveOwnerAimDirection();
	const TArray<FVector> Directions =
	    ReEchoWeaponRuntime::BuildProjectileDirections(AimDirection, Commit.ProjectileCount, Commit.SpreadDegrees);
	bool bSpawnedAny = false;
	const bool bPierceOnCritical =
	    Context.IsValid() && Context->Effects.ContainsByPredicate(
	                             [](const FReEchoWeaponRuneEffectSpec& Effect)
	                             {
		                             return Effect.BehaviorId == TEXT("Part.ProjectilePierceOnCritical");
	                             });
#if !UE_BUILD_SHIPPING
	const bool bTraceRangedCrit =
	    Context.IsValid() && ReEchoRangedCritTrace::ShouldTrace(Commit.WeaponId, Context->Effects);
	if (bTraceRangedCrit)
	{
		UE_LOG(LogReEcho,
		       Warning,
		       TEXT("[RangedCritTrace] Fire owner=%s weapon=%s sequence=%lld directions=%d rawDamage=%.3f "
		            "critical=%d pierceOnCritical=%d source=%d reactionEfficiency=%.3f"),
		       *GetNameSafe(WeaponOwner),
		       *Commit.WeaponId.ToString(),
		       static_cast<long long>(Commit.Attack.Sequence),
		       Directions.Num(),
		       Commit.RawDamage,
		       Commit.bCritical ? 1 : 0,
		       bPierceOnCritical ? 1 : 0,
		       static_cast<int32>(Context->DamageSource),
		       Context->ReactionEfficiency);
	}
#endif
	for (int32 ProjectileIndex = 0; ProjectileIndex < Directions.Num(); ++ProjectileIndex)
	{
		const FVector& Direction = Directions[ProjectileIndex];
		EReEchoElement DebugElementOverride = EReEchoElement::None;
#if !UE_BUILD_SHIPPING
		if (const AReEchoPlayerPawn* PlayerOwner = Cast<AReEchoPlayerPawn>(WeaponOwner))
		{
			DebugElementOverride = PlayerOwner->GetDebugOutgoingElementOverride();
		}
#endif
		const EReEchoElement ProjectileElement =
		    ReEchoWeaponVisual::ResolveProjectileElement(EffectiveDefinition.bUsesDeterministicRandomElement,
		                                                 Commit.Element,
		                                                 Commit.Attack.Sequence,
		                                                 ProjectileIndex,
		                                                 DebugElementOverride);
		const bool bHasWeaponAnchor = IsValid(WeaponAttackVfxRoot);
		const FVector WeaponAnchorLocation =
		    bHasWeaponAnchor ? WeaponAttackVfxRoot->GetComponentLocation() : FVector::ZeroVector;
		const FVector SpawnLocation = ReEchoWeaponVisual::ResolveProjectileSpawnLocation(
		    WeaponAnchorLocation, OwnerLocation, Direction, bHasWeaponAnchor);
		AReEchoProjectileActor* Projectile =
		    GetWorld()->SpawnActor<AReEchoProjectileActor>(SpawnLocation, Direction.Rotation());
		if (!Projectile)
		{
#if !UE_BUILD_SHIPPING
			if (bTraceRangedCrit)
			{
				UE_LOG(LogReEcho,
				       Warning,
				       TEXT("[RangedCritTrace] ProjectileSpawnFailed owner=%s weapon=%s sequence=%lld location=%s"),
				       *GetNameSafe(WeaponOwner),
				       *Commit.WeaponId.ToString(),
				       static_cast<long long>(Commit.Attack.Sequence),
				       *SpawnLocation.ToCompactString());
			}
#endif
			continue;
		}
		Projectile->SetOwner(WeaponOwner);
		Projectile->InitializeProjectile(Direction,
		                                 Commit.RawDamage,
		                                 OwnerLocation,
		                                 ReEchoElementReaction::GetElementColor(ProjectileElement),
		                                 ProjectileElement,
		                                 Context.IsValid() ? Context->ReactionEfficiency
		                                                   : Combatant->Stats.ReactionEfficiency,
		                                 Commit.ExplosionRadiusCm,
		                                 Commit.RangeCm,
		                                 Commit.Attack,
		                                 Commit.bCritical,
		                                 Context.IsValid() ? Context->DamageSource : ResolveOwnerDamageSource(),
		                                 Context.IsValid() ? Context->WeaponVisualKey : GetEquippedWeaponVisualKey(),
		                                 bPierceOnCritical,
		                                 this,
		                                 Context,
		                                 true);
#if !UE_BUILD_SHIPPING
		if (bTraceRangedCrit)
		{
			UE_LOG(LogReEcho,
			       Warning,
			       TEXT("[RangedCritTrace] ProjectileSpawned owner=%s projectile=%s weapon=%s sequence=%lld "
			            "location=%s direction=%s pendingKill=%d"),
			       *GetNameSafe(WeaponOwner),
			       *GetNameSafe(Projectile),
			       *Commit.WeaponId.ToString(),
			       static_cast<long long>(Commit.Attack.Sequence),
			       *SpawnLocation.ToCompactString(),
			       *Direction.ToCompactString(),
			       Projectile->IsActorBeingDestroyed() ? 1 : 0);
		}
#endif
		bSpawnedAny = true;
	}
	return bSpawnedAny;
}

void AReEchoWeaponActor::HandleProjectileResolved(const TSharedPtr<FReEchoWeaponRuneAttackContext>& Context,
                                                  const FReEchoProjectileSnapshot& Snapshot,
                                                  const FReEchoHitResolved& Result,
                                                  const bool bAllowSplit)
{
#if !UE_BUILD_SHIPPING
	if (Context.IsValid() && ReEchoRangedCritTrace::ShouldTrace(Context->Commit.WeaponId, Context->Effects))
	{
		UE_LOG(LogReEcho,
		       Warning,
		       TEXT("[RangedCritTrace] ProjectileResolved owner=%s weapon=%s sequence=%lld projectile=%s target=%s "
		            "commitRaw=%.3f commitCritical=%d resolvedRaw=%.3f applied=%.3f resultCritical=%d blocked=%d "
		            "killed=%d allowSplit=%d"),
		       *GetNameSafe(GetOwner()),
		       *Context->Commit.WeaponId.ToString(),
		       static_cast<long long>(Context->Commit.Attack.Sequence),
		       *Snapshot.ProjectileId.Value.ToString(EGuidFormats::DigitsWithHyphensLower),
		       *GetNameSafe(Result.Target),
		       Context->Commit.RawDamage,
		       Context->Commit.bCritical ? 1 : 0,
		       Result.RawDamage,
		       Result.AppliedDamage,
		       Result.bCritical ? 1 : 0,
		       Result.bBlocked ? 1 : 0,
		       Result.bKilled ? 1 : 0,
		       bAllowSplit ? 1 : 0);
	}
#endif
	if (!Context.IsValid())
	{
		return;
	}
	if (Result.bKilled && Context->SourceCombatant.IsValid() && Context->OnKillHealPercent > 0.0f)
	{
		UReEchoCombatantComponent* SourceCombatant = Context->SourceCombatant.Get();
		SourceCombatant->ApplyHealing(SourceCombatant->Stats.HpMax * Context->OnKillHealPercent);
	}
	ProcessResolvedHit(Context, Result);
	if (!bAllowSplit || Result.AppliedDamage <= 0.0f)
	{
		return;
	}
	for (const FReEchoWeaponRuneEffectSpec& Effect : Context->Effects)
	{
		if (Effect.BehaviorId == TEXT("Part.ProjectileSplitOnHit"))
		{
			SpawnSplitProjectiles(Context, Snapshot, Result, Effect);
			break;
		}
	}
}

void AReEchoWeaponActor::SpawnSplitProjectiles(const TSharedPtr<FReEchoWeaponRuneAttackContext>& Context,
                                               const FReEchoProjectileSnapshot& Snapshot,
                                               const FReEchoHitResolved& Result,
                                               const FReEchoWeaponRuneEffectSpec& Effect)
{
	if (!GetWorld() || !Result.Target)
	{
		return;
	}
	const uint32 ProjectileHash = GetTypeHash(Snapshot.ProjectileId.Value);
	if (Context->SplitProjectileHashes.Contains(ProjectileHash))
	{
		return;
	}
	Context->SplitProjectileHashes.Add(ProjectileHash);
	TArray<AActor*> Candidates;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		IReEchoCombatTarget* Target = Cast<IReEchoCombatTarget>(*It);
		if (*It != Result.Target && Target && Target->IsCombatTargetAlive() &&
		    ReEchoCombatRelations::CanDamage(Context->Commit.Attack, **It, false) &&
		    FVector::Dist2D(Result.HitLocation, Target->GetCombatTargetLocation()) <= Context->Commit.RangeCm)
		{
			Candidates.Add(*It);
		}
	}
	Candidates.Sort(
	    [&Result](const AActor& Left, const AActor& Right)
	    {
		    const float LeftDistance = FVector::DistSquared2D(Result.HitLocation, Left.GetActorLocation());
		    const float RightDistance = FVector::DistSquared2D(Result.HitLocation, Right.GetActorLocation());
		    if (!FMath::IsNearlyEqual(LeftDistance, RightDistance))
		    {
			    return LeftDistance < RightDistance;
		    }
		    const IReEchoCombatTarget* LeftTarget = Cast<IReEchoCombatTarget>(&Left);
		    const IReEchoCombatTarget* RightTarget = Cast<IReEchoCombatTarget>(&Right);
		    return LeftTarget && RightTarget &&
		           LeftTarget->GetCombatTargetTieBreakIndex() < RightTarget->GetCombatTargetTieBreakIndex();
	    });
	const int32 ChildCount = FMath::Min(Candidates.Num(), FMath::Max(0, FMath::RoundToInt(Effect.ParamValue)));
#if !UE_BUILD_SHIPPING
	UE_LOG(LogReEcho,
	       Log,
	       TEXT("[SplitArrowTrace] ParentImpact parentProjectile=%s attackSequence=%lld parentTarget='%s' "
	            "hit=(%.2f,%.2f,%.2f) candidateCount=%d childCount=%d range=%.2f"),
	       *Snapshot.ProjectileId.Value.ToString(EGuidFormats::DigitsWithHyphensLower),
	       static_cast<long long>(Context->Commit.Attack.Sequence),
	       *GetNameSafe(Result.Target),
	       Result.HitLocation.X,
	       Result.HitLocation.Y,
	       Result.HitLocation.Z,
	       Candidates.Num(),
	       ChildCount,
	       Context->Commit.RangeCm);
	for (int32 CandidateIndex = 0; CandidateIndex < Candidates.Num(); ++CandidateIndex)
	{
		const IReEchoCombatTarget* CandidateTarget = Cast<IReEchoCombatTarget>(Candidates[CandidateIndex]);
		const FVector CandidateLocation = CandidateTarget ? CandidateTarget->GetCombatTargetLocation()
		                                                  : Candidates[CandidateIndex]->GetActorLocation();
		UE_LOG(LogReEcho,
		       Log,
		       TEXT("[SplitArrowTrace] Candidate parentProjectile=%s rank=%d selected=%d actor='%s' "
		            "location=(%.2f,%.2f,%.2f) distanceFromHit2D=%.2f"),
		       *Snapshot.ProjectileId.Value.ToString(EGuidFormats::DigitsWithHyphensLower),
		       CandidateIndex,
		       CandidateIndex < ChildCount ? 1 : 0,
		       *GetNameSafe(Candidates[CandidateIndex]),
		       CandidateLocation.X,
		       CandidateLocation.Y,
		       CandidateLocation.Z,
		       FVector::Dist2D(Result.HitLocation, CandidateLocation));
	}
#endif
	const bool bPierceOnCritical = Context->Effects.ContainsByPredicate(
	    [](const FReEchoWeaponRuneEffectSpec& RuneEffect)
	    {
		    return RuneEffect.BehaviorId == TEXT("Part.ProjectilePierceOnCritical");
	    });
	for (int32 Index = 0; Index < ChildCount; ++Index)
	{
		const FVector Direction = (Candidates[Index]->GetActorLocation() - Result.HitLocation).GetSafeNormal2D();
		const FVector SpawnLocation = Result.HitLocation + FVector(0.0f, 0.0f, 20.0f) + Direction * 18.0f;
		AReEchoProjectileActor* Projectile =
		    GetWorld()->SpawnActor<AReEchoProjectileActor>(SpawnLocation, Direction.Rotation());
		if (!Projectile)
		{
			continue;
		}
		Projectile->SetOwner(GetOwner());
		Projectile->InitializeProjectile(Direction,
		                                 Context->Commit.RawDamage * Effect.Value,
		                                 Result.HitLocation,
		                                 ReEchoElementReaction::GetElementColor(Context->Commit.Element),
		                                 Context->Commit.Element,
		                                 Context->ReactionEfficiency,
		                                 Context->Commit.ExplosionRadiusCm,
		                                 Context->Commit.RangeCm,
		                                 Context->Commit.Attack,
		                                 Context->Commit.bCritical,
		                                 Context->DamageSource,
		                                 Context->WeaponVisualKey,
		                                 bPierceOnCritical,
		                                 this,
		                                 Context,
		                                 false,
		                                 Result.Target);
#if !UE_BUILD_SHIPPING
		if (IsValid(Projectile))
		{
			Projectile->ConfigureSplitDiagnostics(
			    Snapshot.ProjectileId.Value, Index, Result.Target, Candidates[Index]);
		}
#endif
	}
}

void AReEchoWeaponActor::UpdateElementIndicator()
{
	if (!ElementIndicator)
	{
		return;
	}
	const EReEchoElement Element = WeaponLogic.GetSnapshot().NextElement;
	ElementIndicator->SetText(
	    FText::FromString(FString::Printf(TEXT("NEXT: %s"), *ReEchoElementReaction::GetElementLabel(Element))));
	ElementIndicator->SetTextRenderColor(ReEchoElementReaction::GetElementColor(Element).ToFColor(false));
}

void AReEchoWeaponActor::RefreshVisualState()
{
	const FReEchoCsvWeaponRow* Definition = FindEquippedDefinition();
	const FName VisualKey = Definition ? Definition->VisualKey : NAME_None;
	const bool bShowSword = VisualKey == TEXT("CrescentBlade");
	const bool bShowElement = VisualKey == TEXT("ElementalOrb");
	const bool bShowStaff = VisualKey == TEXT("MoonStaff");
	const bool bShowScythe = VisualKey == TEXT("Scythe");
	const bool bShowBow = VisualKey == TEXT("Bow");
	const bool bShowGun = VisualKey == TEXT("Gun");
	if (StaffSprite)
	{
		StaffSprite->SetVisibility(bShowStaff);
		StaffSprite->SetHiddenInGame(!bShowStaff);
	}
	if (SwordSprite)
	{
		SwordSprite->SetVisibility(bShowSword);
	}
	if (ElementIndicator)
	{
		ElementIndicator->SetVisibility(bShowElement);
	}
	if (ScytheSprite)
	{
		ScytheSprite->SetVisibility(bShowScythe);
		ScytheSprite->SetHiddenInGame(!bShowScythe);
	}
	if (BowSprite)
	{
		BowSprite->SetVisibility(bShowBow);
		BowSprite->SetHiddenInGame(!bShowBow);
	}
	if (GunSprite)
	{
		GunSprite->SetVisibility(bShowGun);
		GunSprite->SetHiddenInGame(!bShowGun);
	}
}

void AReEchoWeaponActor::RefreshHeldPresentation()
{
	const FReEchoCsvWeaponRow* Definition = FindEquippedDefinition();
	const FName VisualKey = Definition ? Definition->VisualKey : NAME_None;
	const UReEchoWeaponPresentationProfile* WeaponProfile = FReEchoWeaponVisualCatalog::ResolveProfile(VisualKey);
	if (!WeaponProfile)
	{
		return;
	}

	const UReEcho2DCharacterPresentationProfile* CharacterProfile = HeldCharacterProfile.Get();
	const float CharacterReferenceHeight = CharacterProfile ? FMath::Max(CharacterProfile->WorldHeight, 1.0f) : 100.0f;
	const float OwnerScale = GetOwner() ? FMath::Max(FMath::Abs(GetOwner()->GetActorScale3D().Z), 0.01f) : 1.0f;
	const float CharacterWorldHeight = CharacterReferenceHeight * OwnerScale;
	const UReEchoWeaponPresentationCatalog* WeaponCatalog = FReEchoWeaponVisualCatalog::ResolveCatalog();
	const FVector RightAnchorRatio =
	    WeaponCatalog ? WeaponCatalog->RightHandAnchorRatio : ReEchoWeaponVisual::DefaultWeaponAnchorRatio;
	const FVector LeftAnchorRatio =
	    WeaponCatalog ? WeaponCatalog->LeftHandAnchorRatio : ReEchoWeaponVisual::DefaultLeftWeaponAnchorRatio;
	RightWeaponHandAnchorLocation = RightAnchorRatio * CharacterReferenceHeight;
	LeftWeaponHandAnchorLocation = LeftAnchorRatio * CharacterReferenceHeight;
	WeaponHandAnchorLocation =
	    ResolveOwnerVisualFacingSign() < 0.0f ? LeftWeaponHandAnchorLocation : RightWeaponHandAnchorLocation;
	SetActorRelativeLocation(WeaponHandAnchorLocation);

	UTexture2D* Texture = WeaponProfile->HeldTexture.LoadSynchronous();

	UBillboardComponent* Billboard = nullptr;
	if (VisualKey == TEXT("MoonStaff"))
	{
		Billboard = StaffSprite;
	}

	HeldVisualOffset = WeaponProfile->HeldOffsetRatio * CharacterWorldHeight;
	const FVector VisualOffset = ResolveMirroredHeldVisualOffset();
	const float AttackRangeMultiplier = ResolveCurrentAttackRangeMultiplier();
	if (Billboard && Texture)
	{
		const float TextureAxisLength = WeaponProfile->HeldSizeAxis == EReEchoHeldWeaponSizeAxis::Width
		                                    ? FMath::Max(Texture->GetSizeX(), 1)
		                                    : FMath::Max(Texture->GetSizeY(), 1);
		const float UniformScale =
		    ReEchoWeaponVisual::ResolveHeldLength(*WeaponProfile, CharacterWorldHeight) / TextureAxisLength;
		Billboard->SetRelativeLocation(VisualOffset);
		Billboard->SetRelativeRotation(WeaponProfile->HeldRotationOffset);
		Billboard->SetRelativeScale3D(FVector(UniformScale));
	}

	UStaticMeshComponent* WeaponPlane = VisualKey == TEXT("Scythe") ? ScytheSprite.Get()
	                                    : VisualKey == TEXT("Bow")  ? BowSprite.Get()
	                                    : VisualKey == TEXT("Gun")  ? GunSprite.Get()
	                                                                : nullptr;
	if (WeaponPlane && Texture)
	{
		const FVector2D Dimensions =
		    ReEchoWeaponVisual::ResolveHeldDimensions(*Texture, *WeaponProfile, CharacterWorldHeight);
		WeaponPlane->SetRelativeLocation(VisualOffset);
		WeaponPlane->SetRelativeRotation(WeaponProfile->HeldRotationOffset.Quaternion() *
		                                 ReEchoWeaponVisual::GetSwordRotation());
		const FVector AuthoredScale(Dimensions.X / 100.0f, Dimensions.Y / 100.0f, 1.0f);
		WeaponPlane->SetRelativeScale3D(WeaponProfile->bScaleHeldVisualWithAttackRange
		                                    ? ReEchoWeaponVisual::ResolveHeldVisualAttackRangeScale(
		                                          AuthoredScale,
		                                          WeaponProfile->HeldVisualAttackRangeScaleMask,
		                                          AttackRangeMultiplier,
		                                          WeaponProfile->MinHeldVisualAttackRangeMultiplier,
		                                          WeaponProfile->MaxHeldVisualAttackRangeMultiplier)
		                                    : AuthoredScale);
		ApplyHeldPlaneMirror(WeaponPlane, WeaponProfile->HeldMirrorRule);
	}

	if (SwordSprite && VisualKey == TEXT("CrescentBlade") && Texture)
	{
		const FVector2D Dimensions =
		    ReEchoWeaponVisual::ResolveHeldDimensions(*Texture, *WeaponProfile, CharacterWorldHeight);
		SwordSpriteRestLocation = VisualOffset;
		SwordPlanarAngleOffsetRadians = FMath::DegreesToRadians(WeaponProfile->HeldPlanarAngleOffsetDegrees);
		SwordAuthoredRotation = WeaponProfile->HeldRotationOffset.Quaternion();
		SwordSpriteRestRotation = ResolveMirroredSwordRestRotation();
		SwordSprite->SetRelativeLocation(SwordSpriteRestLocation);
		SwordSprite->SetRelativeRotation(SwordSpriteRestRotation);
		const FVector AuthoredScale(Dimensions.X / 100.0f, Dimensions.Y / 100.0f, 1.0f);
		SwordSprite->SetRelativeScale3D(WeaponProfile->bScaleHeldVisualWithAttackRange
		                                    ? ReEchoWeaponVisual::ResolveHeldVisualAttackRangeScale(
		                                          AuthoredScale,
		                                          WeaponProfile->HeldVisualAttackRangeScaleMask,
		                                          AttackRangeMultiplier,
		                                          WeaponProfile->MinHeldVisualAttackRangeMultiplier,
		                                          WeaponProfile->MaxHeldVisualAttackRangeMultiplier)
		                                    : AuthoredScale);
	}
	RefreshWeaponAttackVfxRoot(*WeaponProfile);
}

void AReEchoWeaponActor::RefreshWeaponAttackVfxRoot(const UReEchoWeaponPresentationProfile& WeaponProfile)
{
	if (!WeaponAttackVfxRoot)
	{
		return;
	}

	USceneComponent* WeaponVisual = nullptr;
	if (WeaponProfile.WeaponVisualKey == TEXT("CrescentBlade"))
	{
		WeaponVisual = SwordSprite;
	}
	else if (WeaponProfile.WeaponVisualKey == TEXT("Scythe"))
	{
		WeaponVisual = ScytheSprite;
	}
	else if (WeaponProfile.WeaponVisualKey == TEXT("Bow"))
	{
		WeaponVisual = BowSprite;
	}
	else if (WeaponProfile.WeaponVisualKey == TEXT("Gun"))
	{
		WeaponVisual = GunSprite;
	}
	if (!WeaponVisual)
	{
		WeaponAttackVfxRoot->AttachToComponent(Root, FAttachmentTransformRules::SnapToTargetIncludingScale);
		WeaponAttackVfxRoot->SetRelativeTransform(FTransform::Identity);
		return;
	}

	const bool bVisualHorizontallyMirrored = WeaponVisual->GetRelativeScale3D().X < 0.0f;
	if (WeaponProfile.WeaponVisualKey == TEXT("Gun") && GunSprite && GunSprite->GetStaticMesh())
	{
		const float VerticalRatio =
		    WeaponProfile.bOverrideAttackVfxAnchor ? WeaponProfile.AttackVfxAnchorRatio.Y : 0.0f;
		const FVector MuzzleLocalPoint =
		    ReEchoWeaponVisual::ResolveGunMuzzleLocalPoint(GunSprite->GetStaticMesh()->GetBoundingBox(),
		                                                   ResolveOwnerVisualFacingSign(),
		                                                   bVisualHorizontallyMirrored,
		                                                   VerticalRatio);
		if (WeaponAttackVfxRoot->GetAttachParent() != WeaponVisual)
		{
			WeaponAttackVfxRoot->AttachToComponent(WeaponVisual, FAttachmentTransformRules::SnapToTargetIncludingScale);
		}
		WeaponAttackVfxRoot->SetRelativeTransform(FTransform(FQuat::Identity, MuzzleLocalPoint, FVector::OneVector));
		return;
	}
	const FVector2D AnchorRatio = ReEchoWeaponVisual::ResolveAttackVfxAnchorComponentRatio(
	    WeaponProfile, ResolveOwnerVisualFacingSign(), bVisualHorizontallyMirrored);
	if (WeaponAttackVfxRoot->GetAttachParent() != WeaponVisual)
	{
		WeaponAttackVfxRoot->AttachToComponent(WeaponVisual, FAttachmentTransformRules::SnapToTargetIncludingScale);
	}
	WeaponAttackVfxRoot->SetRelativeTransform(FTransform(FQuat::Identity,
	                                                     FVector(AnchorRatio.X * ReEchoWeaponVisual::WeaponPlaneSizeCm,
	                                                             AnchorRatio.Y * ReEchoWeaponVisual::WeaponPlaneSizeCm,
	                                                             0.0f),
	                                                     FVector::OneVector));
}

#if WITH_DEV_AUTOMATION_TESTS
float AReEchoWeaponActor::ResolveHeldWorldLengthForTests(const UReEchoWeaponPresentationProfile& WeaponProfile,
                                                         const float CharacterReferenceHeight,
                                                         const float OwnerScale)
{
	return ReEchoWeaponVisual::ResolveHeldLength(
	    WeaponProfile, FMath::Max(CharacterReferenceHeight, 1.0f) * FMath::Max(FMath::Abs(OwnerScale), 0.01f));
}

FVector AReEchoWeaponActor::ResolveFacingHeldOffsetForTests(const FVector& HeldOffset,
                                                            const float FacingSign,
                                                            const FVector& CameraRight)
{
	return ReEchoWeaponVisual::ResolveFacingPointAroundCenter(HeldOffset, FVector::ZeroVector, FacingSign, CameraRight);
}

float AReEchoWeaponActor::ResolveTripleSwingAngleForTests(const float Progress, const float DirectionSign)
{
	return ReEchoWeaponVisual::ResolveTripleSwingAngle(Progress, DirectionSign);
}

FVector2D AReEchoWeaponActor::ResolveAttackVfxAnchorRatioForTests(const UReEchoWeaponPresentationProfile& WeaponProfile,
                                                                  const float FacingSign)
{
	return ReEchoWeaponVisual::ResolveAttackVfxAnchorRatio(WeaponProfile, FacingSign);
}

FVector2D
AReEchoWeaponActor::ResolveAttackVfxAnchorComponentRatioForTests(const UReEchoWeaponPresentationProfile& WeaponProfile,
                                                                 const float FacingSign,
                                                                 const bool bVisualHorizontallyMirrored)
{
	return ReEchoWeaponVisual::ResolveAttackVfxAnchorComponentRatio(
	    WeaponProfile, FacingSign, bVisualHorizontallyMirrored);
}

FVector AReEchoWeaponActor::ResolveGunMuzzleLocalPointForTests(const FBox& LocalBounds,
                                                               const float FacingSign,
                                                               const bool bVisualHorizontallyMirrored,
                                                               const float VerticalRatio)
{
	return ReEchoWeaponVisual::ResolveGunMuzzleLocalPoint(
	    LocalBounds, FacingSign, bVisualHorizontallyMirrored, VerticalRatio);
}

FVector AReEchoWeaponActor::ResolveProjectileSpawnLocationForTests(const FVector& WeaponAnchorLocation,
                                                                   const FVector& LegacyOwnerLocation,
                                                                   const FVector& Direction,
                                                                   const bool bHasWeaponAnchor)
{
	return ReEchoWeaponVisual::ResolveProjectileSpawnLocation(
	    WeaponAnchorLocation, LegacyOwnerLocation, Direction, bHasWeaponAnchor);
}

bool AReEchoWeaponActor::CanCutRabbitProjectilesForTests(const FName AttackPatternId)
{
	return ReEchoWeaponVisual::CanCutRabbitProjectiles(AttackPatternId);
}

FVector AReEchoWeaponActor::ResolveSelfCenteredSpinRootLocationForTests(const FVector& HandAnchor,
                                                                        const FVector& VisualOffset,
                                                                        const FQuat& SpinRotation)
{
	return ReEchoWeaponVisual::ResolveSelfCenteredSpinRootLocation(HandAnchor, VisualOffset, SpinRotation);
}

FVector AReEchoWeaponActor::ResolveHeldVisualAttackRangeScaleForTests(const FVector& AuthoredScale,
                                                                      const FVector& ScaleMask,
                                                                      const float RangeMultiplier,
                                                                      const float MinMultiplier,
                                                                      const float MaxMultiplier)
{
	return ReEchoWeaponVisual::ResolveHeldVisualAttackRangeScale(
	    AuthoredScale, ScaleMask, RangeMultiplier, MinMultiplier, MaxMultiplier);
}

EReEchoElement AReEchoWeaponActor::ResolveProjectileElementForTests(const bool bUsesDeterministicRandomElement,
                                                                    const EReEchoElement AttackElement,
                                                                    const int64 AttackSequence,
                                                                    const int32 ProjectileIndex,
                                                                    const EReEchoElement DebugOverride)
{
	return ReEchoWeaponVisual::ResolveProjectileElement(
	    bUsesDeterministicRandomElement, AttackElement, AttackSequence, ProjectileIndex, DebugOverride);
}

#endif

bool AReEchoWeaponActor::SwingMelee(const FReEchoWeaponAttackCommit& Commit,
                                    UReEchoCombatantComponent* Combatant,
                                    const TSharedPtr<FReEchoWeaponRuneAttackContext>& Context)
{
	AActor* WeaponOwner = GetOwner();
	if (!WeaponOwner)
	{
		return false;
	}
	const FVector OwnerLocation = WeaponOwner->GetActorLocation();
	const FVector AimDirection = ResolveOwnerAimDirection();
	// Gameplay geometry comes from the committed step: longsword uses its forward 180-degree ground arc, while
	// scythe owns a true three-dimensional sphere centered on the same final weapon anchor as its slash VFX.
	// Presentation motion never changes this resolved hit authority.
	const bool bScytheSphere = Commit.AttackPatternId == TEXT("Pattern.ScytheSweep");
	const FVector MeleeOrigin =
	    bScytheSphere && IsValid(WeaponAttackVfxRoot) ? WeaponAttackVfxRoot->GetComponentLocation() : OwnerLocation;
	const TArray<AActor*> Targets =
	    bScytheSphere
	        ? ReEchoWeaponGeometry::FindMeleeTargetsInSphere(*GetWorld(), Commit.Attack, MeleeOrigin, Commit.RangeCm)
	        : ReEchoWeaponGeometry::FindMeleeTargets(
	              *GetWorld(), Commit.Attack, MeleeOrigin, AimDirection, Commit.RangeCm, Commit.ArcDegrees);
	for (AActor* Target : Targets)
	{
		ApplyDamageToTarget(*Target, Commit, MeleeOrigin, Combatant, Context);
	}
	if (ReEchoWeaponVisual::CanCutRabbitProjectiles(Commit.AttackPatternId))
	{
		for (TActorIterator<AReEchoEnemyActor> EnemyIt(GetWorld()); EnemyIt; ++EnemyIt)
		{
			if (bScytheSphere)
			{
				EnemyIt->DestroyRabbitProjectilesInMeleeSphere(MeleeOrigin, Commit.RangeCm);
			}
			else
			{
				EnemyIt->DestroyRabbitProjectilesInMeleeArc(
				    OwnerLocation, AimDirection, Commit.RangeCm, Commit.ArcDegrees);
			}
		}
	}
	ProcessAttackResolved(Context);
	StartMeleeAnimation(GetEquippedWeaponVisualKey());
	return true;
}

void AReEchoWeaponActor::StartMeleeAnimation(const FName WeaponVisualKey)
{
	SwordSwingDirection *= -1.0f;
	const UReEchoWeaponPresentationProfile* Profile = FReEchoWeaponVisualCatalog::ResolveProfile(WeaponVisualKey);
	SwordAnimationDuration = Profile ? FMath::Max(Profile->MotionDurationSeconds, 0.01f) : 0.18f;
	SwordAnimationTime =
	    Profile && Profile->MotionMode != EReEchoWeaponMotionMode::None ? SwordAnimationDuration : 0.0f;
}

void AReEchoWeaponActor::BeginScytheThrow(const TSharedPtr<FReEchoWeaponRuneAttackContext>& Context)
{
	if (!Context.IsValid() || !GetOwner())
	{
		return;
	}
	bScytheThrown = true;
	bScytheStationary = false;
	ScytheRuneContext = Context;
	ScytheThrowOrigin = GetOwner()->GetActorLocation();
	ScytheThrowLocation = ScytheThrowOrigin;
	ScytheThrowDirection = ResolveOwnerAimDirection().GetSafeNormal2D();
	ScytheTravelledCm = 0.0f;
	ScytheTickAccumulator = 0.0f;
	ScytheInitialHitTargets.Reset();
	if (ScytheSprite)
	{
		// The thrown scythe owns its world position, but its rotatable plane must keep inheriting FullSpin.
		ScytheSprite->SetAbsolute(true, false, false);
		ScytheSprite->SetWorldLocation(ScytheThrowLocation);
	}
}

void AReEchoWeaponActor::RecallScythe()
{
	bScytheThrown = false;
	bScytheStationary = false;
	ScytheRuneContext.Reset();
	ScytheInitialHitTargets.Reset();
	if (ScytheSprite)
	{
		ScytheSprite->SetAbsolute(false, false, false);
		ScytheSprite->SetRelativeLocation(ResolveMirroredHeldVisualOffset());
	}
}

void AReEchoWeaponActor::AdvanceScytheThrow(const float DeltaSeconds)
{
	if (!bScytheThrown || !ScytheRuneContext.IsValid() || !GetWorld())
	{
		return;
	}
	UReEchoCombatantComponent* SourceCombatant = ScytheRuneContext->SourceCombatant.Get();
	if (!SourceCombatant)
	{
		RecallScythe();
		return;
	}
	if (!bScytheStationary)
	{
		const FVector PreviousLocation = ScytheThrowLocation;
		const float MaximumRangeCm =
		    FMath::Max(1.0f, GetRuneParam(TEXT("Part.ScytheThrowRecall"), TEXT("MaxRangeCm"), 500.0f));
		const float RemainingDistance = FMath::Max(0.0f, MaximumRangeCm - ScytheTravelledCm);
		const float StepDistance = FMath::Min(RemainingDistance, 950.0f * FMath::Max(0.0f, DeltaSeconds));
		ScytheThrowLocation += ScytheThrowDirection * StepDistance;
		ScytheTravelledCm += StepDistance;
		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		{
			IReEchoCombatTarget* Target = Cast<IReEchoCombatTarget>(*It);
			if (!Target || !Target->IsCombatTargetAlive() || ScytheInitialHitTargets.Contains(*It) ||
			    !ReEchoCombatRelations::CanDamage(ScytheRuneContext->Commit.Attack, **It, false) ||
			    !Target->IntersectsCombatPath(PreviousLocation, ScytheThrowLocation, 20.0f))
			{
				continue;
			}
			ScytheInitialHitTargets.Add(*It);
			ApplyDamageToTarget(**It,
			                    ScytheRuneContext->Commit,
			                    PreviousLocation,
			                    SourceCombatant,
			                    ScytheRuneContext,
			                    GetRuneEffectValue(TEXT("Part.ScytheThrowRecall"), TEXT("MaxRangeCm"), 0.6f));
		}
		if (ScytheTravelledCm >= MaximumRangeCm)
		{
			bScytheStationary = true;
			ProcessAttackResolved(ScytheRuneContext);
		}
	}
	else
	{
		ScytheTickAccumulator += DeltaSeconds;
		const float Interval = 1.0f / FMath::Max(0.1f, SourceCombatant->Stats.AttackSpeed);
		while (ScytheTickAccumulator >= Interval)
		{
			ScytheTickAccumulator -= Interval;
			TSharedPtr<FReEchoWeaponRuneAttackContext> TickContext =
			    BuildRuneAttackContext(ScytheRuneContext->Commit, SourceCombatant);
			TickContext->Commit.RawDamage *=
			    GetRuneEffectValue(TEXT("Part.ScytheThrowRecall"), TEXT("StationaryDamageMultiplier"), 0.2f);
			TickContext->Commit.RangeCm = ScytheRuneContext->Commit.RangeCm;
			TickContext->bSecondaryEffect = true;
			for (AActor* Target : ReEchoWeaponGeometry::FindMeleeTargets(*GetWorld(),
			                                                             TickContext->Commit.Attack,
			                                                             ScytheThrowLocation,
			                                                             FVector::ForwardVector,
			                                                             TickContext->Commit.RangeCm,
			                                                             360.0f))
			{
				ApplyDamageToTarget(*Target, TickContext->Commit, ScytheThrowLocation, SourceCombatant, TickContext);
			}
			ProcessAttackResolved(TickContext);
		}
	}
	if (ScytheSprite)
	{
		ScytheSprite->SetWorldLocation(ScytheThrowLocation);
	}
}

void AReEchoWeaponActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bTransitionGameplaySuspended)
	{
		WeaponLogic.Tick(DeltaSeconds);
	}
	const float WorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	WeaponHandAnchorLocation =
	    ResolveOwnerVisualFacingSign() < 0.0f ? LeftWeaponHandAnchorLocation : RightWeaponHandAnchorLocation;
	SetActorRelativeLocation(WeaponHandAnchorLocation);
	SwordSpriteRestRotation = ResolveMirroredSwordRestRotation();
	if (const UReEchoWeaponPresentationProfile* Profile =
	        FReEchoWeaponVisualCatalog::ResolveProfile(GetEquippedWeaponVisualKey()))
	{
		const FName VisualKey = GetEquippedWeaponVisualKey();
		const FVector VisualOffset = ResolveMirroredHeldVisualOffset();
		if (VisualKey == TEXT("CrescentBlade"))
		{
			SwordSpriteRestLocation = VisualOffset;
		}
		else if (VisualKey == TEXT("Scythe") && ScytheSprite && !bScytheThrown)
		{
			ScytheSprite->SetRelativeLocation(VisualOffset);
		}
		if (UStaticMeshComponent* Plane = VisualKey == TEXT("Scythe") ? ScytheSprite.Get()
		                                  : VisualKey == TEXT("Bow")  ? BowSprite.Get()
		                                  : VisualKey == TEXT("Gun")  ? GunSprite.Get()
		                                                              : nullptr)
		{
			Plane->SetRelativeLocation(VisualOffset);
			ApplyHeldPlaneMirror(Plane, Profile->HeldMirrorRule);
		}
		RefreshWeaponAttackVfxRoot(*Profile);
	}
	bool bHeldVisualRangeExpired = false;
	for (int32 Index = TimedRangeStacks.Num() - 1; Index >= 0; --Index)
	{
		if (WorldTime >= TimedRangeStacks[Index].ExpiresAt)
		{
			TimedRangeStacks.RemoveAt(Index);
			bHeldVisualRangeExpired = true;
		}
	}
	if (bHeldVisualRangeExpired)
	{
		RefreshHeldPresentation();
	}
	if (LastRuneAttackContext.IsValid() && LastRuneAttackWorldTime >= 0.0f &&
	    WorldTime >= LastRuneAttackWorldTime + 1.0f && WorldTime >= NextComboDecayWorldTime)
	{
		if (UReEchoCombatantComponent* SourceCombatant = LastRuneAttackContext->SourceCombatant.Get())
		{
			for (const FReEchoWeaponRuneEffectSpec& Effect : LastRuneAttackContext->Effects)
			{
				if (Effect.BehaviorId == TEXT("Part.AttackMoveSpeedOnAttack"))
				{
					SourceCombatant->RemoveOldestTransientStatModifier(Effect.PartId);
				}
			}
		}
		NextComboDecayWorldTime = WorldTime + 1.0f;
	}
	AdvanceScytheThrow(DeltaSeconds);
	if (ElementIndicator && ElementIndicator->IsVisible())
	{
		if (APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
		{
			ElementIndicator->SetWorldRotation((-Camera->GetCameraRotation().Vector()).Rotation());
		}
	}
	const FReEchoCsvWeaponRow* Definition = FindEquippedDefinition();
	const UReEchoWeaponPresentationProfile* Profile =
	    Definition ? FReEchoWeaponVisualCatalog::ResolveProfile(Definition->VisualKey) : nullptr;
	const EReEchoWeaponMotionMode MotionMode = Profile ? Profile->MotionMode : EReEchoWeaponMotionMode::None;
	if (SwordAnimationTime <= 0.0f || MotionMode == EReEchoWeaponMotionMode::None)
	{
		SwordAnimationTime = 0.0f;
		SetActorRelativeRotation(FQuat::Identity);
		SetActorRelativeLocation(WeaponHandAnchorLocation);
		SwordSprite->SetRelativeLocation(SwordSpriteRestLocation);
		SwordSprite->SetRelativeRotation(SwordSpriteRestRotation);
		return;
	}
	SwordAnimationTime = FMath::Max(0.0f, SwordAnimationTime - DeltaSeconds);
	const float Progress = 1.0f - SwordAnimationTime / SwordAnimationDuration;
	const float Angle = MotionMode == EReEchoWeaponMotionMode::FullSpin
	                        ? Progress * 2.0f * PI * SwordSwingDirection
	                        : ReEchoWeaponVisual::ResolveTripleSwingAngle(Progress, SwordSwingDirection);
	const FQuat SpinRotation(ReEchoWeaponVisual::CameraFacingNormal, Angle);
	SetActorRelativeRotation(SpinRotation);
	SetActorRelativeLocation(MotionMode == EReEchoWeaponMotionMode::FullSpin
	                             ? ReEchoWeaponVisual::ResolveSelfCenteredSpinRootLocation(
	                                   WeaponHandAnchorLocation, ResolveMirroredHeldVisualOffset(), SpinRotation)
	                             : WeaponHandAnchorLocation);
	SwordSprite->SetRelativeLocation(SwordSpriteRestLocation);
	SwordSprite->SetRelativeRotation(SwordSpriteRestRotation);
}

float AReEchoWeaponActor::ResolveOwnerVisualFacingSign() const
{
	if (const AReEchoPlayerPawn* Player = Cast<AReEchoPlayerPawn>(GetOwner()))
	{
		return Player->GetVisualFacingSign();
	}
	if (const AReEchoEchoActor* Echo = Cast<AReEchoEchoActor>(GetOwner()))
	{
		return Echo->GetVisualFacingSign();
	}
	return 1.0f;
}

FVector AReEchoWeaponActor::ResolveMirroredHeldVisualOffset() const
{
	const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0);
	const FVector CameraRight =
	    Camera ? FRotationMatrix(Camera->GetCameraRotation()).GetUnitAxis(EAxis::Y) : FVector::RightVector;
	// HeldVisualOffset is a vector from the hand pivot, so its mirror center is the hand-local origin.
	return ReEchoWeaponVisual::ResolveFacingPointAroundCenter(
	    HeldVisualOffset, FVector::ZeroVector, ResolveOwnerVisualFacingSign(), CameraRight);
}

FQuat AReEchoWeaponActor::ResolveMirroredSwordRestRotation() const
{
	const float RightFacingAngle = ReEchoWeaponVisual::SwordRestAngleRadians + SwordPlanarAngleOffsetRadians;
	// The delivered sword texture's visible blade axis is opposite its mathematical local axis. Mirror the authored
	// camera-plane angle itself so the approved right-side upper-right pose becomes its symmetric upper-left pose.
	const float FacingAngle = ResolveOwnerVisualFacingSign() >= 0.0f ? RightFacingAngle : -RightFacingAngle;
	return SwordAuthoredRotation * ReEchoWeaponVisual::GetSwordRotation(FacingAngle);
}

void AReEchoWeaponActor::ApplyHeldPlaneMirror(UStaticMeshComponent* Plane,
                                              const EReEchoHeldWeaponMirrorRule MirrorRule) const
{
	if (!Plane)
	{
		return;
	}
	const bool bFacesLeft = ResolveOwnerVisualFacingSign() < 0.0f;
	const bool bMirror = MirrorRule == EReEchoHeldWeaponMirrorRule::WhenFacingLeft && bFacesLeft ||
	                     MirrorRule == EReEchoHeldWeaponMirrorRule::WhenFacingRight && !bFacesLeft;
	FVector Scale = Plane->GetRelativeScale3D();
	Scale.X = FMath::Abs(Scale.X) * (bMirror ? -1.0f : 1.0f);
	Plane->SetRelativeScale3D(Scale);
}
