#include "Graybox/ReEchoHealthBarActor.h"

#include "Camera/PlayerCameraManager.h"
#include "Combat/ReEchoCombatantComponent.h"
#include "Components/SceneComponent.h"
#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Math/RotationMatrix.h"
#include "UI/ReEchoHealthBarWidget.h"
#include "UObject/ConstructorHelpers.h"

AReEchoHealthBarActor::AReEchoHealthBarActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Root->SetAbsolute(false, true, true);
	Widget = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthWidget"));
	Widget->SetupAttachment(Root);
	Widget->SetWidgetSpace(EWidgetSpace::World);
	Widget->SetDrawSize(FVector2D(96.f, 10.f));
	Widget->SetPivot(FVector2D(0.5f, 0.5f));
	Widget->SetTwoSided(false);
	Widget->SetCastShadow(false);
	Widget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FClassFinder<UReEchoHealthBarWidget> HealthBarClassFinder(
	    TEXT("/Game/ReEcho/UI/WBP_ReEchoEnemyHealthBar"));
	Widget->SetWidgetClass(HealthBarClassFinder.Class ? HealthBarClassFinder.Class.Get()
	                                                : UReEchoHealthBarWidget::StaticClass());
}

void AReEchoHealthBarActor::Initialize(UReEchoCombatantComponent* InCombatant,
                                       const FLinearColor& FillColor,
                                       float InHeight,
                                       float InWidthScale,
                                       USceneComponent* InVisualAnchor)
{
	Combatant = InCombatant;
	VisualAnchor = InVisualAnchor;
	Height = InHeight;
	if (InCombatant && InCombatant->GetOwner())
	{
		AttachToActor(InCombatant->GetOwner(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}
	Widget->SetDrawSize(FVector2D(96.f * InWidthScale, 10.f));
	Widget->InitWidget();
	if (UReEchoHealthBarWidget* HealthWidget = Cast<UReEchoHealthBarWidget>(Widget->GetUserWidgetObject()))
	{
		HealthWidget->InitializeHealth(InCombatant, FillColor);
	}
}

void AReEchoHealthBarActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Combatant.IsValid() || !IsValid(Combatant->GetOwner()))
	{
		Destroy();
		return;
	}
	const FVector OwnerLocation =
	    VisualAnchor.IsValid() ? VisualAnchor->GetComponentLocation() : Combatant->GetOwner()->GetActorLocation();
	const float OwnerScale = FMath::Max(Combatant->GetOwner()->GetActorScale3D().GetAbsMax(), 0.01f);
	FVector Location = OwnerLocation + FVector(0.0f, 0.0f, Height * OwnerScale);
	if (APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		const FRotator CameraRotation = Camera->GetCameraRotation();
		const FRotationMatrix CameraMatrix(CameraRotation);
		Location = OwnerLocation + CameraMatrix.GetUnitAxis(EAxis::Z) * Height * OwnerScale;
		SetActorRotation((-CameraRotation.Vector()).Rotation());
	}
	SetActorScale3D(FVector(OwnerScale));
	SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
	SetActorHiddenInGame(Combatant->CurrentHealth <= 0.f);
}
