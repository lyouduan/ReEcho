#include "Weapons/ReEchoWeaponActor.h"

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

void AReEchoWeaponActor::InitializeWeapon()
{
	SwordSpriteRestLocation = ReEchoWeaponVisual::SwordLocation;
	NextElementIndex = 0;
	AttackSequence = 0;
	CriticalAccumulator = 0.0f;
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
			UE_LOG(LogTemp,
			       Warning,
			       TEXT("Duplicate weapon slot %d; last config wins"),
			       static_cast<int32>(Definition.Slot));
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
	ElementIndicator->SetVisibility(EquippedSlot == EReEchoWeaponSlot::ElementalOrb);
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
	const FReEchoWeaponConfig* Definition = Definitions.Find(EquippedSlot);
	return Definition && Combatant ? Definition->Interval / FMath::Max(0.1f, Combatant->Stats.AttackSpeed) : 0.55f;
}

bool AReEchoWeaponActor::ExecuteAttack(UReEchoCombatantComponent* Combatant)
{
	const FReEchoWeaponConfig* Definition = Definitions.Find(EquippedSlot);
	if (!Combatant || !Definition)
	{
		return false;
	}
	switch (EquippedSlot)
	{
		case EReEchoWeaponSlot::PhysicalOrb:
			return FireStaffLightWave(*Definition, Combatant);
		case EReEchoWeaponSlot::Sword:
			return SwingSword(*Definition, Combatant);
		case EReEchoWeaponSlot::ElementalOrb:
			return FireProjectile(*Definition, Combatant);
		default:
			return false;
	}
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

bool AReEchoWeaponActor::FireStaffLightWave(const FReEchoWeaponConfig& Definition, UReEchoCombatantComponent* Combatant)
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
	const float BaseDamage = Combatant->Stats.PhysicalAttack * Definition.PhysicalCoefficient +
	                         Combatant->Stats.ElementalAttack * Definition.ElementalCoefficient;
	const float Damage = ApplyRoleDamageModifiers(BaseDamage, Combatant->Stats);
	Wave->InitializeWave(AimDirection, Damage, OwnerLocation, Definition.Range);
	return true;
}

bool AReEchoWeaponActor::FireProjectile(const FReEchoWeaponConfig& Definition, UReEchoCombatantComponent* Combatant)
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
	AReEchoProjectileActor* Projectile =
	    GetWorld()->SpawnActor<AReEchoProjectileActor>(SpawnLocation, AimDirection.Rotation());
	if (!Projectile)
	{
		return false;
	}
	const float BaseDamage = Combatant->Stats.PhysicalAttack * Definition.PhysicalCoefficient +
	                         Combatant->Stats.ElementalAttack * Definition.ElementalCoefficient;
	const float Damage = ApplyRoleDamageModifiers(BaseDamage, Combatant->Stats);
	Projectile->SetOwner(WeaponOwner);
	EReEchoElement Element = ConsumeNextElement();
	if (Combatant->Stats.bRandomElementProjectiles)
	{
		switch ((AttackSequence * 17 + 5) % 4)
		{
			case 0:
				Element = EReEchoElement::Water;
				break;
			case 1:
				Element = EReEchoElement::Flame;
				break;
			case 2:
				Element = EReEchoElement::Lightning;
				break;
			default:
				Element = EReEchoElement::Grass;
				break;
		}
	}
	Projectile->InitializeProjectile(AimDirection,
	                                 Damage,
	                                 OwnerLocation,
	                                 ReEchoElementReaction::GetElementColor(Element),
	                                 Element,
	                                 Combatant->Stats.ReactionEfficiency);
	return true;
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

EReEchoElement AReEchoWeaponActor::ConsumeNextElement()
{
	const EReEchoElement Element = PeekNextElement();
	NextElementIndex = (NextElementIndex + 1) % 12;
	UpdateElementIndicator();
	return Element;
}

bool AReEchoWeaponActor::SwingSword(const FReEchoWeaponConfig& Definition, UReEchoCombatantComponent* Combatant)
{
	AActor* WeaponOwner = GetOwner();
	if (!WeaponOwner)
	{
		return false;
	}
	const FVector OwnerLocation = WeaponOwner->GetActorLocation();
	const float Damage =
	    ApplyRoleDamageModifiers(Combatant->Stats.PhysicalAttack * Definition.PhysicalCoefficient, Combatant->Stats);
	// 旋转攻击以角色为圆心覆盖完整一周；敌人受伤逻辑会从圆心向外施加击退。
	for (TActorIterator<AReEchoEnemyActor> It(GetWorld()); It; ++It)
	{
		if (It->IsAlive() && FVector::Dist2D(OwnerLocation, It->GetActorLocation()) <= Definition.Range)
		{
			It->ReceiveGrayboxDamage(Damage, OwnerLocation, WeaponOwner);
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
	if (ElementIndicator && ElementIndicator->IsVisible())
	{
		if (APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
		{
			ElementIndicator->SetWorldRotation((-Camera->GetCameraRotation().Vector()).Rotation());
		}
	}
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
	SwordSprite->SetRelativeRotation(
	    ReEchoWeaponVisual::GetSwordRotation(ReEchoWeaponVisual::SwordRestAngleRadians + Angle));
}
