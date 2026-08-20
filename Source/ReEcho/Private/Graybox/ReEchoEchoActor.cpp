#include "Graybox/ReEchoEchoActor.h"

#include "Combat/ReEchoCombatantComponent.h"
#include "Combat/ReEchoCombatContracts.h"
#include "Combat/ReEchoCombatAudioAdapterComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Graybox/ReEchoEnemyActor.h"
#include "Graybox/ReEchoBillboardDebug.h"
#include "Graybox/ReEchoTrajectoryActor.h"
#include "Materials/MaterialInterface.h"
#include "Recording/ReEchoPlaybackComponent.h"
#include "Run/ReEchoRunSubsystem.h"
#include "Player/ReEchoPlayerPawn.h"
#include "Presentation/VFX/ReEchoCombatVfxComponent.h"
#include "ReEcho.h"
#include "ReEchoAudioEvents.h"
#include "Weapons/ReEchoWeaponActor.h"
#include "UObject/ConstructorHelpers.h"

AReEchoEchoActor::AReEchoEchoActor()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	EffectsRoot = CreateDefaultSubobject<USceneComponent>(TEXT("EffectsRoot"));
	EffectsRoot->SetupAttachment(RootComponent);
	AttackVfxRoot = CreateDefaultSubobject<USceneComponent>(TEXT("AttackVfxRoot"));
	AttackVfxRoot->SetupAttachment(EffectsRoot);
	AttackVfxRoot->bEditableWhenInherited = true;
	HurtVfxRoot = CreateDefaultSubobject<USceneComponent>(TEXT("HurtVfxRoot"));
	HurtVfxRoot->SetupAttachment(EffectsRoot);
	HurtVfxRoot->bEditableWhenInherited = true;
	GroundShadow = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GroundShadow"));
	GroundShadow->SetupAttachment(RootComponent);
	GroundShadow->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GroundShadow->SetCastShadow(false);
	GroundShadow->SetTranslucentSortPriority(-1);
	GroundShadow->SetAbsolute(false, false, true);
	GroundShadow->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")));
	GroundShadow->SetRelativeLocation(FVector(0.0f, 0.0f, -224.0f * 0.28f));
	GroundShadow->SetRelativeScale3D(FVector(0.512f, 0.5376f, 1.0f));
	if (UMaterialInterface* ShadowMaterial = LoadObject<UMaterialInterface>(
	        nullptr, TEXT("/Game/ReEcho/Materials/M_GroundShadow_Procedural.M_GroundShadow_Procedural")))
	{
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
	static ConstructorHelpers::FObjectFinder<UTexture2D> HeartTextureFinder(
	    TEXT("/Game/ReEcho/Textures/Characters/NewCast/Player_Heart.Player_Heart"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> SpadeTextureFinder(
	    TEXT("/Game/ReEcho/Art/Animation2D/Players/Spade/Walk/Textures/Idel_01.Idel_01"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> CloverTextureFinder(
	    TEXT("/Game/ReEcho/Textures/Characters/NewCast/Player_Clover.Player_Clover"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> DiamondTextureFinder(
	    TEXT("/Game/ReEcho/Textures/Characters/NewCast/Player_Diamond.Player_Diamond"));
	EchoTextures.Add(TEXT("J_HEART"), HeartTextureFinder.Object);
	EchoTextures.Add(TEXT("J_SPADE"), SpadeTextureFinder.Object);
	EchoTextures.Add(TEXT("J_CLOVER"), CloverTextureFinder.Object);
	EchoTextures.Add(TEXT("J_DIAMOND"), DiamondTextureFinder.Object);
	ConfigureEchoAppearance(TEXT("J_SPADE"));
	BaseSpriteLocation = CharacterSprite->GetRelativeLocation();
	BaseSpriteScale = CharacterSprite->GetRelativeScale3D();

	Playback = CreateDefaultSubobject<UReEchoPlaybackComponent>(TEXT("Playback"));
	Combatant = CreateDefaultSubobject<UReEchoCombatantComponent>(TEXT("Combatant"));
	CombatEvents = CreateDefaultSubobject<UReEchoCombatEventsComponent>(TEXT("CombatEvents"));
	CombatAudioAdapter = CreateDefaultSubobject<UReEchoCombatAudioAdapterComponent>(TEXT("CombatAudioAdapter"));
	CombatVfx = CreateDefaultSubobject<UReEchoCombatVfxComponent>(TEXT("CombatVfx"));
	CombatVfx->ConfigureAttachmentRoots(AttackVfxRoot, HurtVfxRoot);
	CombatAudioAdapter->ConfigureRouting(EReEchoCombatAudioSource::Echo, FReEchoAudioEvents::EchoAttack, NAME_None);
}

bool AReEchoEchoActor::InitializeEcho(const FReEchoRecording& Recording,
                                      const float Efficiency,
                                      TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot)
{
	if (!Snapshot.IsValid() || Recording.BuildSnapshot.WeaponDomainRevision != Snapshot->WeaponDomainRevision)
	{
		UE_LOG(LogReEcho,
		       Error,
		       TEXT("Cannot initialize echo: weapon domain revision mismatch saved=%s current=%s"),
		       *Recording.BuildSnapshot.WeaponDomainRevision,
		       Snapshot.IsValid() ? *Snapshot->WeaponDomainRevision : TEXT("<none>"));
		return false;
	}
	ConfigureEchoAppearance(Recording.BuildSnapshot.CharacterId);
	DamageEfficiency = Efficiency;
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

	Weapon = GetWorld()->SpawnActor<AReEchoWeaponActor>();
	if (Weapon)
	{
		Weapon->SetOwner(this);
		Weapon->AttachToActor(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		Weapon->SetActorRelativeLocation(FVector::ZeroVector);
		Weapon->InitializeWeapon(&Recording.BuildSnapshot, Snapshot);
		if (!Weapon->SelectWeaponById(Recording.BuildSnapshot.WeaponId))
		{
			UE_LOG(LogReEcho,
			       Error,
			       TEXT("Cannot initialize echo with WeaponId '%s'"),
			       *Recording.BuildSnapshot.WeaponId.ToString());
			Weapon->Destroy();
			Weapon = nullptr;
			return false;
		}
		FReEchoStatBlock EchoStats = Weapon->GetBuildSnapshot().Stats;
		EchoStats.PhysicalAttack = FMath::Max(1.0f, EchoStats.PhysicalAttack * DamageEfficiency);
		EchoStats.ElementalAttack = FMath::Max(1.0f, EchoStats.ElementalAttack * DamageEfficiency);
		Combatant->InitializeFromStats(EchoStats, true);
	}
	if (Weapon && !bAudioLifecycleStarted)
	{
		CombatAudioAdapter->PostConfiguredEvent(FReEchoAudioEvents::EchoSpawn, GetActorLocation());
		bAudioLifecycleStarted = true;
	}
	return Weapon != nullptr;
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
	if (bAudioLifecycleStarted && EndPlayReason == EEndPlayReason::Destroyed)
	{
		CombatAudioAdapter->PostConfiguredEvent(FReEchoAudioEvents::EchoEnd, GetActorLocation());
		bAudioLifecycleStarted = false;
	}
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

void AReEchoEchoActor::ConfigureCardRules(const FReEchoCardRuleSnapshot& Rules, const FReEchoStatBlock& PlayerStats)
{
	bCanAttack = Rules.bEchoesCanAttack;
	if (Combatant && Rules.EchoHealthMultiplier > 1.0f)
	{
		FReEchoStatBlock Stats = Combatant->Stats;
		Stats.HpMax = PlayerStats.HpMax * Rules.EchoHealthMultiplier;
		Stats.HpPoint = Stats.HpMax;
		Combatant->InitializeFromStats(Stats, true);
	}
}

bool AReEchoEchoActor::IsCombatTargetAlive() const
{
	return Combatant && Combatant->IsAlive();
}

bool AReEchoEchoActor::IntersectsCombatPath(const FVector& PathStart,
                                            const FVector& PathEnd,
                                            const float CarrierRadius) const
{
	return FMath::PointDistToSegment(GetActorLocation(), PathStart, PathEnd) <= FMath::Max(0.0f, CarrierRadius) + 35.0f;
}

void AReEchoEchoActor::ModifyOutgoingHit(FReEchoHitIntent& Intent) const
{
	UReEchoRunSubsystem* Run = GetGameInstance() ? GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>() : nullptr;
	if (!Run)
	{
		return;
	}
	const AReEchoPlayerPawn* Player = Cast<AReEchoPlayerPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
	const float DistanceCm = Player ? FVector::Dist2D(Player->GetActorLocation(), GetActorLocation()) : 0.0f;
	const UReEchoCombatantComponent* TargetCombatant =
	    Intent.Target ? Intent.Target->FindComponentByClass<UReEchoCombatantComponent>() : nullptr;
	Run->ModifyCardOutgoingHit(Intent,
	                           Combatant ? Combatant->Stats : Run->CurrentBuild.Stats,
	                           DistanceCm,
	                           TargetCombatant && TargetCombatant->GetElementState().Attached != EReEchoElement::None);
}

void AReEchoEchoActor::NotifyReactionResolved(const FName ReactionId) const
{
	if (UReEchoRunSubsystem* Run = GetGameInstance() ? GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>() : nullptr)
	{
		Run->NotifyCardReaction(ReactionId, false);
	}
}

void AReEchoEchoActor::NotifyKillResolved() const
{
	if (UReEchoRunSubsystem* Run = GetGameInstance() ? GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>() : nullptr)
	{
		Run->NotifyCardKill(true);
	}
}

void AReEchoEchoActor::NotifyDefeated(const EReEchoDamageSource DamageSource) const
{
	if (DamageSource == EReEchoDamageSource::Enemy)
	{
		if (UReEchoRunSubsystem* Run =
		        GetGameInstance() ? GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>() : nullptr)
		{
			Run->NotifyCardEchoDefeated();
			if (AReEchoPlayerPawn* Player = Cast<AReEchoPlayerPawn>(UGameplayStatics::GetPlayerPawn(this, 0)))
			{
				Player->Combatant->InitializeFromStats(Run->CurrentBuild.Stats, false);
			}
		}
	}
}

FString AReEchoEchoActor::GetPinnedWeaponDomainRevision() const
{
	return Weapon ? Weapon->GetPinnedWeaponDomainRevision() : FString();
}

FName AReEchoEchoActor::GetEquippedWeaponId() const
{
	return Weapon ? Weapon->GetEquippedWeaponId() : NAME_None;
}

FVector AReEchoEchoActor::EvaluateRecordedPosition(const float EncounterTime) const
{
	return Playback ? Playback->EvaluateRecordedPosition(EncounterTime) : GetActorLocation();
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

	if (!Weapon || !bCanAttack || !IsCombatTargetAlive())
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
