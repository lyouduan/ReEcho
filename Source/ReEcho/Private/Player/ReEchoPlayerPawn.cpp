#include "Player/ReEchoPlayerPawn.h"

#include "ReEcho.h"
#include "AbilitySystem/ReEchoPlayerAbilities.h"
#include "AbilitySystem/ReEchoCombatAttributeSet.h"
#include "AbilitySystem/ReEchoGameplayTags.h"
#include "AbilitySystemComponent.h"

#include "Camera/PlayerCameraManager.h"
#include "Combat/ReEchoAttackControllerComponent.h"
#include "Combat/ReEchoCombatContracts.h"
#include "Combat/ReEchoCombatAudioAdapterComponent.h"
#include "Combat/ReEchoCombatantComponent.h"
#include "Combat/ReEchoCombatTarget.h"
#include "Core/ReEchoBalanceSettings.h"
#include "Data/ReEchoCsvDataRegistry.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/PlayerController.h"
#include "GameplayAbilitySpec.h"
#include "Graybox/ReEchoEnemyActor.h"
#include "Graybox/ReEchoEchoActor.h"
#include "Graybox/ReEchoCollisionDebug.h"
#include "Graybox/ReEchoProjectileActor.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "PaperFlipbook.h"
#include "Presentation/Animation2D/ReEcho2DAnimationComponent.h"
#include "Presentation/Animation2D/ReEcho2DAnimationTags.h"
#include "Presentation/Animation2D/ReEcho2DPresentationCatalog.h"
#include "Presentation/Animation2D/ReEcho2DPresentationController.h"
#include "Presentation/Animation2D/ReEcho2DFrameCollisionDriver.h"
#include "Presentation/Animation2D/ReEcho2DCharacterPresentationProfile.h"
#include "Presentation/Scene/ReEcho2DSceneLightingComponent.h"
#include "Presentation/VFX/ReEchoCombatVfxComponent.h"
#include "Recording/ReEchoRecorderComponent.h"
#include "Run/ReEchoRunSubsystem.h"
#include "Run/CharacterAbilities/ReEchoCharacterAbilityRuntime.h"
#include "ReEchoGameMode.h"
#include "Weapons/ReEchoWeaponActor.h"
#include "UObject/ConstructorHelpers.h"

