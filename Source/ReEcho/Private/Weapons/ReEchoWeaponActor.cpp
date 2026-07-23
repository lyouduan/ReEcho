#include "Weapons/ReEchoWeaponActor.h"

#include "Combat/ReEchoCombatantComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Graybox/ReEchoEnemyActor.h"
#include "Graybox/ReEchoProjectileActor.h"
#include "Graybox/ReEchoSwordArcActor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
const TMap<EReEchoWeaponSlot, FName> WeaponIds = {
	{EReEchoWeaponSlot::PhysicalOrb, TEXT("W_J_02")},
	{EReEchoWeaponSlot::Sword, TEXT("W_J_01")},
	{EReEchoWeaponSlot::ElementalOrb, TEXT("W_J_03")}
};

void ApplyShapeColor(UStaticMeshComponent* Mesh, const FLinearColor& Color)
{
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(
		nullptr,
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (!Base)
	{
		return;
	}

	UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Base, Mesh);
	Material->SetVectorParameterValue(TEXT("Color"), Color);
	Mesh->SetMaterial(0, Material);
}
}

AReEchoWeaponActor::AReEchoWeaponActor()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(
		nullptr,
		TEXT("/Engine/BasicShapes/Cube.Cube"));

	SwordBlade = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SwordBlade"));
	SwordBlade->SetupAttachment(Root);
	SwordBlade->SetStaticMesh(CubeMesh);
	SwordBlade->SetRelativeLocation(FVector(72.0f, 0.0f, 10.0f));
	SwordBlade->SetRelativeScale3D(FVector(0.72f, 0.065f, 0.07f));
	SwordBlade->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ApplyShapeColor(SwordBlade, FLinearColor(0.75f, 0.85f, 1.0f));

	SwordGuard = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SwordGuard"));
	SwordGuard->SetupAttachment(Root);
	SwordGuard->SetStaticMesh(CubeMesh);
	SwordGuard->SetRelativeLocation(FVector(24.0f, 0.0f, 10.0f));
	SwordGuard->SetRelativeScale3D(FVector(0.08f, 0.28f, 0.09f));
	SwordGuard->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ApplyShapeColor(SwordGuard, FLinearColor(0.95f, 0.55f, 0.08f));

	SetActorEnableCollision(false);
}

void AReEchoWeaponActor::InitializeWeapon()
{
	SwordRestLocation = Root->GetRelativeLocation();

	for (const TPair<EReEchoWeaponSlot, FName>& Pair : WeaponIds)
	{
		FWeaponDefinition Definition;
		if (LoadWeaponDefinition(Pair.Value, Definition))
		{
			Definitions.Add(Pair.Key, Definition);
		}
	}

	SelectWeapon(EReEchoWeaponSlot::PhysicalOrb);
}

void AReEchoWeaponActor::SelectWeapon(const EReEchoWeaponSlot NewSlot)
{
	if (!Definitions.Contains(NewSlot))
	{
		return;
	}

	EquippedSlot = NewSlot;
	AttackCooldown = 0.0f;
	const bool bSwordVisible = EquippedSlot == EReEchoWeaponSlot::Sword;
	SwordBlade->SetVisibility(bSwordVisible);
	SwordGuard->SetVisibility(bSwordVisible);
}

bool AReEchoWeaponActor::SelectWeaponById(const FName WeaponId)
{
	if (GetEquippedWeaponId() == WeaponId)
	{
		return true;
	}

	for (const TPair<EReEchoWeaponSlot, FWeaponDefinition>& Pair : Definitions)
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

	const FWeaponDefinition* Definition = Definitions.Find(EquippedSlot);
	if (!Definition)
	{
		return false;
	}

	bool bAttacked = false;
	switch (EquippedSlot)
	{
	case EReEchoWeaponSlot::PhysicalOrb:
		bAttacked = FireProjectile(
			*Definition,
			Combatant,
			FLinearColor(1.0f, 0.01f, 0.005f));
		break;

	case EReEchoWeaponSlot::Sword:
		bAttacked = SwingSword(*Definition, Combatant);
		break;

	case EReEchoWeaponSlot::ElementalOrb:
		bAttacked = FireProjectile(
			*Definition,
			Combatant,
			FLinearColor(0.15f, 0.45f, 1.0f));
		break;
	}

	if (bAttacked)
	{
		AttackCooldown =
			Definition->Interval / FMath::Max(0.1f, Combatant->Stats.AttackSpeed);
	}

	return bAttacked;
}

