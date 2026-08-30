#include "Graybox/ReEchoProjectileActor.h"

#include "ReEcho.h"
#include "Combat/ReEchoElementReaction.h"
#include "Combat/ReEchoCombatTarget.h"
#include "Camera/PlayerCameraManager.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Math/RotationMatrix.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Presentation/VFX/ReEchoCombatVfxCatalog.h"
#include "Weapons/ReEchoWeaponVisualCatalog.h"
#include "Weapons/ReEchoWeaponActor.h"

namespace ReEchoSplitArrowDiagnostics
{
FVector ResolveTargetLocation(AActor* Actor)
{
	if (const IReEchoCombatTarget* Target = Cast<IReEchoCombatTarget>(Actor))
	{
		return Target->GetCombatTargetLocation();
	}
	return Actor ? Actor->GetActorLocation() : FVector::ZeroVector;
}
} // namespace ReEchoSplitArrowDiagnostics

AReEchoProjectileActor::AReEchoProjectileActor()
{
	PrimaryActorTick.bCanEverTick = true;
	ProjectileLogic = CreateDefaultSubobject<UReEchoProjectileLogicComponent>(TEXT("ProjectileLogic"));
	ProjectileLogic->OnProjectileImpacted.AddUObject(this, &AReEchoProjectileActor::HandleProjectileImpact);
	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->InitSphereRadius(13.f);
	Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Shape = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileShape"));
	Shape->SetupAttachment(RootComponent);
	Shape->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Shape->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")));
	Shape->SetRelativeScale3D(FVector(0.26f));

	ElementLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("ElementLabel"));
	ElementLabel->SetupAttachment(Collision);
	ElementLabel->SetHorizontalAlignment(EHTA_Center);
	ElementLabel->SetVerticalAlignment(EVRTA_TextCenter);
	ElementLabel->SetWorldSize(42.0f);
	ElementLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 28.0f));
	ElementLabel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ElementLabel->SetCastShadow(false);
	ElementLabel->SetTranslucentSortPriority(24);
	ElementLabel->SetVisibility(false);
	if (UMaterialInterface* UnlitTextMaterial =
	        LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineMaterials/UnlitText.UnlitText")))
	{
		ElementLabel->SetTextMaterial(UnlitTextMaterial);
	}
}

void AReEchoProjectileActor::InitializeProjectile(const FVector& Direction,
                                                  float InDamage,
                                                  const FVector& InDamageSource,
                                                  const FLinearColor& Color,
                                                  const EReEchoElement InElement,
                                                  const float InReactionEfficiency,
                                                  const float InExplosionRadiusCm,
                                                  const float InMaxRangeCm,
                                                  const FReEchoAttackIdentity InAttack,
                                                  const bool bInCritical,
                                                  const EReEchoDamageSource InDamageSourceType,
                                                  const FName InWeaponVisualKey,
                                                  const bool bInPierceOnCritical,
                                                  AReEchoWeaponActor* InRuneHost,
                                                  TSharedPtr<FReEchoWeaponRuneAttackContext> InRuneContext,
                                                  const bool bInAllowSplit)
{
	Damage = FMath::Max(0.f, InDamage);
	Element = InElement;
	ExplosionRadiusCm = FMath::Max(0.0f, InExplosionRadiusCm);
	WeaponVisualKey = InWeaponVisualKey;
	RuneHost = InRuneHost;
	RuneContext = MoveTemp(InRuneContext);
	bAllowSplit = bInAllowSplit;
	bImpactVfxSpawned = false;
	FReEchoLogicalProjectileSpec Spec;
	Spec.HitIntent.Attack = InAttack;
	Spec.HitIntent.RawDamage = Damage;
	Spec.HitIntent.bCritical = bInCritical;
	Spec.HitIntent.DamageSource = InDamageSourceType;
	Spec.HitIntent.Element = Element;
	Spec.HitIntent.ReactionEfficiency = FMath::Max(0.0f, InReactionEfficiency);
	Spec.HitIntent.SourceLocation = InDamageSource;
	Spec.Direction = Direction;
	Spec.SpeedCmPerSecond = Speed;
	Spec.CarrierRadiusCm = Collision->GetScaledSphereRadius();
	Spec.ExplosionRadiusCm = ExplosionRadiusCm;
	Spec.MaximumRangeCm = FMath::Max(1.0f, InMaxRangeCm);
	Spec.bPierceOnCritical = bInPierceOnCritical;
	if (!ProjectileLogic->InitializeProjectile(Spec))
	{
		Destroy();
		return;
	}
	const bool bHasElement = ReEchoElementReaction::IsCombatElement(Element);
	Shape->SetVisibility(!bHasElement);
	ElementLabel->SetVisibility(bHasElement);
	if (bHasElement)
	{
		const FString Label = ReEchoElementReaction::GetElementLabel(Element).Left(1);
		ElementLabel->SetText(FText::FromString(Label));
		ElementLabel->SetTextRenderColor(Color.ToFColor(false));
	}
	if (UMaterialInterface* Base =
	        LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
	{
		UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Base, this);
		Material->SetVectorParameterValue(TEXT("Color"), Color);
		Shape->SetMaterial(0, Material);
	}
	ConfigureWeaponVisual(InWeaponVisualKey, Color);
	if (ConfigureWeaponNiagara(InWeaponVisualKey, Direction) && ElementLabel)
	{
		// The dedicated element Niagara is the readable element presentation. Keep the text label only for fallback.
		ElementLabel->SetVisibility(false);
	}
}