AReEchoPlayerPawn::AReEchoPlayerPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	Collision = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->SetVisibility(false);
	Collision->SetCollisionProfileName(TEXT("Pawn"));
	PresentationRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PresentationRoot"));
	PresentationRoot->SetupAttachment(RootComponent);
	FootRoot = CreateDefaultSubobject<USceneComponent>(TEXT("FootRoot"));
	FootRoot->SetupAttachment(PresentationRoot);
	PresentationMotionRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PresentationMotionRoot"));
	PresentationMotionRoot->SetupAttachment(FootRoot);
	FlipbookRoot = CreateDefaultSubobject<USceneComponent>(TEXT("FlipbookRoot"));
	FlipbookRoot->SetupAttachment(PresentationMotionRoot);
	FlipbookRoot->SetRelativeRotation(
	    UReEcho2DAnimationComponent::CalculateCameraFacingRotation(FRotator(-45.0f, 0.0f, 0.0f)));
	GroundRoot = CreateDefaultSubobject<USceneComponent>(TEXT("GroundRoot"));
	GroundRoot->SetupAttachment(FootRoot);
	EffectsRoot = CreateDefaultSubobject<USceneComponent>(TEXT("EffectsRoot"));
	EffectsRoot->SetupAttachment(PresentationMotionRoot);
	AttackVfxRoot = CreateDefaultSubobject<USceneComponent>(TEXT("AttackVfxRoot"));
	AttackVfxRoot->SetupAttachment(EffectsRoot);
	AttackVfxRoot->bEditableWhenInherited = true;
	HurtVfxRoot = CreateDefaultSubobject<USceneComponent>(TEXT("HurtVfxRoot"));
	HurtVfxRoot->SetupAttachment(EffectsRoot);
	HurtVfxRoot->bEditableWhenInherited = true;
	GroundShadow = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GroundShadow"));
	GroundShadow->SetupAttachment(GroundRoot);
	GroundShadow->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GroundShadow->SetCastShadow(false);
	GroundShadow->SetTranslucentSortPriority(-10);
	GroundShadow->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")));
	GroundShadow->SetRelativeLocation(FVector::ZeroVector);
	GroundShadow->SetRelativeScale3D(FVector(0.512f, 0.5376f, 1.0f));
	GroundShadow->bEditableWhenInherited = true;
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GroundShadowMaterialFinder(
	    TEXT("/Game/ReEcho/Materials/M_GroundShadow_Procedural.M_GroundShadow_Procedural"));
	GroundShadow->SetMaterial(0, GroundShadowMaterialFinder.Object);
	SequenceAnimation = CreateDefaultSubobject<UReEcho2DAnimationComponent>(TEXT("FlipbookRenderer"));
	SequenceAnimation->SetupAttachment(FlipbookRoot);
	PresentationController = CreateDefaultSubobject<UReEcho2DPresentationController>(TEXT("PresentationController"));
	FrameCollisionDriver = CreateDefaultSubobject<UReEcho2DFrameCollisionDriver>(TEXT("FrameCollisionDriver"));
	PresentationController->BindCollisionDriver(FrameCollisionDriver);
	SceneLighting = CreateDefaultSubobject<UReEcho2DSceneLightingComponent>(TEXT("SceneLighting"));
	SceneLighting->Configure(SequenceAnimation, GroundShadow);
	static ConstructorHelpers::FObjectFinder<UTexture2D> HeartTextureFinder(
	    TEXT("/Game/ReEcho/Textures/Characters/NewCast/Player_Heart.Player_Heart"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> SpadeTextureFinder(
	    TEXT("/Game/ReEcho/Textures/Characters/NewCast/Player_Cat.Player_Cat"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> CloverTextureFinder(
	    TEXT("/Game/ReEcho/Textures/Characters/NewCast/Player_Clover.Player_Clover"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> DiamondTextureFinder(
	    TEXT("/Game/ReEcho/Textures/Characters/NewCast/Player_Diamond.Player_Diamond"));
	CharacterTextures.Add(TEXT("J_HEART"), HeartTextureFinder.Object);
	CharacterTextures.Add(TEXT("J_SPADE"), SpadeTextureFinder.Object);
	CharacterTextures.Add(TEXT("J_CLOVER"), CloverTextureFinder.Object);
	CharacterTextures.Add(TEXT("J_DIAMOND"), DiamondTextureFinder.Object);
	static ConstructorHelpers::FObjectFinder<UReEcho2DPresentationCatalog> CatalogFinder(
	    TEXT("/Game/ReEcho/DataAsset/Character/Catalogs/DA_CharacterPresentationCatalog."
	         "DA_CharacterPresentationCatalog"));
	PresentationCatalog = CatalogFinder.Object;
	ConfigureCharacter(TEXT("J_SPADE"));
	Movement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("Movement"));
	Movement->MaxSpeed = 420.f;
	AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
	CombatAttributes = CreateDefaultSubobject<UReEchoCombatAttributeSet>(TEXT("CombatAttributes"));
	AbilitySystem->SetIsReplicated(true);
	AbilitySystem->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	Combatant = CreateDefaultSubobject<UReEchoCombatantComponent>(TEXT("Combatant"));
	AttackController = CreateDefaultSubobject<UReEchoAttackControllerComponent>(TEXT("AttackController"));
	Targeting = CreateDefaultSubobject<UReEchoTargetingComponent>(TEXT("Targeting"));
	CombatEvents = CreateDefaultSubobject<UReEchoCombatEventsComponent>(TEXT("CombatEvents"));
	CombatAudioAdapter = CreateDefaultSubobject<UReEchoCombatAudioAdapterComponent>(TEXT("CombatAudioAdapter"));
	CombatVfx = CreateDefaultSubobject<UReEchoCombatVfxComponent>(TEXT("CombatVfx"));
	CombatVfx->ConfigureAttachmentRoots(AttackVfxRoot, HurtVfxRoot);
	Recorder = CreateDefaultSubobject<UReEchoRecorderComponent>(TEXT("Recorder"));
	AutoPossessPlayer = EAutoReceiveInput::Player0;
}

void AReEchoPlayerPawn::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	SetActorScale3D(FVector(FMath::Max(CharacterScale, 0.01f)));
	RefreshFootRoot();
}

void AReEchoPlayerPawn::RefreshFootRoot()
{
	if (FootRoot && Collision)
	{
		FootRoot->SetRelativeLocation(FVector(0.0f, 0.0f, -Collision->GetUnscaledBoxExtent().Z));
		FootRoot->SetRelativeRotation(FRotator::ZeroRotator);
	}
}

void AReEchoPlayerPawn::ConfigureArenaBounds(const FVector2D& Center, const FVector2D& HalfExtents)
{
	ArenaCenter = Center;
	ArenaHalfExtents.X = FMath::Max(100.0f, HalfExtents.X);
	ArenaHalfExtents.Y = FMath::Max(100.0f, HalfExtents.Y);
}

bool AReEchoPlayerPawn::ConfigureCharacter(const FName CharacterId)
{
	const TObjectPtr<UTexture2D>* TextureEntry = CharacterTextures.Find(CharacterId);
	if (!TextureEntry)
	{
		UE_LOG(LogTemp,
		       Error,
		       TEXT("[PlayerSource] ConfigureCharacter rejected unknown character. Actor=%s Character=%s"),
		       *GetNameSafe(this),
		       *CharacterId.ToString());
		return false;
	}

	CurrentCharacterId = CharacterId;
	PortraitTexture = TextureEntry->Get();
	if (!PortraitTexture)
	{
		UE_LOG(LogTemp,
		       Warning,
		       TEXT("[PlayerSource] Character portrait is unavailable; gameplay presentation will continue. Actor=%s "
		            "Character=%s"),
		       *GetNameSafe(this),
		       *CharacterId.ToString());
	}
	RefreshPresentationProfile();
	BaseVisualLocation = FlipbookRoot->GetRelativeLocation();
	BaseVisualScale = FlipbookRoot->GetRelativeScale3D();
	AuthoredMotionLocation = PresentationMotionRoot->GetRelativeLocation();
	BaseEffectsLocation = EffectsRoot->GetRelativeLocation();
	BaseEffectsScale = EffectsRoot->GetRelativeScale3D();
	UE_LOG(LogTemp,
	       Display,
	       TEXT("[PlayerSource] Configured Actor=%s Character=%s Flipbook=%s"),
	       *GetNameSafe(this),
	       *CharacterId.ToString(),
	       *GetNameSafe(SequenceAnimation ? SequenceAnimation->GetFlipbook() : nullptr));
	return true;
}

void AReEchoPlayerPawn::ConfigureArenaBounds(const float HalfExtentX, const float HalfExtentY)
{
	ConfigureArenaBounds(FVector2D::ZeroVector, FVector2D(HalfExtentX, HalfExtentY));
}

void AReEchoPlayerPawn::RestoreEquippedWeapon(const FName WeaponId)
{
	if (Weapon)
	{
		UGameInstance* GameInstance = GetGameInstance();
		if (UReEchoRunSubsystem* RunSubsystem =
		        GameInstance ? GameInstance->GetSubsystem<UReEchoRunSubsystem>() : nullptr;
		    RunSubsystem && !RunSubsystem->CurrentBuild.WeaponDomainRevision.IsEmpty())
		{
			InitializeWeaponFromBuild(RunSubsystem->CurrentBuild, RunSubsystem->GetRunDataSnapshot());
		}
	}
	if (Weapon && !Weapon->SelectWeaponById(WeaponId))
	{
		UE_LOG(LogReEcho, Error, TEXT("Cannot restore equipped WeaponId '%s'"), *WeaponId.ToString());
	}
	RefreshWeaponPresentationSet();
}

bool AReEchoPlayerPawn::InitializeWeaponFromBuild(const FReEchoBuildSnapshot& Build,
                                                  TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot)
{
	if (!Snapshot.IsValid() || !GetWorld())
	{
		return false;
	}
	if (!Weapon)
	{
		Weapon = GetWorld()->SpawnActor<AReEchoWeaponActor>();
		if (!Weapon)
		{
			return false;
		}
		Weapon->SetOwner(this);
		Weapon->AttachToActor(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		Weapon->SetActorRelativeLocation(FVector::ZeroVector);
	}
	Weapon->InitializeWeapon(&Build, Snapshot);
	const bool bInitialized = Weapon->GetEquippedWeaponId() == Build.WeaponId;
	RefreshWeaponPresentationSet();
	return bInitialized;
}

FString AReEchoPlayerPawn::GetPinnedWeaponDomainRevision() const
{
	return Weapon ? Weapon->GetPinnedWeaponDomainRevision() : FString();
}

void AReEchoPlayerPawn::BeginPlay()
{
	Super::BeginPlay();
	RefreshFootRoot();
	if (GroundRoot && FootRoot && GroundRoot->GetAttachParent() != FootRoot)
	{
		GroundRoot->AttachToComponent(FootRoot, FAttachmentTransformRules::KeepRelativeTransform);
	}
	AuthoredGroundRootLocation = GroundRoot ? GroundRoot->GetRelativeLocation() : FVector::ZeroVector;
	AuthoredGroundShadowScale = GroundShadow ? GroundShadow->GetRelativeScale3D() : FVector::OneVector;
	TInlineComponentArray<UReEcho2DAnimationComponent*> AnimationComponents(this);
	for (UReEcho2DAnimationComponent* AnimationComponent : AnimationComponents)
	{
		if (AnimationComponent && AnimationComponent != SequenceAnimation)
		{
			UE_LOG(LogReEcho,
			       Warning,
			       TEXT("Disabling legacy player animation component '%s' on '%s'."),
			       *GetNameSafe(AnimationComponent),
			       *GetNameSafe(this));
			AnimationComponent->Stop();
			AnimationComponent->SetVisibility(false, true);
			AnimationComponent->SetHiddenInGame(true, true);
			AnimationComponent->Deactivate();
		}
	}
	BaseVisualLocation = FlipbookRoot->GetRelativeLocation();
	BaseVisualScale = FlipbookRoot->GetRelativeScale3D();
	AuthoredMotionLocation = PresentationMotionRoot->GetRelativeLocation();
	BaseEffectsLocation = EffectsRoot->GetRelativeLocation();
	BaseEffectsScale = EffectsRoot->GetRelativeScale3D();

	ConfigureMouseInput();

	Weapon = GetWorld()->SpawnActor<AReEchoWeaponActor>();
	if (Weapon)
	{
		Weapon->SetOwner(this);
		Weapon->AttachToActor(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		Weapon->SetActorRelativeLocation(FVector::ZeroVector);
		UGameInstance* GameInstance = GetGameInstance();
		if (UReEchoRunSubsystem* RunSubsystem =
		        GameInstance ? GameInstance->GetSubsystem<UReEchoRunSubsystem>() : nullptr;
		    RunSubsystem && !RunSubsystem->CurrentBuild.WeaponDomainRevision.IsEmpty())
		{
			Weapon->InitializeWeapon(&RunSubsystem->CurrentBuild, RunSubsystem->GetRunDataSnapshot());
		}
		else
		{
			Weapon->InitializeWeapon();
		}
		RefreshWeaponPresentationSet();
	}

	AbilitySystem->InitAbilityActorInfo(this, this);
	Combatant->BindToAbilitySystem(AbilitySystem);
	Combatant->OnHealthChanged.AddUniqueDynamic(this, &AReEchoPlayerPawn::HandleCharacterAbilityHealthChanged);
	AbilitySystem->GetGameplayAttributeValueChangeDelegate(UReEchoCombatAttributeSet::GetMovementSpeedAttribute())
	    .AddUObject(this, &AReEchoPlayerPawn::HandleMovementSpeedAttributeChanged);
	GrantStartupAbilities();
}

void AReEchoPlayerPawn::SetupPlayerInputComponent(UInputComponent* Input)
{
	Super::SetupPlayerInputComponent(Input);
	Input->BindAxis(TEXT("MoveForward"), this, &AReEchoPlayerPawn::MoveForward);
	Input->BindAxis(TEXT("MoveRight"), this, &AReEchoPlayerPawn::MoveRight);
	Input->BindAction(TEXT("BasicAttack"), IE_Pressed, this, &AReEchoPlayerPawn::ManualBasicAttack);
	Input->BindAction(TEXT("BasicAttack"), IE_Released, this, &AReEchoPlayerPawn::ManualStopBasicAttack);
	Input->BindAction(TEXT("ActiveSkill"), IE_Pressed, this, &AReEchoPlayerPawn::ActivateSkill);
	FInputActionBinding& PauseBinding =
	    Input->BindAction(TEXT("PauseMenu"), IE_Pressed, this, &AReEchoPlayerPawn::TogglePauseMenu);
	PauseBinding.bExecuteWhenPaused = true;
	FInputActionBinding& InventoryBinding =
	    Input->BindAction(TEXT("ToggleInventory"), IE_Pressed, this, &AReEchoPlayerPawn::ToggleInventoryMenu);
	InventoryBinding.bExecuteWhenPaused = true;
	FInputActionBinding& ShopBinding =
	    Input->BindAction(TEXT("ToggleShop"), IE_Pressed, this, &AReEchoPlayerPawn::ToggleShopMenu);
	ShopBinding.bExecuteWhenPaused = true;
	FInputActionBinding& StatsBinding =
	    Input->BindAction(TEXT("ToggleStats"), IE_Pressed, this, &AReEchoPlayerPawn::ToggleStatsMenu);
	StatsBinding.bExecuteWhenPaused = true;
}

void AReEchoPlayerPawn::MoveForward(float Value)
{
	AddMovementInput(FVector::ForwardVector, Value);
}

void AReEchoPlayerPawn::MoveRight(float Value)
{
	AddMovementInput(FVector::RightVector, Value);
}

void AReEchoPlayerPawn::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ConstrainToArenaBounds();
	ConfigureMouseInput();

	AttackController->UpdateAutomaticAttack();
	const FReEchoAttackSnapshot AttackSnapshot = AttackController->GetSnapshot();
	if (AttackSnapshot.Mode != EReEchoAttackMode::Automatic || !AttackSnapshot.CurrentTarget)
	{
		UpdateMouseAim();
	}

	UpdateSpriteAnimation(DeltaSeconds);
	ReEchoCollisionDebug::DrawBox(this, Collision, FColor::Cyan);
}

void AReEchoPlayerPawn::ConfigureMouseInput()
{
	if (bMouseInputConfigured)
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		return;
	}

	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(InputMode);
	PlayerController->SetShowMouseCursor(true);
	PlayerController->bEnableClickEvents = true;
	PlayerController->bEnableMouseOverEvents = true;
	bMouseInputConfigured = true;
}

void AReEchoPlayerPawn::UpdateMouseAim()
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		return;
	}

	FVector MouseWorldOrigin;
	FVector MouseWorldDirection;
	if (!PlayerController->DeprojectMousePositionToWorld(MouseWorldOrigin, MouseWorldDirection))
	{
		return;
	}

	const float VerticalDirection = MouseWorldDirection.Z;
	if (FMath::IsNearlyZero(VerticalDirection))
	{
		return;
	}

	const float PlaneDistance = (GetActorLocation().Z - MouseWorldOrigin.Z) / VerticalDirection;
	if (PlaneDistance <= 0.0f)
	{
		return;
	}

	const FVector MouseWorldPosition = MouseWorldOrigin + MouseWorldDirection * PlaneDistance;
	FVector AimDirection = MouseWorldPosition - GetActorLocation();
	AimDirection.Z = 0.0f;
	if (!AimDirection.IsNearlyZero())
	{
		const APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0);
		const FVector CameraRight = CameraManager
		                                ? FRotationMatrix(CameraManager->GetCameraRotation()).GetUnitAxis(EAxis::Y)
		                                : FVector::RightVector;
		const float HorizontalAim = FVector::DotProduct(AimDirection, CameraRight);
		if (FMath::Abs(HorizontalAim) > 5.0f)
		{
			VisualFacingSign = HorizontalAim >= 0.0f ? 1.0f : -1.0f;
		}
		AttackAimDirection = AimDirection.GetSafeNormal2D();
	}
}