bool AReEchoWeaponActor::TryActiveAttack(UReEchoCombatantComponent* Combatant)
{
	return TryBasicAttack(Combatant);
}
FName AReEchoWeaponActor::GetEquippedWeaponId() const
{
	if (const FWeaponDefinition* Definition = Definitions.Find(EquippedSlot))
	{
		return Definition->WeaponId;
	}

	return NAME_None;
}

FString AReEchoWeaponActor::GetEquippedWeaponLabel() const
{
	switch (EquippedSlot)
	{
	case EReEchoWeaponSlot::PhysicalOrb:
		return TEXT("Physical Orb");

	case EReEchoWeaponSlot::Sword:
		return TEXT("Sword");

	case EReEchoWeaponSlot::ElementalOrb:
		return TEXT("Elemental Orb");
	}

	return TEXT("Unknown");
}

bool AReEchoWeaponActor::LoadWeaponDefinition(
	const FName WeaponId,
	FWeaponDefinition& OutDefinition) const
{
	FString JsonText;
	const FString WeaponsPath = FPaths::ProjectContentDir() / TEXT("Data/weapons.json");
	if (!FFileHelper::LoadFileToString(JsonText, *WeaponsPath))
	{
		UE_LOG(LogTemp, Error, TEXT("Unable to load weapons from %s"), *WeaponsPath);
		return false;
	}

	TArray<TSharedPtr<FJsonValue>> WeaponValues;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
	if (!FJsonSerializer::Deserialize(Reader, WeaponValues))
	{
		UE_LOG(LogTemp, Error, TEXT("Unable to parse weapons from %s"), *WeaponsPath);
		return false;
	}

	for (const TSharedPtr<FJsonValue>& WeaponValue : WeaponValues)
	{
		const TSharedPtr<FJsonObject> WeaponObject = WeaponValue->AsObject();
		if (!WeaponObject)
		{
			continue;
		}

		FString CandidateId;
		if (!WeaponObject->TryGetStringField(TEXT("id"), CandidateId)
			|| FName(CandidateId) != WeaponId)
		{
			continue;
		}

		OutDefinition.WeaponId = WeaponId;
		WeaponObject->TryGetNumberField(TEXT("interval"), OutDefinition.Interval);
		WeaponObject->TryGetNumberField(TEXT("range"), OutDefinition.Range);
		WeaponObject->TryGetNumberField(
			TEXT("physicalCoefficient"),
			OutDefinition.PhysicalCoefficient);
		WeaponObject->TryGetNumberField(
			TEXT("elementalCoefficient"),
			OutDefinition.ElementalCoefficient);
		WeaponObject->TryGetNumberField(TEXT("arcDegrees"), OutDefinition.ArcDegrees);
		return true;
	}

	return false;
}

bool AReEchoWeaponActor::FireProjectile(
	const FWeaponDefinition& Definition,
	UReEchoCombatantComponent* Combatant,
	const FLinearColor& Color)
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

	const FVector SpawnLocation =
		OwnerLocation + FVector(0.0f, 0.0f, 35.0f) + AimDirection * 45.0f;
	AReEchoProjectileActor* Projectile =
		GetWorld()->SpawnActor<AReEchoProjectileActor>(
			SpawnLocation,
			AimDirection.Rotation());
	if (!Projectile)
	{
		return false;
	}

	const float Damage =
		Combatant->Stats.PhysicalAttack * Definition.PhysicalCoefficient
		+ Combatant->Stats.ElementalAttack * Definition.ElementalCoefficient;
	Projectile->InitializeProjectile(AimDirection, Damage, OwnerLocation, Color);
	return true;
}

