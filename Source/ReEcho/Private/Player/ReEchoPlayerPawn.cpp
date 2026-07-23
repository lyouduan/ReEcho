#include "Player/ReEchoPlayerPawn.h"

#include "AbilitySystem/ReEchoPlayerAbilities.h"
#include "AbilitySystemComponent.h"

#include "Camera/CameraComponent.h"
#include "Combat/ReEchoCombatantComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/PlayerController.h"
#include "GameplayAbilitySpec.h"
#include "Graybox/ReEchoEnemyActor.h"
#include "Graybox/ReEchoAttackEffects.h"
#include "Graybox/ReEchoBillboardDebug.h"
#include "Graybox/ReEchoCollisionDebug.h"
#include "Graybox/ReEchoHealthBarActor.h"
#include "Graybox/ReEchoProjectileActor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Recording/ReEchoRecorderComponent.h"
#include "Run/ReEchoRunSubsystem.h"
#include "ReEchoGameMode.h"
#include "Weapons/ReEchoWeaponActor.h"

AReEchoPlayerPawn::AReEchoPlayerPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->InitSphereRadius(32.f);
	Collision->SetVisibility(true);
	Collision->SetCollisionProfileName(TEXT("Pawn"));
	Shape = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlayerShape"));
	Shape->SetupAttachment(RootComponent);
	Shape->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Shape->SetVisibility(false);
	Shape->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")));
	Shape->SetRelativeScale3D(FVector(0.65f));
	if (UMaterialInterface* Base =
	        LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
	{
		UMaterialInstanceDynamic* Mat = UMaterialInstanceDynamic::Create(Base, this);
		Mat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.1f, 1.f, 0.25f));
		Shape->SetMaterial(0, Mat);
	}
	GroundShadow = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GroundShadow"));
	GroundShadow->SetupAttachment(RootComponent);
	GroundShadow->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GroundShadow->SetCastShadow(false);
	GroundShadow->SetTranslucentSortPriority(-1);
	GroundShadow->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")));
	GroundShadow->SetRelativeLocation(FVector(0.0f, 0.0f, -49.0f));
	GroundShadow->SetRelativeScale3D(FVector(0.55f, 0.34f, 1.0f));
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
	constexpr float CharacterWorldHeight = 260.0f;
	CharacterSprite->SetRelativeLocation(FVector(0.0f, 0.0f, CharacterWorldHeight * 0.5f));
	CharacterSprite->bIsScreenSizeScaled = false;
	if (UTexture2D* CharacterTexture =
	        LoadObject<UTexture2D>(nullptr, TEXT("/Game/ReEcho/Textures/Characters/Player2D.Player2D")))
	{
		CharacterSprite->SetSprite(CharacterTexture);
		const float TextureScale = CharacterWorldHeight / FMath::Max(1, CharacterTexture->GetSizeY());
		CharacterSprite->SetRelativeScale3D(FVector(TextureScale));
	}
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(RootComponent);
	Camera->SetAbsolute(true, true, false);
	Camera->SetWorldLocation(FVector(-700.0f, 0.0f, 900.0f));
	Camera->SetWorldRotation(FRotator(-55.0f, 0.0f, 0.0f));
	Movement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("Movement"));
	Movement->MaxSpeed = 420.f;
	AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
	AbilitySystem->SetIsReplicated(true);
	AbilitySystem->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	Combatant = CreateDefaultSubobject<UReEchoCombatantComponent>(TEXT("Combatant"));
	Recorder = CreateDefaultSubobject<UReEchoRecorderComponent>(TEXT("Recorder"));
	AutoPossessPlayer = EAutoReceiveInput::Player0;
}

void AReEchoPlayerPawn::BeginPlay()
{
	Super::BeginPlay();
	UpdateFixedCamera();

	ConfigureMouseInput();

	HealthBar = GetWorld()->SpawnActor<AReEchoHealthBarActor>();
	if (HealthBar)
	{
		HealthBar->Initialize(Combatant, FLinearColor(0.1f, 1.f, 0.25f), 100.f, 0.9f, CharacterSprite);
	}

	Weapon = GetWorld()->SpawnActor<AReEchoWeaponActor>();
	if (Weapon)
	{
		Weapon->SetOwner(this);
		Weapon->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
		Weapon->SetActorRelativeLocation(FVector(28.0f, 0.0f, 16.0f));
		Weapon->InitializeWeapon();
	}

	AbilitySystem->InitAbilityActorInfo(this, this);
	GrantStartupAbilities();
}

void AReEchoPlayerPawn::SetupPlayerInputComponent(UInputComponent* Input)
{
	Super::SetupPlayerInputComponent(Input);
	Input->BindAxis(TEXT("MoveForward"), this, &AReEchoPlayerPawn::MoveForward);
	Input->BindAxis(TEXT("MoveRight"), this, &AReEchoPlayerPawn::MoveRight);
	Input->BindAction(TEXT("BasicAttack"), IE_Pressed, this, &AReEchoPlayerPawn::BasicAttack);
	Input->BindAction(TEXT("ActiveSkill"), IE_Pressed, this, &AReEchoPlayerPawn::ActivateSkill);
	Input->BindAction(TEXT("WeaponSlot1"), IE_Pressed, this, &AReEchoPlayerPawn::SelectWeaponSlot1);
	Input->BindAction(TEXT("WeaponSlot2"), IE_Pressed, this, &AReEchoPlayerPawn::SelectWeaponSlot2);
	Input->BindAction(TEXT("WeaponSlot3"), IE_Pressed, this, &AReEchoPlayerPawn::SelectWeaponSlot3);
	FInputActionBinding& PauseBinding =
	    Input->BindAction(TEXT("PauseMenu"), IE_Pressed, this, &AReEchoPlayerPawn::TogglePauseMenu);
	PauseBinding.bExecuteWhenPaused = true;
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
	ConfigureMouseInput();
	UpdateMouseAim();
	UpdateFixedCamera();
	ReEchoBillboardDebug::DrawBounds(this, CharacterSprite, FColor::Green);
	ReEchoCollisionDebug::DrawSphere(this, Collision, FColor::Cyan);
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
		SetActorRotation(AimDirection.Rotation());
	}
}