void AReEchoPlayerPawn::ConstrainToArenaBounds()
{
	if (ArenaHalfExtents.X <= 0.0f || ArenaHalfExtents.Y <= 0.0f)
	{
		return;
	}
	FVector Location = GetActorLocation();
	Location.X = FMath::Clamp(Location.X, ArenaCenter.X - ArenaHalfExtents.X, ArenaCenter.X + ArenaHalfExtents.X);
	Location.Y = FMath::Clamp(Location.Y, ArenaCenter.Y - ArenaHalfExtents.Y, ArenaCenter.Y + ArenaHalfExtents.Y);
	SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
}

FString AReEchoPlayerPawn::GetEquippedWeaponLabel() const
{
	return Weapon ? Weapon->GetEquippedWeaponLabel() : TEXT("None");
}

float AReEchoPlayerPawn::GetCurrentAttackInterval() const
{
	return Weapon ? Weapon->GetAttackInterval(Combatant) : 0.55f;
}

bool AReEchoPlayerPawn::IsAutoAttackMode() const
{
	return AttackController && AttackController->GetAttackMode() == EReEchoAttackMode::Automatic;
}

bool AReEchoPlayerPawn::IsAutoAttackInputHeld() const
{
	return AttackController && AttackController->IsAutomaticHeld();
}