#if !UE_BUILD_SHIPPING
void AReEchoProjectileActor::ConfigureSplitDiagnostics(const FGuid& InParentProjectileId,
                                                       const int32 InChildIndex,
                                                       AActor* InParentHitTarget,
                                                       AActor* InIntendedTarget)
{
	SplitParentProjectileId = InParentProjectileId;
	SplitChildIndex = InChildIndex;
	SplitParentHitTarget = InParentHitTarget;
	SplitIntendedTarget = InIntendedTarget;
	bHasSplitDiagnostics = true;

	const FReEchoProjectileSnapshot Snapshot = ProjectileLogic ? ProjectileLogic->GetSnapshot()
	                                                          : FReEchoProjectileSnapshot{};
	const FVector ParentLocation =
	    ReEchoSplitArrowDiagnostics::ResolveTargetLocation(SplitParentHitTarget.Get());
	const FVector IntendedLocation =
	    ReEchoSplitArrowDiagnostics::ResolveTargetLocation(SplitIntendedTarget.Get());
	UE_LOG(LogReEcho,
	       Log,
	       TEXT("[SplitArrowTrace] ChildSpawn parentProjectile=%s childProjectile=%s childActor='%s' childIndex=%d "
	            "parentTarget='%s' intendedTarget='%s' spawn=(%.2f,%.2f,%.2f) velocity=(%.2f,%.2f,%.2f) "
	            "distanceToParent2D=%.2f distanceToIntended2D=%.2f"),
	       *SplitParentProjectileId.ToString(EGuidFormats::DigitsWithHyphensLower),
	       *Snapshot.ProjectileId.Value.ToString(EGuidFormats::DigitsWithHyphensLower),
	       *GetNameSafe(this),
	       SplitChildIndex,
	       *GetNameSafe(SplitParentHitTarget.Get()),
	       *GetNameSafe(SplitIntendedTarget.Get()),
	       Snapshot.Position.X,
	       Snapshot.Position.Y,
	       Snapshot.Position.Z,
	       Snapshot.Velocity.X,
	       Snapshot.Velocity.Y,
	       Snapshot.Velocity.Z,
	       FVector::Dist2D(Snapshot.Position, ParentLocation),
	       FVector::Dist2D(Snapshot.Position, IntendedLocation));
}
#endif

