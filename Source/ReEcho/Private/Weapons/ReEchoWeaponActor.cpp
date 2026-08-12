#include "Weapons/ReEchoWeaponActor.h"

#include "ReEcho.h"
#include "Camera/PlayerCameraManager.h"

#include "Combat/ReEchoCombatantComponent.h"
#include "Combat/ReEchoElementReaction.h"
#include "Components/BillboardComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Math/RotationMatrix.h"
#include "EngineUtils.h"
#include "Graybox/ReEchoEnemyActor.h"
#include "Graybox/ReEchoStaffLightWaveActor.h"
#include "Graybox/ReEchoProjectileActor.h"
#include "Graybox/ReEchoSwordArcActor.h"
#include "Kismet/GameplayStatics.h"

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
	if (UTexture2D* StaffTexture =
	        LoadObject<UTexture2D>(nullptr, TEXT("/Game/ReEcho/Textures/Effects/MoonStaff.MoonStaff")))
	{
		StaffSprite->SetSprite(StaffTexture);
		constexpr float StaffWorldHeight = 250.0f;
		StaffSprite->SetRelativeScale3D(FVector(StaffWorldHeight / FMath::Max(1, StaffTexture->GetSizeY())));
	}

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
	UTexture2D* WeaponTexture =
	    LoadObject<UTexture2D>(nullptr, TEXT("/Game/ReEcho/Textures/Effects/CrescentWeapon.CrescentWeapon"));
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
	NextElementIndex = 0;
	AttackSequence = 0;
	NextStepCursor = 0;
	StepLockRemaining = 0.0f;
	InvulnerableRemaining = 0.0f;
	CriticalAccumulator = 0.0f;
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
	}
	else
	{
		BuildSnapshot = {};
		BuildSnapshot.WeaponDomainRevision = DataSnapshot->WeaponDomainRevision;
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
	AttackCooldown = 0.0f;
	SwordAnimationTime = 0.0f;
	if (!RebuildEffectiveDefinition())
	{
		UE_LOG(LogReEcho,
		       Fatal,
		       TEXT("Cannot initialize effective weapon definition for WeaponId '%s'"),
		       *EquippedWeaponId.ToString());
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
	AttackCooldown = 0.0f;
	StepLockRemaining = 0.0f;
	InvulnerableRemaining = 0.0f;
	NextStepCursor = 0;
	SwordAnimationTime = 0.0f;
	RefreshVisualState();
	UpdateElementIndicator();
	SwordSprite->SetRelativeLocation(SwordSpriteRestLocation);
	SwordSprite->SetRelativeRotation(ReEchoWeaponVisual::GetSwordRotation(ReEchoWeaponVisual::SwordRestAngleRadians));
	return true;
}

bool AReEchoWeaponActor::TryBasicAttack(UReEchoCombatantComponent* Combatant)
{
	if (!Combatant || AttackCooldown > 0.0f || !ExecuteAttack(Combatant))
	{
		return false;
	}
	AttackCooldown = GetAttackInterval(Combatant);
	return true;
}

bool AReEchoWeaponActor::TryActiveAttack(UReEchoCombatantComponent* Combatant)
{
	return TryBasicAttack(Combatant);
}

bool AReEchoWeaponActor::ExecuteBasicAttack(UReEchoCombatantComponent* Combatant)
{
	return Combatant && ExecuteAttack(Combatant);
}

float AReEchoWeaponActor::GetAttackInterval(UReEchoCombatantComponent* Combatant) const
{
	return bHasEffectiveDefinition && Combatant
	           ? EffectiveDefinition.Weapon.AttackIntervalSeconds / FMath::Max(0.1f, Combatant->Stats.AttackSpeed)
	           : 0.55f;
}

float AReEchoWeaponActor::GetCurrentAttackRangeCm() const
{
	if (!bHasEffectiveDefinition)
	{
		return 0.0f;
	}
	const FReEchoCsvAttackStepRow* Step = ResolveNextAttackStep();
	const float StepRange = Step ? Step->RangeCm : 0.0f;
	return StepRange > 0.0f ? StepRange : EffectiveDefinition.Weapon.RangeCm;
}

float AReEchoWeaponActor::GetAttackCooldownRemaining() const
{
	return AttackCooldown;
}

bool AReEchoWeaponActor::ExecuteAttack(UReEchoCombatantComponent* Combatant)
{
	if (!Combatant || !bHasEffectiveDefinition || StepLockRemaining > 0.0f)
	{
		return false;
	}
	const FReEchoCsvAttackStepRow* Step = ResolveNextAttackStep();
	if (!Step || !Step->bEnabled)
	{
		return false;
	}
	if (Step->ConditionId != NAME_None && Step->ConditionId != TEXT("None"))
	{
		return false;
	}
	if (Step->FormulaId != TEXT("None") && Step->FormulaId != TEXT("Weapon.PhysicalOrElementalCoefficient"))
	{
		return false;
	}
	if (Step->BehaviorId != TEXT("Weapon.AttackStep") && Step->BehaviorId != TEXT("Weapon.DashStrike"))
	{
		return false;
	}
	BeginAttackStep(*Step);
	bool bExecuted = false;
	if (EffectiveDefinition.Weapon.AttackPatternId == TEXT("Pattern.MoonStaffWave"))
	{
		bExecuted = FireStaffLightWave(EffectiveDefinition, *Step, Combatant);
	}
	else if (FMath::Max(EffectiveDefinition.Weapon.ProjectileCount, Step->ProjectileCount) > 0 ||
	         EffectiveDefinition.Weapon.AttackPatternId == TEXT("Pattern.ElementalProjectile") ||
	         EffectiveDefinition.Weapon.AttackPatternId == TEXT("Pattern.BowShot") ||
	         EffectiveDefinition.Weapon.AttackPatternId == TEXT("Pattern.GunShot") ||
	         EffectiveDefinition.Weapon.AttackPatternId == TEXT("Pattern.StaffProjectile"))
	{
		bExecuted = FireProjectile(EffectiveDefinition, *Step, Combatant);
	}
	else
	{
		bExecuted = SwingMelee(EffectiveDefinition, *Step, Combatant);
	}
	if (bExecuted)
	{
		NextStepCursor = (NextStepCursor + 1) % EffectiveDefinition.AttackSteps.Num();
	}
	return bExecuted;
}

FName AReEchoWeaponActor::GetEquippedWeaponId() const
{
	return EquippedWeaponId;
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
	return InvulnerableRemaining > 0.0f;
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

const FReEchoCsvAttackStepRow* AReEchoWeaponActor::ResolveNextAttackStep() const
{
	return EffectiveDefinition.AttackSteps.IsValidIndex(NextStepCursor)
	           ? &EffectiveDefinition.AttackSteps[NextStepCursor]
	           : nullptr;
}

void AReEchoWeaponActor::BeginAttackStep(const FReEchoCsvAttackStepRow& Step)
{
	StepLockRemaining = FMath::Max(0.0f, Step.DurationSeconds);
	if (Step.bInvulnerable)
	{
		InvulnerableRemaining = FMath::Max(InvulnerableRemaining, Step.DurationSeconds);
	}
	if (Step.MovementCm > 0.0f)
	{
		if (AActor* WeaponOwner = GetOwner())
		{
			FVector Direction = WeaponOwner->GetActorForwardVector().GetSafeNormal2D();
			if (Direction.IsNearlyZero())
			{
				Direction = FVector::ForwardVector;
			}
			WeaponOwner->SetActorLocation(WeaponOwner->GetActorLocation() + Direction * Step.MovementCm,
			                              false,
			                              nullptr,
			                              ETeleportType::TeleportPhysics);
		}
	}
}

float AReEchoWeaponActor::ComputeStepDamage(const FReEchoCsvAttackStepRow& Step,
                                            const FReEchoStatBlock& Stats,
                                            const bool bElemental)
{
	const float PhysicalCoefficient =
	    Step.PhysicalCoefficient > 0.0f ? Step.PhysicalCoefficient : EffectiveDefinition.Weapon.PhysicalCoefficient;
	const float ElementalCoefficient =
	    Step.ElementalCoefficient > 0.0f ? Step.ElementalCoefficient : EffectiveDefinition.Weapon.ElementalCoefficient;
	const float BaseDamage = bElemental ? Stats.ElementalAttack * FMath::Max(ElementalCoefficient, PhysicalCoefficient)
	                                    : Stats.PhysicalAttack * PhysicalCoefficient;
	return ApplyRoleDamageModifiers(BaseDamage, Stats);
}

bool AReEchoWeaponActor::ApplyDamageToEnemy(AReEchoEnemyActor& Enemy,
                                            const float Damage,
                                            const FVector& DamageSource,
                                            AActor* DamageCauser,
                                            UReEchoCombatantComponent* Combatant,
                                            const EReEchoElement Element) const
{
	if (!Enemy.IsAlive())
	{
		return false;
	}
	const UReEchoCombatantComponent* EnemyCombatant = Enemy.GetCombatantComponent();
	const float HealthBefore = EnemyCombatant ? EnemyCombatant->CurrentHealth : 0.0f;
	if (Element == EReEchoElement::None)
	{
		Enemy.ReceiveGrayboxDamage(Damage, DamageSource, DamageCauser);
	}
	else
	{
		Enemy.ReceiveElementalDamage(Damage, Element, DamageSource, DamageCauser, Combatant->Stats.ReactionEfficiency);
	}
	const bool bKilled = HealthBefore > 0.0f && !Enemy.IsAlive();
	if (bKilled && Combatant && EffectiveDefinition.OnKillHealPercent > 0.0f)
	{
		Combatant->ApplyHealing(Combatant->Stats.HpMax * EffectiveDefinition.OnKillHealPercent);
	}
	return true;
}

bool AReEchoWeaponActor::FireStaffLightWave(const FReEchoEffectiveWeaponDefinition& Definition,
                                            const FReEchoCsvAttackStepRow& Step,
                                            UReEchoCombatantComponent* Combatant)
{
	AActor* WeaponOwner = GetOwner();
	if (!WeaponOwner)
	{
		return false;
	}
	const FVector OwnerLocation = WeaponOwner->GetActorLocation();
	FVector AimDirection = WeaponOwner->GetActorForwardVector().GetSafeNormal2D();
	if (AimDirection.IsNearlyZero())
	{
		AimDirection = FVector::ForwardVector;
	}
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
	const float Damage = ComputeStepDamage(Step, Combatant->Stats, false);
	const float Range = Step.RangeCm > 0.0f ? Step.RangeCm : Definition.Weapon.RangeCm;
	Wave->InitializeWave(AimDirection, Damage, OwnerLocation, Range);
	return true;
}

bool AReEchoWeaponActor::FireProjectile(const FReEchoEffectiveWeaponDefinition& Definition,
                                        const FReEchoCsvAttackStepRow& Step,
                                        UReEchoCombatantComponent* Combatant)
{
	AActor* WeaponOwner = GetOwner();
	if (!WeaponOwner)
	{
		return false;
	}
	const FVector OwnerLocation = WeaponOwner->GetActorLocation();
	FVector AimDirection = WeaponOwner->GetActorForwardVector().GetSafeNormal();
	if (AimDirection.IsNearlyZero())
	{
		AimDirection = FVector::ForwardVector;
	}
	const int32 ProjectileCount = FMath::Max(Definition.Weapon.ProjectileCount, Step.ProjectileCount);
	const float SpreadDegrees =
	    Step.ConcentrationDegrees > 0.0f ? Step.ConcentrationDegrees : Definition.Weapon.ConcentrationDegrees;
	const TArray<FVector> Directions =
	    ReEchoWeaponRuntime::BuildProjectileDirections(AimDirection, ProjectileCount, SpreadDegrees);
	const float Range = Step.RangeCm > 0.0f ? Step.RangeCm : Definition.Weapon.RangeCm;
	const float ExplosionRadius =
	    Step.ExplosionRadiusCm > 0.0f ? Step.ExplosionRadiusCm : Definition.Weapon.ExplosionRadiusCm;
	const bool bElemental = Definition.bUsesCyclingElement || Definition.bUsesDeterministicRandomElement ||
	                        ReEchoWeaponRuntime::ElementFromDamageChannel(Definition.DamageChannelId, AttackSequence) !=
	                            EReEchoElement::None;
	const float Damage = ComputeStepDamage(Step, Combatant->Stats, bElemental);
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
		EReEchoElement Element = EReEchoElement::None;
		if (Definition.bUsesCyclingElement)
		{
			Element = ConsumeNextElement();
		}
		else
		{
			Element = ReEchoWeaponRuntime::ElementFromDamageChannel(Definition.DamageChannelId, AttackSequence);
		}
		Projectile->InitializeProjectile(Direction,
		                                 Damage,
		                                 OwnerLocation,
		                                 ReEchoElementReaction::GetElementColor(Element),
		                                 Element,
		                                 Combatant->Stats.ReactionEfficiency,
		                                 ExplosionRadius,
		                                 Range);
		bSpawnedAny = true;
	}
	return bSpawnedAny;
}

EReEchoElement AReEchoWeaponActor::PeekNextElement() const
{
	switch (NextElementIndex % 12)
	{
		case 0:
			return EReEchoElement::Water;
		case 1:
			return EReEchoElement::Flame;
		case 2:
			return EReEchoElement::Grass;
		case 3:
			return EReEchoElement::Flame;
		case 4:
			return EReEchoElement::Water;
		case 5:
			return EReEchoElement::Lightning;
		case 6:
			return EReEchoElement::Grass;
		case 7:
			return EReEchoElement::Lightning;
		case 8:
			return EReEchoElement::Water;
		case 9:
			return EReEchoElement::Grass;
		case 10:
			return EReEchoElement::Grass;
		default:
			return EReEchoElement::Water;
	}
}

void AReEchoWeaponActor::UpdateElementIndicator()
{
	if (!ElementIndicator)
	{
		return;
	}
	const EReEchoElement Element = PeekNextElement();
	ElementIndicator->SetText(
	    FText::FromString(FString::Printf(TEXT("NEXT: %s"), *ReEchoElementReaction::GetElementLabel(Element))));
	ElementIndicator->SetTextRenderColor(ReEchoElementReaction::GetElementColor(Element).ToFColor(false));
}

void AReEchoWeaponActor::RefreshVisualState()
{
	const FReEchoCsvWeaponRow* Definition = FindEquippedDefinition();
	const FName VisualKey = Definition ? Definition->VisualKey : NAME_None;
	const bool bShowStaff = VisualKey == TEXT("MoonStaff");
	const bool bShowSword = VisualKey == TEXT("CrescentBlade");
	const bool bShowElement = VisualKey == TEXT("ElementalOrb");
	if (StaffSprite)
	{
		StaffSprite->SetVisibility(bShowStaff);
	}
	if (SwordSprite)
	{
		SwordSprite->SetVisibility(bShowSword);
	}
	if (ElementIndicator)
	{
		ElementIndicator->SetVisibility(bShowElement);
	}
}

EReEchoElement AReEchoWeaponActor::ConsumeNextElement()
{
	const EReEchoElement Element = PeekNextElement();
	NextElementIndex = (NextElementIndex + 1) % 12;
	UpdateElementIndicator();
	return Element;
}

bool AReEchoWeaponActor::SwingMelee(const FReEchoEffectiveWeaponDefinition& Definition,
                                    const FReEchoCsvAttackStepRow& Step,
                                    UReEchoCombatantComponent* Combatant)
{
	AActor* WeaponOwner = GetOwner();
	if (!WeaponOwner)
	{
		return false;
	}
	const FVector OwnerLocation = WeaponOwner->GetActorLocation();
	FVector AimDirection = WeaponOwner->GetActorForwardVector().GetSafeNormal2D();
	if (AimDirection.IsNearlyZero())
	{
		AimDirection = FVector::ForwardVector;
	}
	const EReEchoElement Element =
	    ReEchoWeaponRuntime::ElementFromDamageChannel(Definition.DamageChannelId, AttackSequence);
	const float Damage = ComputeStepDamage(Step, Combatant->Stats, Element != EReEchoElement::None);
	const float Range = Step.RangeCm > 0.0f ? Step.RangeCm : Definition.Weapon.RangeCm;
	const float ArcDegrees = Step.ArcDegrees > 0.0f ? Step.ArcDegrees : Definition.Weapon.ArcDegrees;
	// 旋转攻击以角色为圆心覆盖完整一周；敌人受伤逻辑会从圆心向外施加击退。
	for (TActorIterator<AReEchoEnemyActor> It(GetWorld()); It; ++It)
	{
		if (It->IsAlive() && ReEchoWeaponRuntime::IsInsideMeleeArc(
		                         OwnerLocation, AimDirection, It->GetActorLocation(), Range, ArcDegrees))
		{
			ApplyDamageToEnemy(**It, Damage, OwnerLocation, WeaponOwner, Combatant, Element);
		}
	}
	StartSwordAnimation();
	return true;
}

float AReEchoWeaponActor::ApplyRoleDamageModifiers(const float BaseDamage, const FReEchoStatBlock& Stats)
{
	++AttackSequence;
	float Damage = FMath::Max(0.0f, BaseDamage);
	if (Stats.RoleId == TEXT("Hunter"))
	{
		CriticalAccumulator += FMath::Clamp(Stats.CriticalRate, 0.0f, 1.0f);
		if (CriticalAccumulator >= 1.0f)
		{
			CriticalAccumulator -= 1.0f;
			Damage *= 1.0f + FMath::Max(0.0f, Stats.CriticalEffect);
		}
	}
	if (Stats.RoleId == TEXT("Brave") && AttackSequence % 2 == 0)
	{
		Damage *= 1.0f + FMath::Max(0.0f, Stats.EverySecondAttackBonus);
	}
	return Damage;
}

void AReEchoWeaponActor::StartSwordAnimation()
{
	SwordSwingDirection *= -1.0f;
	SwordAnimationTime = SwordAnimationDuration;
	SpawnSwordArc();
}

void AReEchoWeaponActor::SpawnSwordArc()
{
	AActor* WeaponOwner = GetOwner();
	if (!WeaponOwner)
	{
		return;
	}
	const FVector ArcLocation = WeaponOwner->GetActorLocation() + FVector(-12.0f, 0.0f, 42.0f);
	if (AReEchoSwordArcActor* SwordArc =
	        GetWorld()->SpawnActor<AReEchoSwordArcActor>(ArcLocation, WeaponOwner->GetActorRotation()))
	{
		SwordArc->SetOwner(WeaponOwner);
		SwordArc->InitializeArc(SwordSwingDirection);
	}
}

void AReEchoWeaponActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AttackCooldown = FMath::Max(0.0f, AttackCooldown - DeltaSeconds);
	StepLockRemaining = FMath::Max(0.0f, StepLockRemaining - DeltaSeconds);
	InvulnerableRemaining = FMath::Max(0.0f, InvulnerableRemaining - DeltaSeconds);
	if (ElementIndicator && ElementIndicator->IsVisible())
	{
		if (APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
		{
			ElementIndicator->SetWorldRotation((-Camera->GetCameraRotation().Vector()).Rotation());
		}
	}
	const FReEchoCsvWeaponRow* Definition = FindEquippedDefinition();
	const bool bUsesSwordVisual = Definition && Definition->VisualKey == TEXT("CrescentBlade");
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