bool AReEchoPlayerPawn::IsManualAttackInputHeld() const
{
	return AttackController && AttackController->IsManualHeld();
}

bool AReEchoPlayerPawn::IsWeaponActionLocked() const
{
	return Weapon && Weapon->GetActionLockRemaining() > 0.0f;
}

float AReEchoPlayerPawn::GetWeaponActionLockRemaining() const
{
	return Weapon ? Weapon->GetActionLockRemaining() : 0.0f;
}

UAbilitySystemComponent* AReEchoPlayerPawn::GetAbilitySystemComponent() const
{
	return AbilitySystem;
}

bool AReEchoPlayerPawn::IsCombatTargetAlive() const
{
	return Combatant && Combatant->IsAlive();
}

FVector AReEchoPlayerPawn::GetCombatTargetLocation() const
{
	return GetActorLocation();
}

int32 AReEchoPlayerPawn::GetCombatTargetTieBreakIndex() const
{
	return 0;
}

UReEchoCombatantComponent* AReEchoPlayerPawn::GetCombatTargetCombatant() const
{
	return Combatant;
}

bool AReEchoPlayerPawn::IntersectsCombatPath(const FVector& PathStart,
                                             const FVector& PathEnd,
                                             const float CarrierRadius) const
{
	if (!Collision || !Collision->IsCollisionEnabled())
	{
		return false;
	}
	const FBox ExpandedBounds = Collision->Bounds.GetBox().ExpandBy(FMath::Max(0.0f, CarrierRadius));
	return ExpandedBounds.IsInsideOrOn(PathStart) || ExpandedBounds.IsInsideOrOn(PathEnd) ||
	       FMath::LineBoxIntersection(ExpandedBounds, PathStart, PathEnd, PathEnd - PathStart);
}

float AReEchoPlayerPawn::ModifyIncomingRawDamage(const FReEchoHitIntent& Intent) const
{
	if (const AActor* SourceActor = Intent.Attack.Source.Get())
	{
		UE_LOG(LogReEcho,
		       Warning,
		       TEXT("[PlayerDamage] raw=%.2f from=%s(%s) source=%d element=%d"),
		       Intent.RawDamage,
		       *SourceActor->GetName(),
		       *SourceActor->GetClass()->GetName(),
		       static_cast<int32>(Intent.DamageSource),
		       static_cast<int32>(Intent.Element));
	}
	else
	{
		UE_LOG(LogReEcho,
		       Warning,
		       TEXT("[PlayerDamage] raw=%.2f from=None source=%d element=%d"),
		       Intent.RawDamage,
		       static_cast<int32>(Intent.DamageSource),
		       static_cast<int32>(Intent.Element));
	}
	if (UReEchoRunSubsystem* Run = GetGameInstance() ? GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>() : nullptr)
	{
		return Run->ModifyCardIncomingHit(Intent.RawDamage);
	}
	return Intent.RawDamage;
}