bool AReEchoProjectileActor::ConfigureWeaponNiagara(const FName InWeaponVisualKey, const FVector& Direction)
{
	EReEchoCombatVfxSemantic Semantic;
	bool bElementSpecificFlight = false;
	if (InWeaponVisualKey == TEXT("Bow"))
	{
		bElementSpecificFlight = FReEchoCombatVfxCatalog::ResolveBowFlightSemantic(Element, Semantic);
		if (!bElementSpecificFlight)
		{
			Semantic = EReEchoCombatVfxSemantic::PlayerBowFlight;
		}
	}
	else if (InWeaponVisualKey == TEXT("Gun"))
	{
		bElementSpecificFlight = FReEchoCombatVfxCatalog::ResolveGunFlightSemantic(Element, Semantic);
		if (!bElementSpecificFlight)
		{
			Semantic = EReEchoCombatVfxSemantic::PlayerGunFlight;
		}
	}
	else
	{
		return false;
	}
	Shape->SetVisibility(false);
	UNiagaraSystem* System = LoadObject<UNiagaraSystem>(nullptr, *FReEchoCombatVfxCatalog::ResolvePath(Semantic));
	if (!System && bElementSpecificFlight)
	{
		// Presentation failure must not change the logical projectile. Retain the element text and use the weapon's
		// configured default Travel system when the dedicated asset is missing or unloadable.
		Semantic = InWeaponVisualKey == TEXT("Bow") ? EReEchoCombatVfxSemantic::PlayerBowFlight
		                                            : EReEchoCombatVfxSemantic::PlayerGunFlight;
		System = LoadObject<UNiagaraSystem>(nullptr, *FReEchoCombatVfxCatalog::ResolvePath(Semantic));
		bElementSpecificFlight = false;
	}
	if (System)
	{
		const FRotator FlightRotation = FReEchoCombatVfxCatalog::ResolveRotation(Semantic, Direction);
		FlightEffect = UNiagaraFunctionLibrary::SpawnSystemAttached(
		    System,
		    RootComponent,
		    NAME_None,
		    FVector::ZeroVector,
		    InWeaponVisualKey == TEXT("Bow") ? FRotator::ZeroRotator : FlightRotation,
		    FVector::OneVector,
		    EAttachLocation::KeepRelativeOffset,
		    false,
		    ENCPoolMethod::None,
		    true);
		if (FlightEffect)
		{
			FlightEffect->SetTranslucentSortPriority(1000);
			if (InWeaponVisualKey == TEXT("Bow"))
			{
				// Preserve every authored particle/renderer setting. Only rotate the complete system once so its
				// authored +Y flight axis matches the committed shooter-to-target direction.
				FlightEffect->SetAbsolute(false, true, false);
				FlightEffect->SetWorldRotation(FlightRotation);
			}
		}
	}
	return bElementSpecificFlight && FlightEffect != nullptr;
}

void AReEchoProjectileActor::HandleProjectileImpact(const FReEchoProjectileSnapshot& Snapshot,
                                                    const FReEchoHitResolved& Result)
{
#if !UE_BUILD_SHIPPING
	if (bHasSplitDiagnostics)
	{
		UE_LOG(LogReEcho,
		       Log,
		       TEXT("[SplitArrowTrace] ChildImpact parentProjectile=%s childProjectile=%s childActor='%s' "
		            "childIndex=%d parentTarget='%s' intendedTarget='%s' actualTarget='%s' "
		            "position=(%.2f,%.2f,%.2f) hit=(%.2f,%.2f,%.2f) travelled=%.2f raw=%.3f applied=%.3f "
		            "blocked=%d killed=%d"),
		       *SplitParentProjectileId.ToString(EGuidFormats::DigitsWithHyphensLower),
		       *Snapshot.ProjectileId.Value.ToString(EGuidFormats::DigitsWithHyphensLower),
		       *GetNameSafe(this),
		       SplitChildIndex,
		       *GetNameSafe(SplitParentHitTarget.Get()),
		       *GetNameSafe(SplitIntendedTarget.Get()),
		       *GetNameSafe(Result.Target),
		       Snapshot.Position.X,
		       Snapshot.Position.Y,
		       Snapshot.Position.Z,
		       Result.HitLocation.X,
		       Result.HitLocation.Y,
		       Result.HitLocation.Z,
		       Snapshot.TravelledCm,
		       Result.RawDamage,
		       Result.AppliedDamage,
		       Result.bBlocked ? 1 : 0,
		       Result.bKilled ? 1 : 0);
	}
#endif
	if (RuneHost.IsValid() && RuneContext.IsValid())
	{
		RuneHost->HandleProjectileResolved(RuneContext, Snapshot, Result, bAllowSplit);
	}
	if (bImpactVfxSpawned || Result.AppliedDamage <= 0.0f)
	{
		return;
	}
	bImpactVfxSpawned = true;
	SpawnWeaponImpactNiagara(GetActorLocation(), Snapshot.Velocity);
}

