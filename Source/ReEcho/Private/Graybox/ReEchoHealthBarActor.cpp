#include "Graybox/ReEchoHealthBarActor.h"

#include "Camera/PlayerCameraManager.h"
#include "Combat/ReEchoCombatantComponent.h"
#include "Components/SceneComponent.h"
#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UI/ReEchoHealthBarWidget.h"

AReEchoHealthBarActor::AReEchoHealthBarActor()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Widget = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthWidget"));
	Widget->SetupAttachment(Root);
	Widget->SetWidgetSpace(EWidgetSpace::World);
	Widget->SetDrawSize(FVector2D(180.f, 24.f));
	Widget->SetPivot(FVector2D(0.5f, 0.5f));
	Widget->SetTwoSided(false);
	Widget->SetCastShadow(false);
	Widget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Widget->SetWidgetClass(UReEchoHealthBarWidget::StaticClass());
}

void AReEchoHealthBarActor::Initialize(UReEchoCombatantComponent* InCombatant,
                                       const FLinearColor& FillColor,
                                       float InHeight,
                                       float InWidthScale)
{
	Combatant = InCombatant;
	Height = InHeight;
	Widget->SetDrawSize(FVector2D(180.f * InWidthScale, 24.f));
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
	const FVector Location = Combatant->GetOwner()->GetActorLocation() + FVector(0.f, 0.f, Height);
	SetActorLocation(Location);
	if (APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		SetActorRotation((Camera->GetCameraLocation() - Location).Rotation());
	}
	SetActorHiddenInGame(Combatant->CurrentHealth <= 0.f);
}