void AReEchoPlayerPawn::ModifyOutgoingHit(FReEchoHitIntent& Intent) const
{
	UReEchoRunSubsystem* Run = GetGameInstance() ? GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>() : nullptr;
	if (Run)
	{
		float EchoDistanceCm = 0.0f;
		for (TActorIterator<AReEchoEchoActor> It(GetWorld()); It; ++It)
		{
			if (It->IsCombatTargetAlive())
			{
				EchoDistanceCm =
				    FMath::Max(EchoDistanceCm, FVector::Dist2D(GetActorLocation(), It->GetActorLocation()));
			}
		}
		const UReEchoCombatantComponent* TargetCombatant =
		    Intent.Target ? Intent.Target->FindComponentByClass<UReEchoCombatantComponent>() : nullptr;
		Run->ModifyCardOutgoingHit(Intent,
		                           Combatant ? Combatant->Stats : Run->CurrentBuild.Stats,
		                           EchoDistanceCm,
		                           TargetCombatant &&
		                               TargetCombatant->GetElementState().Attached != EReEchoElement::None);
	}

#if !UE_BUILD_SHIPPING
	if (DebugOutgoingElementOverride != EReEchoElement::None)
	{
		Intent.Element = DebugOutgoingElementOverride;
	}
#endif
}

void AReEchoPlayerPawn::SetDebugOutgoingElementOverride(const EReEchoElement Element)
{
#if !UE_BUILD_SHIPPING
	DebugOutgoingElementOverride = Element;
#endif
}

EReEchoElement AReEchoPlayerPawn::GetDebugOutgoingElementOverride() const
{
#if UE_BUILD_SHIPPING
	return EReEchoElement::None;
#else
	return DebugOutgoingElementOverride;
#endif
}

void AReEchoPlayerPawn::NotifyReactionResolved(const FName ReactionId) const
{
	if (UReEchoRunSubsystem* Run = GetGameInstance() ? GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>() : nullptr)
	{
		const float Healing = Run->NotifyCardReaction(ReactionId, true);
		if (Combatant && Healing > 0.0f)
		{
			Combatant->ApplyHealing(Healing);
		}
		if (Combatant)
		{
			Combatant->InitializeFromStats(Run->CurrentBuild.Stats, false);
		}
	}
}

void AReEchoPlayerPawn::NotifyKillResolved() const
{
	if (UReEchoRunSubsystem* Run = GetGameInstance() ? GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>() : nullptr)
	{
		Run->NotifyCardKill(false);
		if (Combatant)
		{
			Combatant->InitializeFromStats(Run->CurrentBuild.Stats, false);
		}
	}
}

void AReEchoPlayerPawn::GrantStartupAbilities()
{
	if (!HasAuthority() || !AbilitySystem)
	{
		return;
	}

	auto GrantAbility = [this](const TSubclassOf<UGameplayAbility> AbilityClass, const FGameplayTag InputTag)
	{
		FGameplayAbilitySpec Spec(AbilityClass, 1);
		Spec.GetDynamicSpecSourceTags().AddTag(InputTag);
		AbilitySystem->GiveAbility(Spec);
	};
	GrantAbility(UReEchoBasicAttackAbility::StaticClass(), ReEchoGameplayTags::Input_Attack_Basic);
	GrantAbility(UReEchoActiveAttackAbility::StaticClass(), ReEchoGameplayTags::Input_Attack_Active);
}

void AReEchoPlayerPawn::AbilityInputPressed(const FGameplayTag& InputTag)
{
	if (!AbilitySystem || !InputTag.IsValid())
	{
		return;
	}
	FScopedAbilityListLock AbilityListLock(*AbilitySystem);
	for (FGameplayAbilitySpec& Spec : AbilitySystem->GetActivatableAbilities())
	{
		if (!Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			continue;
		}
		Spec.InputPressed = true;
		if (Spec.IsActive())
		{
			AbilitySystem->AbilitySpecInputPressed(Spec);
		}
		else
		{
			AbilitySystem->TryActivateAbility(Spec.Handle);
		}
	}
}

void AReEchoPlayerPawn::AbilityInputReleased(const FGameplayTag& InputTag)
{
	if (!AbilitySystem || !InputTag.IsValid())
	{
		return;
	}
	FScopedAbilityListLock AbilityListLock(*AbilitySystem);
	for (FGameplayAbilitySpec& Spec : AbilitySystem->GetActivatableAbilities())
	{
		if (Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			Spec.InputPressed = false;
			if (Spec.IsActive())
			{
				AbilitySystem->AbilitySpecInputReleased(Spec);
			}
		}
	}
}

void AReEchoPlayerPawn::TogglePauseMenu()
{
	if (AReEchoGameMode* GameMode = GetWorld()->GetAuthGameMode<AReEchoGameMode>())
	{
		GameMode->TogglePauseMenu();
	}
}

void AReEchoPlayerPawn::ToggleInventoryMenu()
{
	UE_LOG(LogReEcho, Log, TEXT("[AttrPanel] Pawn: ToggleInventoryMenu key received"));
	if (AReEchoGameMode* GameMode = GetWorld()->GetAuthGameMode<AReEchoGameMode>())
	{
		GameMode->ToggleInventoryMenu();
	}
}

void AReEchoPlayerPawn::ToggleShopMenu()
{
	UE_LOG(LogReEcho, Log, TEXT("[AttrPanel] Pawn: ToggleShopMenu key received"));
	if (AReEchoGameMode* GameMode = GetWorld()->GetAuthGameMode<AReEchoGameMode>())
	{
		GameMode->ToggleShopMenu();
	}
}

void AReEchoPlayerPawn::ToggleStatsMenu()
{
	if (AReEchoGameMode* GameMode = GetWorld()->GetAuthGameMode<AReEchoGameMode>())
	{
		GameMode->ToggleStatsMenu();
	}
}

