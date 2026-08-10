#include "Graybox/ReEchoEchoActor.h"

#include "Combat/ReEchoCombatantComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "Graybox/ReEchoEnemyActor.h"
#include "Graybox/ReEchoBillboardDebug.h"
#include "Graybox/ReEchoTrajectoryActor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Recording/ReEchoPlaybackComponent.h"
#include "Weapons/ReEchoWeaponActor.h"
#include "UObject/ConstructorHelpers.h"

AReEchoEchoActor::AReEchoEchoActor()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	GroundShadow = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GroundShadow"));
	GroundShadow->SetupAttachment(RootComponent);
	GroundShadow->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GroundShadow->SetCastShadow(false);
	GroundShadow->SetTranslucentSortPriority(-1);
	GroundShadow->SetAbsolute(false, false, true);
	GroundShadow->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")));
	GroundShadow->SetRelativeLocation(FVector(0.0f, 0.0f, -224.0f * 0.28f));
	GroundShadow->SetRelativeScale3D(FVector(0.512f, 0.5376f, 1.0f));
	if (UMaterialInterface* ShadowBase = LoadObject<UMaterialInterface>(
	        nullptr, TEXT("/Paper2D/TranslucentUnlitSpriteMaterial.TranslucentUnlitSpriteMaterial")))
	{
		UMaterialInstanceDynamic* ShadowMaterial = UMaterialInstanceDynamic::Create(ShadowBase, this);
		ShadowMaterial->SetTextureParameterValue(
		    TEXT("SpriteTexture"),
		    LoadObject<UTexture2D>(nullptr,
		                           TEXT("/Game/ReEcho/Textures/Characters/SoftGroundShadow.SoftGroundShadow")));
		GroundShadow->SetMaterial(0, ShadowMaterial);
	}
	CharacterSprite = CreateDefaultSubobject<UBillboardComponent>(TEXT("CharacterSprite"));
	CharacterSprite->SetupAttachment(RootComponent);
	CharacterSprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CharacterSprite->SetHiddenInGame(false);
	CharacterSprite->SetVisibility(true);
	constexpr float CharacterWorldHeight = 224.0f;
	CharacterSprite->SetRelativeLocation(FVector::ZeroVector);
	CharacterSprite->bIsScreenSizeScaled = false;
	static ConstructorHelpers::FObjectFinder<UTexture2D> CatTextureFinder(
	    TEXT("/Game/ReEcho/Textures/Characters/NewCast/Echo_Cat.Echo_Cat"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> HeartTextureFinder(
	    TEXT("/Game/ReEcho/Textures/Characters/NewCast/Echo_Heart.Echo_Heart"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> SpadeTextureFinder(
	    TEXT("/Game/ReEcho/Textures/Characters/NewCast/Echo_Spade.Echo_Spade"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> CloverTextureFinder(
	    TEXT("/Game/ReEcho/Textures/Characters/NewCast/Echo_Clover.Echo_Clover"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> DiamondTextureFinder(
	    TEXT("/Game/ReEcho/Textures/Characters/NewCast/Echo_Diamond.Echo_Diamond"));
	EchoTextures.Add(TEXT("J_CAT"), CatTextureFinder.Object);
	EchoTextures.Add(TEXT("J_HEART"), HeartTextureFinder.Object);
	EchoTextures.Add(TEXT("J_SPADE"), SpadeTextureFinder.Object);
	EchoTextures.Add(TEXT("J_CLOVER"), CloverTextureFinder.Object);
	EchoTextures.Add(TEXT("J_DIAMOND"), DiamondTextureFinder.Object);
	ConfigureEchoAppearance(TEXT("J_CAT"));
	BaseSpriteLocation = CharacterSprite->GetRelativeLocation();
	BaseSpriteScale = CharacterSprite->GetRelativeScale3D();

	Playback = CreateDefaultSubobject<UReEchoPlaybackComponent>(TEXT("Playback"));
	Combatant = CreateDefaultSubobject<UReEchoCombatantComponent>(TEXT("Combatant"));
}

void AReEchoEchoActor::InitializeEcho(const FReEchoRecording& Recording, const float Efficiency)
{
	ConfigureEchoAppearance(Recording.BuildSnapshot.CharacterId);
	Playback->LoadRecording(Recording);

	for (TActorIterator<AReEchoTrajectoryActor> TrajectoryIterator(GetWorld()); TrajectoryIterator;
	     ++TrajectoryIterator)
	{
		TrajectoryIterator->Destroy();
	}

	Trajectory = GetWorld()->SpawnActor<AReEchoTrajectoryActor>();
	if (Trajectory)
	{
		Trajectory->SetOwner(this);
		Trajectory->InitializeTrajectory(Recording);
	}

	FReEchoStatBlock EchoStats = Recording.BuildSnapshot.Stats;
	EchoStats.PhysicalAttack = FMath::Max(1.0f, EchoStats.PhysicalAttack * Efficiency);
	EchoStats.ElementalAttack = FMath::Max(1.0f, EchoStats.ElementalAttack * Efficiency);
	Combatant->InitializeFromStats(EchoStats, true);

	Weapon = GetWorld()->SpawnActor<AReEchoWeaponActor>();
	if (Weapon)
	{
		Weapon->SetOwner(this);
		Weapon->AttachToActor(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		Weapon->SetActorRelativeLocation(FVector::ZeroVector);
		Weapon->InitializeWeapon();
		Weapon->ConfigureLockedWeapon(Recording.BuildSnapshot.WeaponId);
	}
}

bool AReEchoEchoActor::ConfigureEchoAppearance(const FName CharacterId)
{
	const TObjectPtr<UTexture2D>* TextureEntry = EchoTextures.Find(CharacterId);
	if (!TextureEntry || !TextureEntry->Get())
	{
		return false;
	}

	UTexture2D* Texture = TextureEntry->Get();
	constexpr float CharacterWorldHeight = 224.0f;
	CharacterSprite->SetSprite(Texture);
	const float TextureScale = CharacterWorldHeight / FMath::Max(1, Texture->GetSizeY());
	CharacterSprite->SetRelativeScale3D(FVector(TextureScale));
	BaseSpriteLocation = CharacterSprite->GetRelativeLocation();
	BaseSpriteScale = CharacterSprite->GetRelativeScale3D();
	return true;
}

void AReEchoEchoActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Weapon)
	{
		Weapon->Destroy();
		Weapon = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

const FReEchoStatBlock& AReEchoEchoActor::GetCurrentStats() const
{
	return Combatant->Stats;
}

float AReEchoEchoActor::GetCurrentHealth() const
{
	return Combatant->CurrentHealth;
}

void AReEchoEchoActor::AdvanceEcho(const float EncounterTime)
{
	Playback->AdvancePlayback(EncounterTime);
}

void AReEchoEchoActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ReEchoBillboardDebug::DrawBounds(this, CharacterSprite, FColor(180, 70, 255));

	VisualTime += DeltaSeconds;
	AttackVisualRemaining = FMath::Max(0.0f, AttackVisualRemaining - DeltaSeconds);
	const float Bob = FMath::Sin(VisualTime * 3.2f) * 2.5f;
	const float AttackPulse =
	    AttackVisualRemaining > 0.0f ? FMath::Sin((1.0f - AttackVisualRemaining / 0.2f) * PI) : 0.0f;
	CharacterSprite->SetRelativeLocation(BaseSpriteLocation + FVector(AttackPulse * 18.0f, 0.0f, Bob));
	CharacterSprite->SetRelativeScale3D(BaseSpriteScale *
	                                    FVector(1.0f + AttackPulse * 0.07f, 1.0f - AttackPulse * 0.03f, 1.0f));

	if (!Weapon)
	{
		return;
	}

	AReEchoEnemyActor* NearestEnemy = nullptr;
	float NearestDistanceSquared = FMath::Square(AutoTargetRange);
	for (TActorIterator<AReEchoEnemyActor> EnemyIterator(GetWorld()); EnemyIterator; ++EnemyIterator)
	{
		if (!EnemyIterator->IsAlive())
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared2D(GetActorLocation(), EnemyIterator->GetActorLocation());
		if (DistanceSquared < NearestDistanceSquared)
		{
			NearestDistanceSquared = DistanceSquared;
			NearestEnemy = *EnemyIterator;
		}
	}

	if (!NearestEnemy)
	{
		return;
	}

	const FVector AimDirection = (NearestEnemy->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	if (!AimDirection.IsNearlyZero())
	{
		SetActorRotation(AimDirection.Rotation());
	}

	if (Weapon->TryBasicAttack(Combatant))
	{
		AttackVisualRemaining = 0.2f;
	}
}
