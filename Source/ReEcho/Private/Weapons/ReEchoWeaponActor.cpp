#include "Weapons/ReEchoWeaponActor.h"

#include "ReEcho.h"
#include "Camera/PlayerCameraManager.h"

#include "Combat/ReEchoCombatantComponent.h"
#include "Combat/ReEchoAttackControllerComponent.h"
#include "Combat/ReEchoCombatContracts.h"
#include "Combat/ReEchoCombatTarget.h"
#include "Combat/ReEchoElementReaction.h"
#include "Combat/ReEchoHitResolver.h"
#include "Components/BillboardComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Math/RotationMatrix.h"
#include "Graybox/ReEchoStaffLightWaveActor.h"
#include "Graybox/ReEchoEchoActor.h"
#include "Graybox/ReEchoProjectileActor.h"
#include "Graybox/ReEchoSwordArcActor.h"
#include "Kismet/GameplayStatics.h"
#include "Player/ReEchoPlayerPawn.h"
#include "Presentation/Weapon/ReEchoWeaponPresentationProfile.h"
#include "Weapons/ReEchoWeaponGeometry.h"
#include "Weapons/ReEchoWeaponVisualCatalog.h"

namespace ReEchoWeaponVisual
{
const FVector StaffLocation(-16.0f, 30.0f, 6.0f);
const FVector SwordLocation(8.0f, 0.0f, 0.0f);
constexpr float SwordRestAngleRadians = -PI / 4.0f;
const FVector CameraFacingNormal(-0.573576f, 0.0f, 0.819152f);

FQuat GetSwordRotation(const float SpinRadians = 0.0f)
{
	const FQuat CameraFacingRotation = FRotationMatrix::MakeFromZX(CameraFacingNormal, FVector::RightVector).ToQuat();
	return FQuat(CameraFacingNormal, SpinRadians) * CameraFacingRotation;
}
}