void AReEchoPlayerPawn::BasicAttack()
{
	AbilityInputPressed(ReEchoGameplayTags::Input_Attack_Basic);
}

void AReEchoPlayerPawn::StopBasicAttack()
{
	AbilityInputReleased(ReEchoGameplayTags::Input_Attack_Basic);
}

void AReEchoPlayerPawn::ManualBasicAttack()
{
	// 物理（手动）输入仅在手动模式下驱动 GAS basic-attack 输入。
	// 自动模式下该共享 GAS spec 归自动循环所有，因此物理按下必须被忽略，
	// 既不能无目标起手，也不能触碰自动循环持有的同一输入源（Planner review 2026-08-12）。
	AttackController->BeginManualAttack();
}

void AReEchoPlayerPawn::ManualStopBasicAttack()
{
	AttackController->EndManualAttack();
}

void AReEchoPlayerPawn::ReleaseAllBasicAttackInputs()
{
	// 菜单开/关必须释放每个 held basic-attack 输入源，使恢复游戏时不会残留
	// 来自自动循环或物理输入的陈旧 held 状态。
	AttackController->ReleaseAttackRequests();
}

void AReEchoPlayerPawn::SetAutoAttackMode(const bool bAuto)
{
	if (IsAutoAttackMode() == bAuto)
	{
		return;
	}
	AttackController->SetAttackMode(bAuto ? EReEchoAttackMode::Automatic : EReEchoAttackMode::Manual);
	if (bAuto)
	{
		// 切换到自动模式：物理（手动）held 输入不再权威，必须释放它，
		// 这样自动循环才能干净地独占共享的 GAS basic-attack 输入。
		if (AttackController->IsManualHeld())
		{
			AttackController->ReleaseAttackRequests();
		}
	}
	else
	{
		// 切换到手动模式：释放模拟的自动 held 输入。
		ReleaseAutoAttackInput();
	}
}

void AReEchoPlayerPawn::PressAutoAttackInput()
{
	if (AttackController->IsAutomaticHeld())
	{
		return;
	}
	AttackController->UpdateAutomaticAttack();
}

void AReEchoPlayerPawn::ReleaseAutoAttackInput()
{
	if (AttackController->IsAutomaticHeld())
	{
		AttackController->ReleaseAttackRequests();
	}
}

void AReEchoPlayerPawn::UpdateAutoAttack(bool& bOutHasTarget)
{
	bOutHasTarget = false;

	const bool bCanAuto = AttackController->GetAttackMode() == EReEchoAttackMode::Automatic && Combatant &&
	                      Combatant->IsAlive() && AbilitySystem &&
	                      AbilitySystem->GetGameplayTagCount(ReEchoGameplayTags::State_Menu) == 0;
	if (!bCanAuto)
	{
		ReleaseAutoAttackInput();
		return;
	}

	const float RangeCm = Weapon ? Weapon->GetCurrentAttackRangeCm() : 0.0f;
	AReEchoEnemyActor* Target = FindNearestEnemyInRange(RangeCm);
	if (!Target)
	{
		ReleaseAutoAttackInput();
		return;
	}

	FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
	ToTarget.Z = 0.0f;
	if (!ToTarget.IsNearlyZero())
	{
		const APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0);
		const FVector CameraRight = CameraManager
		                                ? FRotationMatrix(CameraManager->GetCameraRotation()).GetUnitAxis(EAxis::Y)
		                                : FVector::RightVector;
		const float HorizontalAim = FVector::DotProduct(ToTarget, CameraRight);
		if (FMath::Abs(HorizontalAim) > 1.0f)
		{
			VisualFacingSign = HorizontalAim >= 0.0f ? 1.0f : -1.0f;
		}
		AttackAimDirection = ToTarget.GetSafeNormal2D();
	}
	bOutHasTarget = true;

	// Automatic and manual attack share the same held GAS BasicAttack input. The ability and
	// weapon action lock remain the only attack-rate authorities; automatic mode only selects a
	// target and keeps the input held.
	PressAutoAttackInput();
}

AReEchoEnemyActor* AReEchoPlayerPawn::FindNearestEnemyInRange(const float RangeCm) const
{
	TArray<FReEchoAttackTargetCandidate> Candidates;
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AReEchoEnemyActor> It(World); It; ++It)
		{
			AReEchoEnemyActor* Enemy = *It;
			if (!IsValid(Enemy))
			{
				continue;
			}
			FReEchoAttackTargetCandidate& Candidate = Candidates.AddDefaulted_GetRef();
			Candidate.Location = Enemy->GetActorLocation();
			Candidate.bAlive = Enemy->IsAlive();
			Candidate.StableId = Enemy->GetSpawnIndex();
			Candidate.Source = Enemy;
		}
	}
	const int32 Index = SelectNearestEnemyInRange(GetActorLocation(), RangeCm, Candidates);
	return Index != INDEX_NONE ? Candidates[Index].Source : nullptr;
}

int32 AReEchoPlayerPawn::SelectNearestEnemyInRange(const FVector& Origin,
                                                   const float RangeCm,
                                                   TArrayView<const FReEchoAttackTargetCandidate> Candidates)
{
	int32 BestIndex = INDEX_NONE;
	float BestDistSq = RangeCm * RangeCm;
	for (int32 i = 0; i < Candidates.Num(); ++i)
	{
		const FReEchoAttackTargetCandidate& Candidate = Candidates[i];
		if (!Candidate.bAlive)
		{
			continue;
		}
		const float DistSq = FVector::DistSquared(Origin, Candidate.Location);
		if (DistSq > BestDistSq)
		{
			continue;
		}
		if (BestIndex == INDEX_NONE)
		{
			BestIndex = i;
			BestDistSq = DistSq;
		}
		else if (FMath::IsNearlyEqual(DistSq, BestDistSq, 1e-3f))
		{
			if (Candidate.StableId < Candidates[BestIndex].StableId)
			{
				BestIndex = i;
			}
		}
		else
		{
			BestIndex = i;
			BestDistSq = DistSq;
		}
	}
	return BestIndex;
}

void AReEchoPlayerPawn::ActivateSkill()
{
	AbilityInputPressed(ReEchoGameplayTags::Input_Attack_Active);
	AbilityInputReleased(ReEchoGameplayTags::Input_Attack_Active);
}