void AReEchoProjectileActor::SpawnWeaponImpactNiagara(const FVector& Location, const FVector& Direction)
{
	EReEchoCombatVfxSemantic Semantic;
	if (!FReEchoCombatVfxCatalog::ResolveProjectileImpactSemantic(WeaponVisualKey, ExplosionRadiusCm, Semantic))
	{
		return;
	}
	if (UNiagaraSystem* System = LoadObject<UNiagaraSystem>(nullptr, *FReEchoCombatVfxCatalog::ResolvePath(Semantic)))
	{
		if (UNiagaraComponent* ImpactEffect = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		        GetWorld(),
		        System,
		        Location,
		        FReEchoCombatVfxCatalog::ResolveRotation(Semantic, Direction),
		        FVector::OneVector,
		        true,
		        true,
		        ENCPoolMethod::None,
		        true))
		{
			ImpactEffect->SetTranslucentSortPriority(1000);
		}
	}
}

FString AReEchoProjectileActor::ResolveWeaponTexturePath(const FName WeaponVisualKey)
{
	if (WeaponVisualKey != TEXT("Bow") && WeaponVisualKey != TEXT("Gun") && WeaponVisualKey != TEXT("MoonStaff"))
	{
		return FString();
	}
	return FReEchoWeaponVisualCatalog::ResolveAttackTexturePath(WeaponVisualKey);
}

void AReEchoProjectileActor::ConfigureWeaponVisual(const FName InWeaponVisualKey, const FLinearColor& Color)
{
	const FString TexturePath = ResolveWeaponTexturePath(InWeaponVisualKey);
	UTexture2D* Texture = TexturePath.IsEmpty() ? nullptr : LoadObject<UTexture2D>(nullptr, *TexturePath);
	if (Texture)
	{
		UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
		UMaterialInterface* SpriteMaterial = LoadObject<UMaterialInterface>(
		    nullptr, TEXT("/Paper2D/TranslucentUnlitSpriteMaterial.TranslucentUnlitSpriteMaterial"));
		if (Plane && SpriteMaterial)
		{
			Shape->SetStaticMesh(Plane);
			// The texture is a camera card. Gameplay direction remains owned by ProjectileLogic.
			Shape->SetAbsolute(false, true, false);
			Shape->SetWorldRotation(
			    FRotationMatrix::MakeFromZX(FVector(-0.573576f, 0.0f, 0.819152f), FVector::RightVector).Rotator());
			UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(SpriteMaterial, this);
			Material->SetTextureParameterValue(TEXT("SpriteTexture"), Texture);
			Shape->SetMaterial(0, Material);
			const float Height = InWeaponVisualKey == TEXT("Bow") ? 34.0f : 56.0f;
			const float Aspect = static_cast<float>(Texture->GetSizeX()) / FMath::Max(1, Texture->GetSizeY());
			Shape->SetRelativeScale3D(FVector(Height * Aspect / 100.0f, Height / 100.0f, 1.0f));
			Shape->SetVisibility(true);
			return;
		}
	}

	// Distinct procedural fallbacks keep missing art playable and visually diagnosable.
	if (InWeaponVisualKey == TEXT("Bow"))
	{
		Shape->SetRelativeScale3D(FVector(0.12f, 0.48f, 0.12f));
	}
	else if (InWeaponVisualKey == TEXT("Gun"))
	{
		Shape->SetRelativeScale3D(FVector(0.14f));
	}
	else if (InWeaponVisualKey == TEXT("MoonStaff"))
	{
		Shape->SetRelativeScale3D(FVector(0.34f, 0.18f, 0.34f));
	}
}

void AReEchoProjectileActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (ElementLabel && ElementLabel->IsVisible())
	{
		if (APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
		{
			ElementLabel->SetWorldRotation((-Camera->GetCameraRotation().Vector()).Rotation());
		}
	}
}
