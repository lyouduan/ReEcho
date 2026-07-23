#include "Player/ReEchoPlayerPawn.h"

#include "Camera/CameraComponent.h"
#include "Combat/ReEchoCombatantComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/PlayerController.h"
#include "Graybox/ReEchoEnemyActor.h"
#include "Graybox/ReEchoAttackEffects.h"
#include "Graybox/ReEchoHealthBarActor.h"
#include "Graybox/ReEchoProjectileActor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Recording/ReEchoRecorderComponent.h"
#include "Run/ReEchoRunSubsystem.h"
#include "Weapons/ReEchoWeaponActor.h"

AReEchoPlayerPawn::AReEchoPlayerPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->InitSphereRadius(32.f);
	Collision->SetCollisionProfileName(TEXT("Pawn"));
	Shape = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlayerShape"));
	Shape->SetupAttachment(RootComponent);
	Shape->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Shape->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")));
	Shape->SetRelativeScale3D(FVector(0.65f));
	if (UMaterialInterface* Base =
	        LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
	{
		UMaterialInstanceDynamic* Mat = UMaterialInstanceDynamic::Create(Base, this);
		Mat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.1f, 1.f, 0.25f));
		Shape->SetMaterial(0, Mat);
	}
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(RootComponent);
	Camera->SetAbsolute(true, true, false);
	Camera->SetWorldLocation(FVector(-700.0f, 0.0f, 900.0f));
	Camera->SetWorldRotation(FRotator(-55.0f, 0.0f, 0.0f));
	Movement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("Movement"));
	Movement->MaxSpeed = 420.f;
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
		HealthBar->Initialize(Combatant, FLinearColor(0.1f, 1.f, 0.25f), 105.f, 1.15f);
	}

	Weapon = GetWorld()->SpawnActor<AReEchoWeaponActor>();
	if (Weapon)
	{
		Weapon->SetOwner(this);
		Weapon->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
		Weapon->SetActorRelativeLocation(FVector(28.0f, 0.0f, 16.0f));
		Weapon->InitializeWeapon();
	}
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
}

void AReEchoPlayerPawn::ConfigureMouseInput()
{
	if (bMouseInputConfigured)
	{
		return;
	}

	APlayerController* PlayerController =
		Cast<APlayerController>(GetController());
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
	APlayerController* PlayerController =
		Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		return;
	}

	FVector MouseWorldOrigin;
	FVector MouseWorldDirection;
	if (!PlayerController->DeprojectMousePositionToWorld(
			MouseWorldOrigin,
			MouseWorldDirection))
	{
		return;
	}

	const float VerticalDirection = MouseWorldDirection.Z;
	if (FMath::IsNearlyZero(VerticalDirection))
	{
		return;
	}

	const float PlaneDistance =
		(GetActorLocation().Z - MouseWorldOrigin.Z) / VerticalDirection;
	if (PlaneDistance <= 0.0f)
	{
		return;
	}

	const FVector MouseWorldPosition =
		MouseWorldOrigin + MouseWorldDirection * PlaneDistance;
	FVector AimDirection = MouseWorldPosition - GetActorLocation();
	AimDirection.Z = 0.0f;
	if (!AimDirection.IsNearlyZero())
	{
		SetActorRotation(AimDirection.Rotation());
	}
}

void AReEchoPlayerPawn::UpdateFixedCamera()
{
	Camera->SetWorldLocation(
		GetActorLocation() + FVector(-700.0f, 0.0f, 900.0f));
	Camera->SetWorldRotation(FRotator(-55.0f, 0.0f, 0.0f));
}

FString AReEchoPlayerPawn::GetEquippedWeaponLabel() const
{
	return Weapon ? Weapon->GetEquippedWeaponLabel() : TEXT("None");
}

void AReEchoPlayerPawn::BasicAttack()
{
	if (Weapon)
	{
		Weapon->TryBasicAttack(Combatant);
	}
}

void AReEchoPlayerPawn::SelectWeaponSlot1()
{
	if (Weapon)
	{
		Weapon->SelectWeapon(EReEchoWeaponSlot::PhysicalOrb);
	}

	if (UReEchoRunSubsystem* RunSubsystem =
			GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>())
	{
		RunSubsystem->SetEquippedWeapon(TEXT("W_J_02"));
		Recorder->UpdateBuildSnapshot(RunSubsystem->CurrentBuild);
		OnWeaponChanged.Broadcast(TEXT("W_J_02"));
	}
}

void AReEchoPlayerPawn::SelectWeaponSlot2()
{
	if (Weapon)
	{
		Weapon->SelectWeapon(EReEchoWeaponSlot::Sword);
	}

	if (UReEchoRunSubsystem* RunSubsystem =
			GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>())
	{
		RunSubsystem->SetEquippedWeapon(TEXT("W_J_01"));
		Recorder->UpdateBuildSnapshot(RunSubsystem->CurrentBuild);
		OnWeaponChanged.Broadcast(TEXT("W_J_01"));
	}
}

void AReEchoPlayerPawn::SelectWeaponSlot3()
{
	if (Weapon)
	{
		Weapon->SelectWeapon(EReEchoWeaponSlot::ElementalOrb);
	}

	if (UReEchoRunSubsystem* RunSubsystem =
			GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>())
	{
		RunSubsystem->SetEquippedWeapon(TEXT("W_J_03"));
		Recorder->UpdateBuildSnapshot(RunSubsystem->CurrentBuild);
		OnWeaponChanged.Broadcast(TEXT("W_J_03"));
	}
}
void AReEchoPlayerPawn::ActivateSkill()
{
	if (Weapon && Weapon->TryActiveAttack(Combatant))
	{
		OnActiveSkill.Broadcast(GetActorLocation(), Weapon->GetEquippedWeaponId());
	}
}

