#include "Player/ReEchoPlayerPawn.h"

#include "AbilitySystem/ReEchoPlayerAbilities.h"
#include "AbilitySystemComponent.h"

#include "Camera/CameraComponent.h"
#include "Combat/ReEchoCombatantComponent.h"
#include "Core/ReEchoBalanceSettings.h"
#include "Components/BillboardComponent.h"
#include "Components/CapsuleComponent.h"
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
#include "Graybox/ReEchoProjectileActor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Recording/ReEchoRecorderComponent.h"
#include "Run/ReEchoRunSubsystem.h"
#include "ReEchoGameMode.h"
#include "Weapons/ReEchoWeaponActor.h"
#include "UObject/ConstructorHelpers.h"

AReEchoPlayerPawn::AReEchoPlayerPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	constexpr float CharacterWorldHeight = 224.0f;
	Collision = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->InitCapsuleSize(27.2f, CharacterWorldHeight * 0.5f);
	Collision->SetVisibility(false);
	Collision->SetCollisionProfileName(TEXT("Pawn"));
	GroundShadow = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GroundShadow"));
	GroundShadow->SetupAttachment(RootComponent);
	GroundShadow->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GroundShadow->SetCastShadow(false);
	GroundShadow->SetTranslucentSortPriority(-0.8);
	GroundShadow->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")));
	GroundShadow->SetRelativeLocation(FVector(0.0f, 0.0f, -CharacterWorldHeight * 0.28f));
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
	CharacterSprite->SetRelativeLocation(FVector::ZeroVector);
	CharacterSprite->bIsScreenSizeScaled = false;
	if (UTexture2D* CharacterTexture =
	        LoadObject<UTexture2D>(nullptr, TEXT("/Game/ReEcho/Textures/Characters/Player2D.Player2D")))
	{
		CharacterSprite->SetSprite(CharacterTexture);
		const float TextureScale = CharacterWorldHeight / FMath::Max(1, CharacterTexture->GetSizeY());
		CharacterSprite->SetRelativeScale3D(FVector(TextureScale));
	}
	static ConstructorHelpers::FObjectFinder<UTexture2D> CatTextureFinder(
	    TEXT("/Game/ReEcho/Textures/Characters/NewCast/Player_Cat.Player_Cat"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> HeartTextureFinder(
	    TEXT("/Game/ReEcho/Textures/Characters/NewCast/Player_Heart.Player_Heart"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> SpadeTextureFinder(
	    TEXT("/Game/ReEcho/Textures/Characters/NewCast/Player_Spade.Player_Spade"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> CloverTextureFinder(
	    TEXT("/Game/ReEcho/Textures/Characters/NewCast/Player_Clover.Player_Clover"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> DiamondTextureFinder(
	    TEXT("/Game/ReEcho/Textures/Characters/NewCast/Player_Diamond.Player_Diamond"));
	CharacterTextures.Add(TEXT("J_CAT"), CatTextureFinder.Object);
	CharacterTextures.Add(TEXT("J_HEART"), HeartTextureFinder.Object);
	CharacterTextures.Add(TEXT("J_SPADE"), SpadeTextureFinder.Object);
	CharacterTextures.Add(TEXT("J_CLOVER"), CloverTextureFinder.Object);
	CharacterTextures.Add(TEXT("J_DIAMOND"), DiamondTextureFinder.Object);
	ConfigureCharacter(TEXT("J_CAT"));
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(RootComponent);
	Camera->SetAbsolute(true, true, true);
	Camera->SetProjectionMode(ECameraProjectionMode::Orthographic);
	Camera->SetOrthoWidth(GetDefault<UReEchoBalanceSettings>()->ArenaSceneWorldHeight * (16.0f / 9.0f));
	Camera->SetAspectRatio(16.0f / 9.0f);
	Camera->SetConstraintAspectRatio(true);
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

void AReEchoPlayerPawn::ConfigureArenaBounds(const float HalfExtentX, const float HalfExtentY)
{
	ArenaHalfExtents.X = FMath::Max(100.0f, HalfExtentX);
	ArenaHalfExtents.Y = FMath::Max(100.0f, HalfExtentY);
}

bool AReEchoPlayerPawn::ConfigureCharacter(const FName CharacterId)
{
	const TObjectPtr<UTexture2D>* TextureEntry = CharacterTextures.Find(CharacterId);
	if (!TextureEntry || !TextureEntry->Get())
	{
		return false;
	}

	UTexture2D* Texture = TextureEntry->Get();
	constexpr float CharacterWorldHeight = 224.0f;
	CharacterSprite->SetSprite(Texture);
	const float TextureScale = CharacterWorldHeight / FMath::Max(1, Texture->GetSizeY());
	CharacterSprite->SetRelativeScale3D(FVector(TextureScale));
	IdleAnimationFrames = {Texture};
	AttackAnimationFrames = {Texture};
	BaseSpriteScale = CharacterSprite->GetRelativeScale3D();
	return true;
}

void AReEchoPlayerPawn::BeginPlay()
{
	Super::BeginPlay();
	UpdateFollowCamera();
	BaseSpriteLocation = CharacterSprite->GetRelativeLocation();
	BaseSpriteScale = CharacterSprite->GetRelativeScale3D();

	ConfigureMouseInput();

	Weapon = GetWorld()->SpawnActor<AReEchoWeaponActor>();
	if (Weapon)
	{
		Weapon->SetOwner(this);
		Weapon->AttachToActor(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		Weapon->SetActorRelativeLocation(FVector::ZeroVector);
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
	Input->BindAction(TEXT("BasicAttack"), IE_Released, this, &AReEchoPlayerPawn::StopBasicAttack);
	Input->BindAction(TEXT("ActiveSkill"), IE_Pressed, this, &AReEchoPlayerPawn::ActivateSkill);
	Input->BindAction(TEXT("WeaponSlot1"), IE_Pressed, this, &AReEchoPlayerPawn::SelectWeaponSlot1);
	Input->BindAction(TEXT("WeaponSlot2"), IE_Pressed, this, &AReEchoPlayerPawn::SelectWeaponSlot2);
	Input->BindAction(TEXT("WeaponSlot3"), IE_Pressed, this, &AReEchoPlayerPawn::SelectWeaponSlot3);
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
	UpdateMouseAim();
	if (bBasicAttackHeld)
	{
		TryActivatePlayerAbility(UReEchoBasicAttackAbility::StaticClass());
	}
	UpdateSpriteAnimation(DeltaSeconds);
	ReEchoBillboardDebug::DrawBounds(this, CharacterSprite, FColor::Green);
	ReEchoCollisionDebug::DrawCapsule(this, Collision, FColor::Cyan);
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
		const float HorizontalAim = FVector::DotProduct(AimDirection, Camera->GetRightVector());
		if (FMath::Abs(HorizontalAim) > 5.0f)
		{
			VisualFacingSign = HorizontalAim >= 0.0f ? 1.0f : -1.0f;
		}
		SetActorRotation(AimDirection.Rotation());
	}
}

void AReEchoPlayerPawn::ConstrainToArenaBounds()
{
	if (ArenaHalfExtents.X <= 0.0f || ArenaHalfExtents.Y <= 0.0f)
	{
		return;
	}
	FVector Location = GetActorLocation();
	Location.X = FMath::Clamp(Location.X, -ArenaHalfExtents.X, ArenaHalfExtents.X);
	Location.Y = FMath::Clamp(Location.Y, -ArenaHalfExtents.Y, ArenaHalfExtents.Y);
	SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
}

void AReEchoPlayerPawn::UpdateFollowCamera()
{
	// 固定正交镜头完整覆盖 11200×6300 的16:9场景背景。
	Camera->SetWorldLocation(FVector(-700.0f, 0.0f, 900.0f));
	Camera->SetWorldRotation(FRotator(-55.0f, 0.0f, 0.0f));
	Camera->SetOrthoWidth(GetDefault<UReEchoBalanceSettings>()->ArenaSceneWorldHeight * (16.0f / 9.0f));
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

void AReEchoPlayerPawn::ToggleInventoryMenu()
{
	if (AReEchoGameMode* GameMode = GetWorld()->GetAuthGameMode<AReEchoGameMode>())
	{
		GameMode->ToggleInventoryMenu();
	}
}

void AReEchoPlayerPawn::ToggleShopMenu()
{
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
	bBasicAttackHeld = true;
	TryActivatePlayerAbility(UReEchoBasicAttackAbility::StaticClass());
}

void AReEchoPlayerPawn::StopBasicAttack()
{
	bBasicAttackHeld = false;
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
	const bool bAttacked = Weapon && Weapon->TryBasicAttack(Combatant);
	if (bAttacked)
	{
		StartAttackVisual(0.18f, 16.0f);
	}
	return bAttacked;
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

void AReEchoPlayerPawn::PlayHitVisual()
{
	HitVisualRemaining = 0.18f;
}

void AReEchoPlayerPawn::StartAttackVisual(const float Duration, const float Strength)
{
	AttackVisualDuration = Duration;
	AttackVisualRemaining = Duration;
	AttackVisualStrength = Strength;
}

void AReEchoPlayerPawn::UpdateSpriteAnimation(const float DeltaSeconds)
{
	if (!CharacterSprite)
	{
		return;
	}
	VisualTime += DeltaSeconds;
	AttackVisualRemaining = FMath::Max(0.0f, AttackVisualRemaining - DeltaSeconds);
	HitVisualRemaining = FMath::Max(0.0f, HitVisualRemaining - DeltaSeconds);
	const bool bMoving = GetVelocity().SizeSquared2D() > 25.0f;
	const float Bob = FMath::Sin(VisualTime * (bMoving ? 10.0f : 3.0f)) * (bMoving ? 4.0f : 1.8f);
	float Lunge = 0.0f;
	float ScaleX = 1.0f;
	float ScaleY = 1.0f;
	if (AttackVisualRemaining > 0.0f && AttackVisualDuration > 0.0f)
	{
		const float Progress = 1.0f - AttackVisualRemaining / AttackVisualDuration;
		const float Pulse = FMath::Sin(Progress * PI);
		Lunge = Pulse * AttackVisualStrength;
		ScaleX += Pulse * 0.08f;
		ScaleY -= Pulse * 0.04f;
	}
	if (HitVisualRemaining > 0.0f)
	{
		const float HitRatio = HitVisualRemaining / 0.18f;
		Lunge -= FMath::Sin(VisualTime * 85.0f) * 7.0f * HitRatio;
		ScaleX *= 1.12f;
		ScaleY *= 0.86f;
	}
	CharacterSprite->SetRelativeLocation(BaseSpriteLocation + FVector(Lunge, 0.0f, Bob));
	CharacterSprite->SetRelativeScale3D(BaseSpriteScale * FVector(ScaleX, ScaleY, 1.0f));
	UpdateSequenceFrame();
}

void AReEchoPlayerPawn::UpdateSequenceFrame()
{
	UTexture2D* Frame = nullptr;
	if (AttackVisualRemaining > 0.0f && AttackVisualDuration > 0.0f && AttackAnimationFrames.Num() > 0)
	{
		const float Progress = 1.0f - AttackVisualRemaining / AttackVisualDuration;
		const int32 FrameIndex =
		    FMath::Clamp(FMath::FloorToInt(Progress * AttackAnimationFrames.Num()), 0, AttackAnimationFrames.Num() - 1);
		Frame = AttackAnimationFrames[FrameIndex];
	}
	else if (IdleAnimationFrames.Num() > 0)
	{
		constexpr float IdleFramesPerSecond = 4.0f;
		const int32 FrameIndex = FMath::FloorToInt(VisualTime * IdleFramesPerSecond) % IdleAnimationFrames.Num();
		Frame = IdleAnimationFrames[FrameIndex];
	}

	const bool bFrameChanged = Frame && CharacterSprite->Sprite != Frame;
	if (bFrameChanged)
	{
		CharacterSprite->SetSprite(Frame);
	}
	if (Frame && (bFrameChanged || !FMath::IsNearlyEqual(AppliedVisualFacingSign, VisualFacingSign)))
	{
		const int32 FrameWidth = Frame->GetSizeX();
		const int32 FrameHeight = Frame->GetSizeY();
		if (VisualFacingSign < 0.0f)
		{
			CharacterSprite->SetUV(FrameWidth, -FrameWidth, 0, FrameHeight);
		}
		else
		{
			CharacterSprite->SetUV(0, FrameWidth, 0, FrameHeight);
		}
		AppliedVisualFacingSign = VisualFacingSign;
	}
}
