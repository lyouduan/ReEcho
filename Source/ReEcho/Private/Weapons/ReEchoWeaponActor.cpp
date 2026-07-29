#include "Weapons/ReEchoWeaponActor.h"

#include "Combat/ReEchoCombatantComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Math/RotationMatrix.h"
#include "EngineUtils.h"
#include "Graybox/ReEchoEnemyActor.h"
#include "Graybox/ReEchoStaffLightWaveActor.h"
#include "Graybox/ReEchoProjectileActor.h"
#include "Graybox/ReEchoSwordArcActor.h"

namespace ReEchoWeaponVisual
{
const FVector StaffLocation(-16.0f, 30.0f, 6.0f);
const FVector SwordLocation(8.0f, 0.0f, 0.0f);
constexpr float SwordRestAngleRadians = -PI / 4.0f;
const FVector CameraFacingNormal(-0.573576f, 0.0f, 0.819152f);

FQuat GetSwordRotation(const float SpinRadians = 0.0f)
{
	const FQuat CameraFacingRotation =
		FRotationMatrix::MakeFromZX(CameraFacingNormal, FVector::RightVector).ToQuat();
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

	StaffSprite = CreateDefaultSubobject<UBillboardComponent>(TEXT("StaffSprite"));
	StaffSprite->SetupAttachment(Root);
	StaffSprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StaffSprite->SetCastShadow(false);
	StaffSprite->SetTranslucentSortPriority(6);
	StaffSprite->SetRelativeLocation(ReEchoWeaponVisual::StaffLocation);
	StaffSprite->SetHiddenInGame(false);
	StaffSprite->bIsScreenSizeScaled = false;
	if (UTexture2D* StaffTexture = LoadObject<UTexture2D>(
		    nullptr, TEXT("/Game/ReEcho/Textures/Effects/MoonStaff.MoonStaff")))
	{
		StaffSprite->SetSprite(StaffTexture);
		constexpr float StaffWorldHeight = 250.0f;
		StaffSprite->SetRelativeScale3D(FVector(
			StaffWorldHeight / FMath::Max(1, StaffTexture->GetSizeY())));
	}

	// Billboard 会在渲染阶段覆盖组件旋转；使用透明 Plane 才能稳定显示武器自身的 360 度旋转。
	SwordSprite = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SwordSprite"));
	SwordSprite->SetupAttachment(Root);
	SwordSprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SwordSprite->SetCastShadow(false);
	SwordSprite->SetTranslucentSortPriority(-1);
	SwordSprite->SetRelativeLocation(ReEchoWeaponVisual::SwordLocation);
	SwordSprite->SetRelativeRotation(
		ReEchoWeaponVisual::GetSwordRotation(ReEchoWeaponVisual::SwordRestAngleRadians));
	SwordSprite->SetHiddenInGame(false);
	if (UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(
		    nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")))
	{
		SwordSprite->SetStaticMesh(PlaneMesh);
	}
	UMaterialInterface* SpriteMaterial = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Paper2D/TranslucentUnlitSpriteMaterial.TranslucentUnlitSpriteMaterial"));
	UTexture2D* WeaponTexture = LoadObject<UTexture2D>(
		nullptr, TEXT("/Game/ReEcho/Textures/Effects/CrescentWeapon.CrescentWeapon"));
	if (SpriteMaterial && WeaponTexture)
	{
		UMaterialInstanceDynamic* MaterialInstance =
			UMaterialInstanceDynamic::Create(SpriteMaterial, this);
		MaterialInstance->SetTextureParameterValue(TEXT("SpriteTexture"), WeaponTexture);
		SwordSprite->SetMaterial(0, MaterialInstance);
		constexpr float WeaponWorldWidth = 250.0f;
		const float AspectRatio = static_cast<float>(WeaponTexture->GetSizeY())
			/ FMath::Max(1, WeaponTexture->GetSizeX());
		SwordSprite->SetRelativeScale3D(FVector(
			WeaponWorldWidth / 100.0f,
			WeaponWorldWidth * AspectRatio / 100.0f,
			1.0f));
	}
	SetActorEnableCollision(false);
}

void AReEchoWeaponActor::InitializeWeapon()
{
	SwordSpriteRestLocation = ReEchoWeaponVisual::SwordLocation;
	Definitions.Reset();
	const UReEchoBalanceSettings* Settings = GetDefault<UReEchoBalanceSettings>();
	for (const FReEchoWeaponConfig& Definition : Settings->Weapons)
	{
		if (Definition.Slot == EReEchoWeaponSlot::None || Definition.WeaponId.IsNone())
		{
			UE_LOG(LogTemp, Warning, TEXT("Ignoring weapon config with an empty slot or id"));
			continue;
		}
		if (Definitions.Contains(Definition.Slot))
		{
			UE_LOG(LogTemp, Warning, TEXT("Duplicate weapon slot %d; last config wins"), static_cast<int32>(Definition.Slot));
		}
		Definitions.Add(Definition.Slot, Definition);
	}
	if (Definitions.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("No weapons configured in ReEcho Balance settings"));
		StaffSprite->SetVisibility(false);
		SwordSprite->SetVisibility(false);
		return;
	}
	const EReEchoWeaponSlot InitialSlot = Definitions.Contains(EReEchoWeaponSlot::PhysicalOrb)
		? EReEchoWeaponSlot::PhysicalOrb
		: Definitions.CreateConstIterator().Key();
	SelectWeapon(InitialSlot);
}

void AReEchoWeaponActor::SelectWeapon(const EReEchoWeaponSlot NewSlot)
{
	if (!Definitions.Contains(NewSlot))
	{
		return;
	}
	EquippedSlot = NewSlot;
	AttackCooldown = 0.0f;
	SwordAnimationTime = 0.0f;
	StaffSprite->SetVisibility(EquippedSlot == EReEchoWeaponSlot::PhysicalOrb);
	SwordSprite->SetVisibility(EquippedSlot == EReEchoWeaponSlot::Sword);
	SwordSprite->SetRelativeLocation(SwordSpriteRestLocation);
	SwordSprite->SetRelativeRotation(
		ReEchoWeaponVisual::GetSwordRotation(ReEchoWeaponVisual::SwordRestAngleRadians));
}

bool AReEchoWeaponActor::SelectWeaponById(const FName WeaponId)
{
	if (GetEquippedWeaponId() == WeaponId)
	{
		return true;
	}
	for (const TPair<EReEchoWeaponSlot, FReEchoWeaponConfig>& Pair : Definitions)
	{
		if (Pair.Value.WeaponId == WeaponId)
		{
			SelectWeapon(Pair.Key);
			return true;
		}
	}
	return false;
}

bool AReEchoWeaponActor::TryBasicAttack(UReEchoCombatantComponent* Combatant)
{
	if (!Combatant || AttackCooldown > 0.0f)
	{
		return false;
	}
	const FReEchoWeaponConfig* Definition = Definitions.Find(EquippedSlot);
	if (!Definition)
	{
		return false;
	}
	bool bAttacked = false;
	switch (EquippedSlot)
	{
	case EReEchoWeaponSlot::PhysicalOrb:
		bAttacked = FireStaffLightWave(*Definition, Combatant);
		break;
	case EReEchoWeaponSlot::Sword:
		bAttacked = SwingSword(*Definition, Combatant);
		break;
	case EReEchoWeaponSlot::ElementalOrb:
		bAttacked = FireProjectile(*Definition, Combatant);
		break;
	default:
		break;
	}
	if (bAttacked)
	{
		AttackCooldown = Definition->Interval / FMath::Max(0.1f, Combatant->Stats.AttackSpeed);
	}
	return bAttacked;
}

bool AReEchoWeaponActor::TryActiveAttack(UReEchoCombatantComponent* Combatant)
{
	return TryBasicAttack(Combatant);
}

FName AReEchoWeaponActor::GetEquippedWeaponId() const
{
	if (const FReEchoWeaponConfig* Definition = Definitions.Find(EquippedSlot))
	{
		return Definition->WeaponId;
	}
	return NAME_None;
}

FString AReEchoWeaponActor::GetEquippedWeaponLabel() const
{
	if (const FReEchoWeaponConfig* Definition = Definitions.Find(EquippedSlot))
	{
		return Definition->DisplayName.IsEmpty() ? Definition->WeaponId.ToString() : Definition->DisplayName.ToString();
	}
	return TEXT("Unknown");
}

bool AReEchoWeaponActor::FireStaffLightWave(
	const FReEchoWeaponConfig& Definition,
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
	const FVector StaffHeadLocation =
		StaffSprite->GetComponentLocation() + CameraUp * 45.0f;
	const FVector SpawnLocation = StaffHeadLocation + AimDirection * 18.0f;
	AReEchoStaffLightWaveActor* Wave = GetWorld()->SpawnActor<AReEchoStaffLightWaveActor>(
		SpawnLocation, FRotator::ZeroRotator);
	if (!Wave)
	{
		return false;
	}
	Wave->SetOwner(WeaponOwner);
	const float Damage = Combatant->Stats.PhysicalAttack * Definition.PhysicalCoefficient
		+ Combatant->Stats.ElementalAttack * Definition.ElementalCoefficient;
	Wave->InitializeWave(AimDirection, Damage, OwnerLocation, Definition.Range);
	return true;
}

bool AReEchoWeaponActor::FireProjectile(
	const FReEchoWeaponConfig& Definition,
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
	const FVector SpawnLocation = OwnerLocation + FVector(0.0f, 0.0f, 35.0f) + AimDirection * 45.0f;
	AReEchoProjectileActor* Projectile = GetWorld()->SpawnActor<AReEchoProjectileActor>(SpawnLocation, AimDirection.Rotation());
	if (!Projectile)
	{
		return false;
	}
	const float Damage = Combatant->Stats.PhysicalAttack * Definition.PhysicalCoefficient
		+ Combatant->Stats.ElementalAttack * Definition.ElementalCoefficient;
	Projectile->InitializeProjectile(AimDirection, Damage, OwnerLocation, Definition.ProjectileColor);
	return true;
}

bool AReEchoWeaponActor::SwingSword(
	const FReEchoWeaponConfig& Definition,
	UReEchoCombatantComponent* Combatant)
{
	AActor* WeaponOwner = GetOwner();
	if (!WeaponOwner)
	{
		return false;
	}
	const FVector OwnerLocation = WeaponOwner->GetActorLocation();
	const float Damage = Combatant->Stats.PhysicalAttack * Definition.PhysicalCoefficient;
	// 旋转攻击以角色为圆心覆盖完整一周；敌人受伤逻辑会从圆心向外施加击退。
	for (TActorIterator<AReEchoEnemyActor> It(GetWorld()); It; ++It)
	{
		if (It->IsAlive()
			&& FVector::Dist2D(OwnerLocation, It->GetActorLocation()) <= Definition.Range)
		{
			It->ReceiveGrayboxDamage(Damage, OwnerLocation);
		}
	}
	StartSwordAnimation();
	return true;
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
	if (AReEchoSwordArcActor* SwordArc = GetWorld()->SpawnActor<AReEchoSwordArcActor>(
		    ArcLocation, WeaponOwner->GetActorRotation()))
	{
		SwordArc->SetOwner(WeaponOwner);
		SwordArc->InitializeArc(SwordSwingDirection);
	}
}

void AReEchoWeaponActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AttackCooldown = FMath::Max(0.0f, AttackCooldown - DeltaSeconds);
	if (SwordAnimationTime <= 0.0f || EquippedSlot != EReEchoWeaponSlot::Sword)
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
	SwordSprite->SetRelativeRotation(ReEchoWeaponVisual::GetSwordRotation(
		ReEchoWeaponVisual::SwordRestAngleRadians + Angle));
}