void AReEchoPlayerPawn::UpdateFixedCamera()
{
	Camera->SetWorldLocation(GetActorLocation() + FVector(-700.0f, 0.0f, 900.0f));
	Camera->SetWorldRotation(FRotator(-55.0f, 0.0f, 0.0f));
}

FString AReEchoPlayerPawn::GetEquippedWeaponLabel() const
{
	return Weapon ? Weapon->GetEquippedWeaponLabel() : TEXT("None");
}

UAbilitySystemComponent* AReEchoPlayerPawn::GetAbilitySystemComponent() const
{
	return AbilitySystem;
}

void AReEchoPlayerPawn::GrantStartupAbilities()
{
	if (!HasAuthority() || !AbilitySystem)
	{
		return;
	}

	AbilitySystem->GiveAbility(FGameplayAbilitySpec(UReEchoBasicAttackAbility::StaticClass(), 1));
	AbilitySystem->GiveAbility(FGameplayAbilitySpec(UReEchoActiveAttackAbility::StaticClass(), 1));
	AbilitySystem->GiveAbility(FGameplayAbilitySpec(UReEchoSelectWeaponSlot1Ability::StaticClass(), 1));
	AbilitySystem->GiveAbility(FGameplayAbilitySpec(UReEchoSelectWeaponSlot2Ability::StaticClass(), 1));
	AbilitySystem->GiveAbility(FGameplayAbilitySpec(UReEchoSelectWeaponSlot3Ability::StaticClass(), 1));
}

bool AReEchoPlayerPawn::TryActivatePlayerAbility(const TSubclassOf<UGameplayAbility> AbilityClass)
{
	return AbilitySystem && AbilityClass && AbilitySystem->TryActivateAbilityByClass(AbilityClass);
}

void AReEchoPlayerPawn::TogglePauseMenu()
{
	if (AReEchoGameMode* GameMode = GetWorld()->GetAuthGameMode<AReEchoGameMode>())
	{
		GameMode->TogglePauseMenu();
	}
}

void AReEchoPlayerPawn::BasicAttack()
{
	TryActivatePlayerAbility(UReEchoBasicAttackAbility::StaticClass());
}

void AReEchoPlayerPawn::ActivateSkill()
{
	TryActivatePlayerAbility(UReEchoActiveAttackAbility::StaticClass());
}

void AReEchoPlayerPawn::SelectWeaponSlot1()
{
	TryActivatePlayerAbility(UReEchoSelectWeaponSlot1Ability::StaticClass());
}

void AReEchoPlayerPawn::SelectWeaponSlot2()
{
	TryActivatePlayerAbility(UReEchoSelectWeaponSlot2Ability::StaticClass());
}

void AReEchoPlayerPawn::SelectWeaponSlot3()
{
	TryActivatePlayerAbility(UReEchoSelectWeaponSlot3Ability::StaticClass());
}

bool AReEchoPlayerPawn::ExecuteBasicAttackAbility()
{
	return Weapon && Weapon->TryBasicAttack(Combatant);
}

bool AReEchoPlayerPawn::ExecuteActiveAttackAbility()
{
	if (!Weapon || !Weapon->TryActiveAttack(Combatant))
	{
		return false;
	}

	OnActiveSkill.Broadcast(GetActorLocation(), Weapon->GetEquippedWeaponId());
	return true;
}

bool AReEchoPlayerPawn::ExecuteSelectWeaponSlot1Ability()
{
	return ExecuteSelectWeaponAbility(EReEchoWeaponSlot::PhysicalOrb, TEXT("W_J_02"));
}

bool AReEchoPlayerPawn::ExecuteSelectWeaponSlot2Ability()
{
	return ExecuteSelectWeaponAbility(EReEchoWeaponSlot::Sword, TEXT("W_J_01"));
}

bool AReEchoPlayerPawn::ExecuteSelectWeaponSlot3Ability()
{
	return ExecuteSelectWeaponAbility(EReEchoWeaponSlot::ElementalOrb, TEXT("W_J_03"));
}

bool AReEchoPlayerPawn::ExecuteSelectWeaponAbility(const EReEchoWeaponSlot WeaponSlot, const FName WeaponId)
{
	if (!Weapon)
	{
		return false;
	}

	Weapon->SelectWeapon(WeaponSlot);

	if (UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>())
	{
		RunSubsystem->SetEquippedWeapon(WeaponId);
		Recorder->UpdateBuildSnapshot(RunSubsystem->CurrentBuild);
		OnWeaponChanged.Broadcast(WeaponId);
	}

	return true;
}