AReEchoWeaponActor::AReEchoWeaponActor()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	// 武器只继承持有者位置，不继承鼠标瞄准产生的角色旋转。
	Root->SetAbsolute(false, true, false);

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
	const FString StaffTexturePath = FReEchoWeaponVisualCatalog::ResolveHeldTexturePath(TEXT("Staff"));
	if (UTexture2D* StaffTexture = LoadObject<UTexture2D>(nullptr, *StaffTexturePath))
	{
		StaffSprite->SetSprite(StaffTexture);
		constexpr float StaffWorldHeight = 250.0f;
		StaffSprite->SetRelativeScale3D(FVector(StaffWorldHeight / FMath::Max(1, StaffTexture->GetSizeY())));
	}

	auto CreateWeaponBillboard = [this](const TCHAR* Name, const FString& TexturePath)
	{
		UBillboardComponent* Billboard = CreateDefaultSubobject<UBillboardComponent>(Name);
		Billboard->SetupAttachment(Root);
		Billboard->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Billboard->SetCastShadow(false);
		Billboard->SetTranslucentSortPriority(6);
		Billboard->SetRelativeLocation(ReEchoWeaponVisual::StaffLocation);
		Billboard->SetHiddenInGame(false);
		if (UTexture2D* Tex = LoadObject<UTexture2D>(nullptr, *TexturePath))
		{
			Billboard->SetSprite(Tex);
			constexpr float WorldHeight = 250.0f;
			Billboard->SetRelativeScale3D(FVector(WorldHeight / FMath::Max(1, Tex->GetSizeY())));
		}
		return Billboard;
	};
	ScytheSprite =
	    CreateWeaponBillboard(TEXT("ScytheSprite"), FReEchoWeaponVisualCatalog::ResolveHeldTexturePath(TEXT("Scythe")));
	WhipSprite =
	    CreateWeaponBillboard(TEXT("WhipSprite"), FReEchoWeaponVisualCatalog::ResolveHeldTexturePath(TEXT("Whip")));
	BowSprite =
	    CreateWeaponBillboard(TEXT("BowSprite"), FReEchoWeaponVisualCatalog::ResolveHeldTexturePath(TEXT("Bow")));
	GunSprite =
	    CreateWeaponBillboard(TEXT("GunSprite"), FReEchoWeaponVisualCatalog::ResolveHeldTexturePath(TEXT("Gun")));

	// Billboard 会在渲染阶段覆盖组件旋转；使用透明 Plane 才能稳定显示武器自身的 360 度旋转。
	SwordSprite = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SwordSprite"));
	SwordSprite->SetupAttachment(Root);
	SwordSprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SwordSprite->SetCastShadow(false);
	SwordSprite->SetTranslucentSortPriority(-1);
	SwordSprite->SetRelativeLocation(ReEchoWeaponVisual::SwordLocation);
	SwordSprite->SetRelativeRotation(ReEchoWeaponVisual::GetSwordRotation(ReEchoWeaponVisual::SwordRestAngleRadians));
	SwordSprite->SetHiddenInGame(false);
	if (UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")))
	{
		SwordSprite->SetStaticMesh(PlaneMesh);
	}
	UMaterialInterface* SpriteMaterial = LoadObject<UMaterialInterface>(
	    nullptr, TEXT("/Paper2D/TranslucentUnlitSpriteMaterial.TranslucentUnlitSpriteMaterial"));
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

void AReEchoWeaponActor::InitializeWeapon(const FReEchoBuildSnapshot* InBuildSnapshot,
                                          TSharedPtr<const FReEchoCsvDataSnapshot> InSnapshot)
{
	SwordSpriteRestLocation = ReEchoWeaponVisual::SwordLocation;
	WeaponLogic.Reset();
	LastAttackCommit = {};
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
	UpdateElementIndicator();
	SwordSprite->SetRelativeLocation(SwordSpriteRestLocation);
	SwordSprite->SetRelativeRotation(ReEchoWeaponVisual::GetSwordRotation(ReEchoWeaponVisual::SwordRestAngleRadians));
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
	LastCommittedAttackStepId = NAME_None;
	LastCommittedAttackStepIndex = INDEX_NONE;
	SwordAnimationTime = 0.0f;
	if (!WeaponLogic.Initialize(ReEchoWeaponRuntime::CompileLogicDefinition(EffectiveDefinition)))
	{
		return false;
	}
	RefreshVisualState();
	UpdateElementIndicator();
	SwordSprite->SetRelativeLocation(SwordSpriteRestLocation);
	SwordSprite->SetRelativeRotation(ReEchoWeaponVisual::GetSwordRotation(ReEchoWeaponVisual::SwordRestAngleRadians));
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
	if (!ExecuteAttack(Combatant, LastAttackCommit))
	{
		WeaponLogic.RollbackLastCommit();
		LastCommittedAttackStepId = NAME_None;
		LastCommittedAttackStepIndex = INDEX_NONE;
		return false;
	}
	WeaponLogic.ConfirmLastCommit();
	UpdateElementIndicator();
	if (AActor* WeaponOwner = GetOwner())
	{
		if (UReEchoCombatEventsComponent* Events = WeaponOwner->FindComponentByClass<UReEchoCombatEventsComponent>())
		{
			FReEchoAttackCommittedEvent Event;
			Event.Attack = LastAttackCommit.Attack;
			if (const UReEchoAttackControllerComponent* Controller =
			        WeaponOwner->FindComponentByClass<UReEchoAttackControllerComponent>())
			{
				Event.Target = Controller->GetSnapshot().CurrentTarget;
			}
			Event.WeaponId = GetEquippedWeaponId();
			Event.AttackPatternId = GetAttackPatternId();
			Event.AttackStepId = LastCommittedAttackStepId;
			Event.StepIndex = LastCommittedAttackStepIndex;
			Event.Origin = WeaponOwner->GetActorLocation();
			Event.Direction = ResolveOwnerAimDirection();
			Events->PublishAttackCommitted(Event);
		}
	}
	return true;
}

bool AReEchoWeaponActor::TryActiveAttack(UReEchoCombatantComponent* Combatant)
{
	if (!Combatant || !WeaponLogic.TryCommitActiveAttack(GetOwner(), Combatant->Stats, LastAttackCommit))
	{
		return false;
	}
	if (!ExecuteAttack(Combatant, LastAttackCommit))
	{
		WeaponLogic.RollbackLastCommit();
		return false;
	}
	WeaponLogic.ConfirmLastCommit();
	UpdateElementIndicator();
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
	return WeaponLogic.GetCurrentRangeCm();
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
	switch (Commit.Carrier)
	{
		case EReEchoWeaponAttackCarrier::Wave:
			return FireStaffLightWave(Commit, Combatant);
		case EReEchoWeaponAttackCarrier::Projectile:
			return FireProjectile(Commit, Combatant);
		default:
			return SwingMelee(Commit, Combatant);
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
	return bHasEffectiveDefinition;
}

bool AReEchoWeaponActor::ApplyDamageToTarget(AActor& Target,
                                             const FReEchoWeaponAttackCommit& Commit,
                                             const FVector& DamageSource,
                                             UReEchoCombatantComponent* Combatant) const
{
	const IReEchoCombatTarget* CombatTarget = Cast<IReEchoCombatTarget>(&Target);
	UReEchoCombatantComponent* TargetCombatant = CombatTarget ? CombatTarget->GetCombatTargetCombatant() : nullptr;
	if (!TargetCombatant || !TargetCombatant->IsAlive())
	{
		return false;
	}
	const bool bWasAlive = TargetCombatant->IsAlive();
	FReEchoHitIntent Intent;
	Intent.Attack = Commit.Attack;
	Intent.Target = &Target;
	Intent.RawDamage = Commit.RawDamage;
	Intent.DamageSource = ResolveOwnerDamageSource();
	Intent.Element = Commit.Element;
	Intent.ReactionEfficiency = Combatant ? Combatant->Stats.ReactionEfficiency : 1.0f;
	Intent.bCritical = Commit.bCritical;
	Intent.SourceLocation = DamageSource;
	Intent.HitLocation = Target.GetActorLocation();
	const float AppliedDamage = ReEchoHitResolver::ResolveHit(Intent).AppliedDamage;
	const bool bKilled = bWasAlive && !TargetCombatant->IsAlive();
	if (bKilled && Combatant && WeaponLogic.GetOnKillHealPercent() > 0.0f)
	{
		Combatant->ApplyHealing(Combatant->Stats.HpMax * WeaponLogic.GetOnKillHealPercent());
	}
	return AppliedDamage > 0.0f || TargetCombatant->IsAlive();
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

bool AReEchoWeaponActor::FireProjectile(const FReEchoWeaponAttackCommit& Commit, UReEchoCombatantComponent* Combatant)
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
	for (const FVector& Direction : Directions)
	{
		const FVector SpawnLocation = OwnerLocation + FVector(0.0f, 0.0f, 35.0f) + Direction * 45.0f;
		AReEchoProjectileActor* Projectile =
		    GetWorld()->SpawnActor<AReEchoProjectileActor>(SpawnLocation, Direction.Rotation());
		if (!Projectile)
		{
			continue;
		}
		Projectile->SetOwner(WeaponOwner);
		Projectile->InitializeProjectile(Direction,
		                                 Commit.RawDamage,
		                                 OwnerLocation,
		                                 ReEchoElementReaction::GetElementColor(Commit.Element),
		                                 Commit.Element,
		                                 Combatant->Stats.ReactionEfficiency,
		                                 Commit.ExplosionRadiusCm,
		                                 Commit.RangeCm,
		                                 Commit.Attack,
		                                 ResolveOwnerDamageSource(),
		                                 GetEquippedWeaponVisualKey());
		bSpawnedAny = true;
	}
	return bSpawnedAny;
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
	const bool bShowSword = VisualKey == TEXT("CrescentBlade") || VisualKey == TEXT("Whip");
	const bool bShowElement = VisualKey == TEXT("ElementalOrb");
	const bool bShowStaff = VisualKey == TEXT("MoonStaff") || VisualKey == TEXT("Staff");
	const bool bShowScythe = VisualKey == TEXT("Scythe");
	// Whip asset pending production; rendered as longsword placeholder, so its billboard stays hidden.
	const bool bShowWhip = false;
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
	if (WhipSprite)
	{
		WhipSprite->SetVisibility(bShowWhip);
		WhipSprite->SetHiddenInGame(!bShowWhip);
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

bool AReEchoWeaponActor::SwingMelee(const FReEchoWeaponAttackCommit& Commit, UReEchoCombatantComponent* Combatant)
{
	AActor* WeaponOwner = GetOwner();
	if (!WeaponOwner)
	{
		return false;
	}
	const FVector OwnerLocation = WeaponOwner->GetActorLocation();
	const FVector AimDirection = ResolveOwnerAimDirection();
	// 旋转攻击以角色为圆心覆盖完整一周；敌人受伤逻辑会从圆心向外施加击退。
	for (AActor* Target : ReEchoWeaponGeometry::FindMeleeTargets(
	         *GetWorld(), Commit.Attack, OwnerLocation, AimDirection, Commit.RangeCm, Commit.ArcDegrees))
	{
		ApplyDamageToTarget(*Target, Commit, OwnerLocation, Combatant);
	}
	StartMeleeAnimation(GetEquippedWeaponVisualKey());
	return true;
}

void AReEchoWeaponActor::StartMeleeAnimation(const FName WeaponVisualKey)
{
	SwordSwingDirection *= -1.0f;
	const UReEchoWeaponPresentationProfile* Profile = FReEchoWeaponVisualCatalog::ResolveProfile(WeaponVisualKey);
	SwordAnimationTime =
	    Profile && Profile->MotionMode == EReEchoWeaponMotionMode::FullSpin ? SwordAnimationDuration : 0.0f;
	// Longsword and scythe attack presentation is owned by the combat Niagara event adapter.
	// Whip remains on its legacy placeholder until dedicated Niagara art is delivered.
	if (Profile && !Profile->LegacyAttackTexture.IsNull() && WeaponVisualKey == TEXT("Whip"))
	{
		SpawnMeleeArc(WeaponVisualKey);
	}
}

void AReEchoWeaponActor::SpawnMeleeArc(const FName WeaponVisualKey)
{
	AActor* WeaponOwner = GetOwner();
	if (!WeaponOwner)
	{
		return;
	}
	const FVector ArcLocation = WeaponOwner->GetActorLocation() + FVector(-12.0f, 0.0f, 42.0f);
	if (AReEchoSwordArcActor* SwordArc =
	        GetWorld()->SpawnActor<AReEchoSwordArcActor>(ArcLocation, ResolveOwnerAimDirection().Rotation()))
	{
		SwordArc->SetOwner(WeaponOwner);
		SwordArc->InitializeArc(SwordSwingDirection, WeaponVisualKey);
	}
}

void AReEchoWeaponActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	WeaponLogic.Tick(DeltaSeconds);
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
	const bool bUsesSwordVisual = Profile && Profile->MotionMode == EReEchoWeaponMotionMode::FullSpin;
	if (SwordAnimationTime <= 0.0f || !bUsesSwordVisual)
	{
		SwordAnimationTime = 0.0f;
		SwordSprite->SetRelativeLocation(SwordSpriteRestLocation);
		SwordSprite->SetRelativeRotation(
		    ReEchoWeaponVisual::GetSwordRotation(ReEchoWeaponVisual::SwordRestAngleRadians));
		return;
	}
	SwordAnimationTime = FMath::Max(0.0f, SwordAnimationTime - DeltaSeconds);
	const float Progress = 1.0f - SwordAnimationTime / SwordAnimationDuration;
	const float Angle = Progress * 2.0f * PI * SwordSwingDirection;
	// 武器位置固定在手部挂点，只旋转贴图自身，不再绕角色公转。
	SwordSprite->SetRelativeLocation(SwordSpriteRestLocation);
	SwordSprite->SetRelativeRotation(
	    ReEchoWeaponVisual::GetSwordRotation(ReEchoWeaponVisual::SwordRestAngleRadians + Angle));
}