bool AReEchoPlayerPawn::ExecuteBasicAttackAbility()
{
	const bool bAttacked = Weapon && Weapon->TryBasicAttack(Combatant);
	if (bAttacked)
	{
		StartAttackVisual(0.18f, 16.0f);
	}
	return bAttacked;
}

EReEchoAttackAttempt AReEchoPlayerPawn::TryCommitBasicAttack()
{
	if (!Weapon || !Combatant)
	{
		return EReEchoAttackAttempt::Rejected;
	}
	if (Weapon->GetAttackCooldownRemaining() > KINDA_SMALL_NUMBER)
	{
		return EReEchoAttackAttempt::Waiting;
	}
	if (!ExecuteBasicAttackAbility())
	{
		return EReEchoAttackAttempt::Rejected;
	}
	return EReEchoAttackAttempt::Committed;
}

float AReEchoPlayerPawn::GetBasicAttackWaitRemaining() const
{
	return Weapon ? Weapon->GetAttackCooldownRemaining() : 0.0f;
}

bool AReEchoPlayerPawn::ExecuteActiveAttack()
{
	return ExecuteActiveAttackAbility();
}

float AReEchoPlayerPawn::GetActiveAttackCooldown() const
{
	return GetCurrentAttackInterval();
}

bool AReEchoPlayerPawn::CanIssueAttackRequest() const
{
	return Combatant && Combatant->IsAlive() && AbilitySystem &&
	       AbilitySystem->GetGameplayTagCount(ReEchoGameplayTags::State_Menu) == 0;
}

float AReEchoPlayerPawn::GetAutomaticAttackRange() const
{
	return Weapon ? Weapon->GetCurrentAttackRangeCm() : 0.0f;
}

void AReEchoPlayerPawn::FaceAutomaticTarget(AActor& Target)
{
	FVector ToTarget = Target.GetActorLocation() - GetActorLocation();
	ToTarget.Z = 0.0f;
	if (!ToTarget.IsNearlyZero())
	{
		const APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0);
		const FVector CameraRight = CameraManager
		                                ? FRotationMatrix(CameraManager->GetCameraRotation()).GetUnitAxis(EAxis::Y)
		                                : FVector::RightVector;
		const float HorizontalAim = FVector::DotProduct(ToTarget, CameraRight);
		if (FMath::Abs(HorizontalAim) > 1.0f)
		{
			VisualFacingSign = HorizontalAim >= 0.0f ? 1.0f : -1.0f;
		}
		AttackAimDirection = ToTarget.GetSafeNormal2D();
	}
}

void AReEchoPlayerPawn::PressBasicAttackInput()
{
	BasicAttack();
}

void AReEchoPlayerPawn::ReleaseBasicAttackInput()
{
	StopBasicAttack();
}

FName AReEchoPlayerPawn::GetAttackWeaponId() const
{
	return Weapon ? Weapon->GetEquippedWeaponId() : NAME_None;
}

FName AReEchoPlayerPawn::GetAttackStepId() const
{
	return Weapon ? Weapon->GetCurrentAttackStepId() : NAME_None;
}

float AReEchoPlayerPawn::GetAttackReadinessRemaining() const
{
	return GetBasicAttackWaitRemaining();
}

float AReEchoPlayerPawn::GetEffectiveAttackSpeed() const
{
	return Combatant ? Combatant->Stats.AttackSpeed : 1.0f;
}

bool AReEchoPlayerPawn::ExecuteActiveAttackAbility()
{
	if (!Weapon || !Weapon->TryActiveAttack(Combatant))
	{
		return false;
	}

	StartAttackVisual(0.28f, 24.0f);
	OnActiveSkill.Broadcast(GetActorLocation(), Weapon->GetEquippedWeaponId());
	return true;
}

void AReEchoPlayerPawn::PlayHitVisual()
{
	HitVisualRemaining = 0.18f;
}

bool AReEchoPlayerPawn::IsWeaponInvulnerable() const
{
	return Weapon && Weapon->IsInvulnerableWindowActive();
}

void AReEchoPlayerPawn::StartAttackVisual(const float Duration, const float Strength)
{
	AttackVisualDuration = Duration;
	AttackVisualRemaining = Duration;
	AttackVisualStrength = Strength;
	if (PresentationController)
	{
		PresentationController->PlayAction(
		    ReEcho2DAnimationTags::Attack_Basic, true, NextPresentationAttackInstanceId++);
	}
}

void AReEchoPlayerPawn::UpdateSpriteAnimation(const float DeltaSeconds)
{
	if (!PresentationRoot)
	{
		return;
	}
	VisualTime += DeltaSeconds;
	AttackVisualRemaining = FMath::Max(0.0f, AttackVisualRemaining - DeltaSeconds);
	HitVisualRemaining = FMath::Max(0.0f, HitVisualRemaining - DeltaSeconds);
	const bool bMoving = GetVelocity().SizeSquared2D() > 25.0f;
	float Lunge = 0.0f;
	float ScaleX = 1.0f;
	float ScaleY = 1.0f;
	if (HitVisualRemaining > 0.0f)
	{
		const float HitRatio = HitVisualRemaining / 0.18f;
		Lunge -= FMath::Sin(VisualTime * 85.0f) * 7.0f * HitRatio;
		ScaleX *= 1.12f;
		ScaleY *= 0.86f;
	}
	if (PresentationController)
	{
		PresentationController->SetMoving(bMoving);
	}
	UpdateSequenceFrame();
	ApplyPresentationMotion(FVector(Lunge, 0.0f, 0.0f), FVector(ScaleX, ScaleY, 1.0f));
	RefreshGroundShadowFromFlipbook();
}

