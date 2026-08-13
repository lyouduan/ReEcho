#include "Player/ReEchoPlayerPawn.h"

#include "ReEcho.h"
#include "AbilitySystem/ReEchoPlayerAbilities.h"
#include "AbilitySystem/ReEchoCombatAttributeSet.h"
#include "AbilitySystem/ReEchoGameplayTags.h"
#include "AbilitySystemComponent.h"

#include "Camera/CameraComponent.h"
#include "Combat/ReEchoAttackControllerComponent.h"
#include "Combat/ReEchoCombatContracts.h"
#include "Combat/ReEchoCombatAudioAdapterComponent.h"
#include "Combat/ReEchoCombatantComponent.h"
#include "Combat/ReEchoCombatTarget.h"
#include "Core/ReEchoBalanceSettings.h"
#include "Data/ReEchoCsvDataRegistry.h"
#include "Components/BillboardComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
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
#include "PaperFlipbook.h"
#include "Presentation/Animation2D/ReEcho2DAnimationComponent.h"
#include "Presentation/Animation2D/ReEcho2DAnimationProfile.h"
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
	VisualEffectRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualEffectRoot"));
	VisualEffectRoot->SetupAttachment(RootComponent);
	CharacterSprite = CreateDefaultSubobject<UBillboardComponent>(TEXT("CharacterSprite"));
	CharacterSprite->SetupAttachment(VisualEffectRoot);
	CharacterSprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CharacterSprite->SetHiddenInGame(false);
	CharacterSprite->SetVisibility(true);
	CharacterSprite->SetRelativeLocation(FVector::ZeroVector);
	CharacterSprite->bIsScreenSizeScaled = false;
	SequenceAnimation = CreateDefaultSubobject<UReEcho2DAnimationComponent>(TEXT("SequenceAnimation"));
	SequenceAnimation->SetupAttachment(VisualEffectRoot);
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
	    TEXT("/Game/2DAnim/Player/Idel_01.Idel_01"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> CloverTextureFinder(
	    TEXT("/Game/ReEcho/Textures/Characters/NewCast/Player_Clover.Player_Clover"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> DiamondTextureFinder(
	    TEXT("/Game/ReEcho/Textures/Characters/NewCast/Player_Diamond.Player_Diamond"));
	CharacterTextures.Add(TEXT("J_CAT"), CatTextureFinder.Object);
	CharacterTextures.Add(TEXT("J_HEART"), HeartTextureFinder.Object);
	CharacterTextures.Add(TEXT("J_SPADE"), SpadeTextureFinder.Object);
	CharacterTextures.Add(TEXT("J_CLOVER"), CloverTextureFinder.Object);
	CharacterTextures.Add(TEXT("J_DIAMOND"), DiamondTextureFinder.Object);
	static ConstructorHelpers::FObjectFinder<UPaperFlipbook> SpadeIdleFinder(TEXT("/Game/2DAnim/Flipbook/Idel.Idel"));
	SpadeIdleFlipbook = SpadeIdleFinder.Object;
	static ConstructorHelpers::FObjectFinder<UPaperFlipbook> SpadeAttackFinder(TEXT("/Game/2DAnim/Flipbook/s.s"));
	SpadeAttackFlipbook = SpadeAttackFinder.Object;
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
	CombatAttributes = CreateDefaultSubobject<UReEchoCombatAttributeSet>(TEXT("CombatAttributes"));
	AbilitySystem->SetIsReplicated(true);
	AbilitySystem->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	Combatant = CreateDefaultSubobject<UReEchoCombatantComponent>(TEXT("Combatant"));
	AttackController = CreateDefaultSubobject<UReEchoAttackControllerComponent>(TEXT("AttackController"));
	Targeting = CreateDefaultSubobject<UReEchoTargetingComponent>(TEXT("Targeting"));
	CombatEvents = CreateDefaultSubobject<UReEchoCombatEventsComponent>(TEXT("CombatEvents"));
	CombatAudioAdapter = CreateDefaultSubobject<UReEchoCombatAudioAdapterComponent>(TEXT("CombatAudioAdapter"));
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
	CurrentCharacterId = CharacterId;
	Current2DAnimationState = EReEcho2DAnimationState::Idle;
	constexpr float CharacterWorldHeight = 224.0f;
	CharacterSprite->SetSprite(Texture);
	const float TextureScale = CharacterWorldHeight / FMath::Max(1, Texture->GetSizeY());
	static const FName SpadeCharacterId(TEXT("J_SPADE"));
	CharacterSprite->SetRelativeScale3D(CharacterId == SpadeCharacterId ? FVector::OneVector : FVector(TextureScale));
	IdleAnimationFrames = {Texture};
	AttackAnimationFrames = {Texture};
	SequenceAnimation->DeactivateAnimation();
	CharacterSprite->SetVisibility(true);
	CharacterSprite->SetHiddenInGame(false);
	VisualEffectRoot->SetRelativeLocation(FVector::ZeroVector);
	VisualEffectRoot->SetRelativeScale3D(FVector::OneVector);
	BaseVisualLocation = VisualEffectRoot->GetRelativeLocation();
	BaseVisualScale = VisualEffectRoot->GetRelativeScale3D();
	return true;
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
	return Weapon->GetEquippedWeaponId() == Build.WeaponId;
}

FString AReEchoPlayerPawn::GetPinnedWeaponDomainRevision() const
{
	return Weapon ? Weapon->GetPinnedWeaponDomainRevision() : FString();
}

void AReEchoPlayerPawn::BeginPlay()
{
	Super::BeginPlay();
	UpdateFollowCamera();
	BaseVisualLocation = VisualEffectRoot->GetRelativeLocation();
	BaseVisualScale = VisualEffectRoot->GetRelativeScale3D();

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
	}

	AbilitySystem->InitAbilityActorInfo(this, this);
	Combatant->BindToAbilitySystem(AbilitySystem);
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
	if (!SequenceAnimation->IsAnimationActive())
	{
		ReEchoBillboardDebug::DrawBounds(this, CharacterSprite, FColor::Green);
	}
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
		VisualFacingSign = ToTarget.X >= 0.0f ? 1.0f : -1.0f;
		SetActorRotation(ToTarget.Rotation());
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
		VisualFacingSign = ToTarget.X >= 0.0f ? 1.0f : -1.0f;
		SetActorRotation(ToTarget.Rotation());
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
	static const FName MoonStaffWeaponId(TEXT("W_J_02"));
	if (CurrentCharacterId == TEXT("J_SPADE") && Weapon && Weapon->GetEquippedWeaponId() == MoonStaffWeaponId &&
	    SpadeAttackFlipbook)
	{
		SequenceAttackRemaining = SpadeAttackFlipbook->GetTotalDuration();
		if (Current2DAnimationState == EReEcho2DAnimationState::Attack && SequenceAnimation->IsAnimationActive())
		{
			SequenceAnimation->SetAnimationState(EReEcho2DAnimationState::Attack, false, true);
		}
	}
}

void AReEchoPlayerPawn::UpdateSpriteAnimation(const float DeltaSeconds)
{
	if (!VisualEffectRoot)
	{
		return;
	}
	VisualTime += DeltaSeconds;
	AttackVisualRemaining = FMath::Max(0.0f, AttackVisualRemaining - DeltaSeconds);
	SequenceAttackRemaining = FMath::Max(0.0f, SequenceAttackRemaining - DeltaSeconds);
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
	VisualEffectRoot->SetRelativeLocation(BaseVisualLocation + FVector(Lunge, 0.0f, Bob));
	VisualEffectRoot->SetRelativeScale3D(BaseVisualScale * FVector(ScaleX, ScaleY, 1.0f));
	UpdateSpadeAnimationState(bMoving);
	UpdateSequenceFrame();
}

void AReEchoPlayerPawn::UpdateSpadeAnimationState(const bool bMoving)
{
	static const FName SpadeCharacterId(TEXT("J_SPADE"));
	static const FName MoonStaffWeaponId(TEXT("W_J_02"));
	if (CurrentCharacterId != SpadeCharacterId)
	{
		TransitionSpadeAnimationState(EReEcho2DAnimationState::Idle);
		return;
	}

	const bool bMoonStaffAttack =
	    SequenceAttackRemaining > 0.0f && Weapon && Weapon->GetEquippedWeaponId() == MoonStaffWeaponId;
	const EReEcho2DAnimationState DesiredState = bMoonStaffAttack ? EReEcho2DAnimationState::Attack
	                                             : bMoving        ? EReEcho2DAnimationState::Move
	                                                              : EReEcho2DAnimationState::Idle;
	TransitionSpadeAnimationState(DesiredState);
}

void AReEchoPlayerPawn::TransitionSpadeAnimationState(const EReEcho2DAnimationState NewState)
{
	if (Current2DAnimationState == NewState)
	{
		return;
	}
	Current2DAnimationState = NewState;
	if (NewState == EReEcho2DAnimationState::Idle)
	{
		SequenceAnimation->DeactivateAnimation();
		CharacterSprite->SetVisibility(true);
		CharacterSprite->SetHiddenInGame(false);
		return;
	}
	if (NewState == EReEcho2DAnimationState::Attack && !SpadeAttackFlipbook)
	{
		Current2DAnimationState = EReEcho2DAnimationState::Idle;
		SequenceAnimation->DeactivateAnimation();
		CharacterSprite->SetVisibility(true);
		CharacterSprite->SetHiddenInGame(false);
		return;
	}

	FReEcho2DAnimationProfile Profile;
	Profile.DefaultFlipbook = SpadeIdleFlipbook;
	Profile.StateFlipbooks.Add(EReEcho2DAnimationState::Attack, SpadeAttackFlipbook);
	Profile.WorldHeight = 224.0f;
	Profile.bUseNativeScale = true;
	Profile.TranslucentSortPriority = 10;
	if (!SequenceAnimation->IsAnimationActive() &&
	    SequenceAnimation->ActivateProfile(Profile) != EReEcho2DAnimationActivationResult::Activated)
	{
		Current2DAnimationState = EReEcho2DAnimationState::Idle;
		CharacterSprite->SetVisibility(true);
		CharacterSprite->SetHiddenInGame(false);
		return;
	}

	SequenceAnimation->SetAnimationState(NewState, NewState == EReEcho2DAnimationState::Move);
	CharacterSprite->SetVisibility(false);
	CharacterSprite->SetHiddenInGame(true);
}

void AReEchoPlayerPawn::UpdateSequenceFrame()
{
	if (SequenceAnimation && SequenceAnimation->IsAnimationActive())
	{
		SequenceAnimation->SetFacingSign(VisualFacingSign);
		return;
	}
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

void AReEchoPlayerPawn::HandleMovementSpeedAttributeChanged(const FOnAttributeChangeData& Data)
{
	if (Movement)
	{
		Movement->MaxSpeed = 420.0f * FMath::Max(0.1f, Data.NewValue);
	}
}