bool AReEchoWeaponActor::SwingSword(
	const FWeaponDefinition& Definition,
	UReEchoCombatantComponent* Combatant)
{
	AActor* WeaponOwner = GetOwner();
	if (!WeaponOwner)
	{
		return false;
	}

	const FVector OwnerLocation = WeaponOwner->GetActorLocation();
	FVector AttackDirection = WeaponOwner->GetActorForwardVector().GetSafeNormal2D();
	if (AttackDirection.IsNearlyZero())
	{
		AttackDirection = FVector::ForwardVector;
	}

	const float MinimumDot =
		FMath::Cos(FMath::DegreesToRadians(Definition.ArcDegrees * 0.5f));
	const float Damage =
		Combatant->Stats.PhysicalAttack * Definition.PhysicalCoefficient;

	for (TActorIterator<AReEchoEnemyActor> EnemyIterator(GetWorld()); EnemyIterator; ++EnemyIterator)
	{
		if (!EnemyIterator->IsAlive())
		{
			continue;
		}

		const FVector ToEnemy =
			(EnemyIterator->GetActorLocation() - OwnerLocation).GetSafeNormal2D();
		const float Distance = FVector::Dist2D(
			OwnerLocation,
			EnemyIterator->GetActorLocation());
		if (Distance <= Definition.Range
			&& FVector::DotProduct(AttackDirection, ToEnemy) >= MinimumDot)
		{
			EnemyIterator->ReceiveGrayboxDamage(Damage, OwnerLocation);
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

	const FVector ArcLocation =
		WeaponOwner->GetActorLocation() + FVector(0.0f, 0.0f, 42.0f);
	AReEchoSwordArcActor* SwordArc =
		GetWorld()->SpawnActor<AReEchoSwordArcActor>(
			ArcLocation,
			WeaponOwner->GetActorRotation());
	if (SwordArc)
	{
		SwordArc->SetOwner(WeaponOwner);
		SwordArc->InitializeArc(SwordSwingDirection);
	}
}

void AReEchoWeaponActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AttackCooldown = FMath::Max(0.0f, AttackCooldown - DeltaSeconds);

	if (SwordAnimationTime <= 0.0f)
	{
		if (EquippedSlot == EReEchoWeaponSlot::Sword)
		{
			SetActorRelativeLocation(SwordRestLocation);
			SetActorRelativeRotation(FRotator(0.0f, 25.0f, 0.0f));
		}
		return;
	}

	SwordAnimationTime = FMath::Max(0.0f, SwordAnimationTime - DeltaSeconds);
	const float Progress = 1.0f - SwordAnimationTime / SwordAnimationDuration;

	float SwingYaw = 25.0f;
	float SwingRoll = 0.0f;
	if (Progress < 0.18f)
	{
		const float Phase = Progress / 0.18f;
		const float EasedPhase =
			FMath::InterpEaseInOut(0.0f, 1.0f, Phase, 2.0f);
		SwingYaw =
			FMath::Lerp(-72.0f, -88.0f, EasedPhase) * SwordSwingDirection;
		SwingRoll =
			FMath::Lerp(8.0f, 20.0f, EasedPhase) * SwordSwingDirection;
	}
	else if (Progress < 0.72f)
	{
		const float Phase = (Progress - 0.18f) / 0.54f;
		const float EasedPhase =
			FMath::InterpEaseInOut(0.0f, 1.0f, Phase, 2.0f);
		SwingYaw =
			FMath::Lerp(-88.0f, 88.0f, EasedPhase) * SwordSwingDirection;
		SwingRoll =
			FMath::Lerp(20.0f, -14.0f, EasedPhase) * SwordSwingDirection;
	}
	else
	{
		const float Phase = (Progress - 0.72f) / 0.28f;
		const float EasedPhase =
			FMath::InterpEaseOut(0.0f, 1.0f, Phase, 2.0f);
		SwingYaw = FMath::Lerp(
			88.0f * SwordSwingDirection,
			25.0f,
			EasedPhase);
		SwingRoll = FMath::Lerp(
			-14.0f * SwordSwingDirection,
			0.0f,
			EasedPhase);
	}

	const float ForwardOffset =
		FMath::Sin(Progress * PI) * 24.0f;
	SetActorRelativeLocation(
		SwordRestLocation + FVector(ForwardOffset, 0.0f, 0.0f));
	SetActorRelativeRotation(FRotator(0.0f, SwingYaw, SwingRoll));
}