void AReEchoPlayerPawn::RefreshPresentationProfile()
{
	UReEcho2DCharacterPresentationProfile* Profile = nullptr;
	if (PresentationCatalog)
	{
		const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
		const FReEchoCsvCharacterRow* Character =
		    Snapshot.IsValid() ? Snapshot->FindCharacter(CurrentCharacterId) : nullptr;
		Profile = Character ? PresentationCatalog->ResolveProfile(Character->AppearanceId) : nullptr;
	}
	if (PresentationController)
	{
		PresentationController->Configure(
		    SequenceAnimation, Profile, Weapon ? Weapon->GetEquippedWeaponVisualKey() : NAME_None);
	}
	ActivePresentationProfile = Profile;
	if (Weapon)
	{
		Weapon->ConfigureHeldPresentation(ActivePresentationProfile);
	}
	BaseVisualLocation = FlipbookRoot->GetRelativeLocation();
	BaseVisualScale = FlipbookRoot->GetRelativeScale3D();
	AuthoredMotionLocation = PresentationMotionRoot->GetRelativeLocation();
	BaseEffectsLocation = EffectsRoot->GetRelativeLocation();
	BaseEffectsScale = EffectsRoot->GetRelativeScale3D();
}

void AReEchoPlayerPawn::ApplyPresentationMotion(const FVector& Offset, const FVector& Scale)
{
	FlipbookRoot->SetRelativeLocation(BaseVisualLocation);
	FlipbookRoot->SetRelativeScale3D(BaseVisualScale * Scale);
	EffectsRoot->SetRelativeLocation(BaseEffectsLocation);
	EffectsRoot->SetRelativeScale3D(BaseEffectsScale * Scale);
	RefreshFootpointAlignment();
	PresentationMotionRoot->SetRelativeLocation(AuthoredMotionLocation + CalculatedFootAlignmentOffset + Offset);
}

void AReEchoPlayerPawn::RefreshFootpointAlignment()
{
	CalculatedFootAlignmentOffset = FVector::ZeroVector;
	const UPaperFlipbook* Flipbook = SequenceAnimation ? SequenceAnimation->GetFlipbook() : nullptr;
	if (!FlipbookRoot || !SequenceAnimation || !Flipbook ||
	    (ActivePresentationProfile && !ActivePresentationProfile->bAutoAlignFootpoint))
	{
		return;
	}

	CalculatedFootAlignmentOffset = UReEcho2DAnimationComponent::CalculateFootAlignmentOffset(
	    Flipbook->GetRenderBounds(),
	    SequenceAnimation->GetRelativeTransform(),
	    FlipbookRoot->GetRelativeTransform(),
	    AuthoredMotionLocation,
	    ActivePresentationProfile ? ActivePresentationProfile->FootpointOffset : FVector::ZeroVector);
}

void AReEchoPlayerPawn::RefreshGroundShadowFromFlipbook()
{
	const UPaperFlipbook* Flipbook = SequenceAnimation ? SequenceAnimation->GetFlipbook() : nullptr;
	const UStaticMesh* ShadowMesh = GroundShadow ? GroundShadow->GetStaticMesh() : nullptr;
	if (!FootRoot || !GroundRoot || !FlipbookRoot || !SequenceAnimation || !Flipbook || !GroundShadow || !ShadowMesh)
	{
		return;
	}

	const FBoxSphereBounds FlipbookBounds = Flipbook->GetRenderBounds();
	const FVector LocalBottomCenter(
	    FlipbookBounds.Origin.X, FlipbookBounds.Origin.Y, FlipbookBounds.Origin.Z - FlipbookBounds.BoxExtent.Z);
	const FVector BottomWorld = SequenceAnimation->GetComponentTransform().TransformPosition(LocalBottomCenter);
	const FVector BottomInFootRoot = FootRoot->GetComponentTransform().InverseTransformPosition(BottomWorld);
	GroundRoot->SetRelativeLocation(FVector(BottomInFootRoot.X, BottomInFootRoot.Y, AuthoredGroundRootLocation.Z));

	const float FlipbookWidth = UReEcho2DAnimationComponent::CalculateFlipbookPresentationWidth(
	    FlipbookBounds, SequenceAnimation->GetRelativeTransform(), FlipbookRoot->GetRelativeTransform());
	const float ShadowNativeWidth = ShadowMesh->GetBounds().BoxExtent.Y * 2.0f;
	if (FlipbookWidth <= UE_SMALL_NUMBER || ShadowNativeWidth <= UE_SMALL_NUMBER)
	{
		return;
	}

	FVector ShadowScale = AuthoredGroundShadowScale;
	ShadowScale.Y = FlipbookWidth / ShadowNativeWidth;
	GroundShadow->SetRelativeScale3D(ShadowScale);
}

void AReEchoPlayerPawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Combatant)
	{
		Combatant->OnHealthChanged.RemoveDynamic(this, &AReEchoPlayerPawn::HandleCharacterAbilityHealthChanged);
	}
	Super::EndPlay(EndPlayReason);
}

void AReEchoPlayerPawn::RefreshWeaponPresentationSet()
{
	if (PresentationController)
	{
		PresentationController->SetWeaponVisualSetId(Weapon ? Weapon->GetEquippedWeaponVisualKey() : NAME_None);
	}
	if (Weapon)
	{
		Weapon->ConfigureHeldPresentation(ActivePresentationProfile);
	}
}

void AReEchoPlayerPawn::UpdateSequenceFrame()
{
	if (SequenceAnimation && SequenceAnimation->IsAnimationActive())
	{
		SequenceAnimation->SetFacingSign(VisualFacingSign);
	}
}

void AReEchoPlayerPawn::HandleMovementSpeedAttributeChanged(const FOnAttributeChangeData& Data)
{
	if (Movement)
	{
		Movement->MaxSpeed = 420.0f * FMath::Max(0.1f, Data.NewValue);
	}
}

void AReEchoPlayerPawn::HandleCharacterAbilityHealthChanged(const float CurrentHealth, const float MaximumHealth)
{
	if (!Combatant)
	{
		return;
	}
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
	const FVector2D Bonus = Snapshot.IsValid() ? ReEchoCharacterAbilityRuntime::ResolveCurrentMissingHealthAttackBonus(
	                                                 *Snapshot, CurrentCharacterId, CurrentHealth, MaximumHealth)
	                                           : FVector2D::ZeroVector;
	Combatant->SetAdditiveAttackModifier(TEXT("Character.MissingHealthSteps"), Bonus.X, Bonus.Y);
}
