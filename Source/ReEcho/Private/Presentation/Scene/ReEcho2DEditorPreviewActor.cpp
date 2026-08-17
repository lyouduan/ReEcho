#include "Presentation/Scene/ReEcho2DEditorPreviewActor.h"

#include "Components/ChildActorComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"

AReEcho2DEditorPreviewActor::AReEcho2DEditorPreviewActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bIsEditorOnlyActor = true;
	PreviewRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PreviewRoot"));
	SetRootComponent(PreviewRoot);
	CharacterPreview = CreateDefaultSubobject<UChildActorComponent>(TEXT("CharacterPreview"));
	CharacterPreview->SetupAttachment(PreviewRoot);
	CharacterPreview->SetIsVisualizationComponent(true);
	// HiddenInGame keeps the exact Blueprint visible in the editor viewport while
	// guaranteeing that its render children never leak into PIE for even one frame.
	CharacterPreview->SetHiddenInGame(true);
}

void AReEcho2DEditorPreviewActor::BeginPlay()
{
	Super::BeginPlay();
	// Editor-only actors are still duplicated into some PIE paths before cook filtering. Remove the
	// gameplay ChildActor explicitly so previews can never contribute collision, AI or presentation.
	if (CharacterPreview)
	{
		CharacterPreview->DestroyChildActor();
		CharacterPreview->SetChildActorClass(nullptr);
	}
	SetActorEnableCollision(false);
	SetActorHiddenInGame(true);
	Destroy();
}

void AReEcho2DEditorPreviewActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (const UWorld* World = GetWorld(); World &&
	    (World->WorldType == EWorldType::PIE || World->WorldType == EWorldType::Game))
	{
		CharacterPreview->DestroyChildActor();
		CharacterPreview->SetChildActorClass(nullptr);
		return;
	}
	if (CharacterPreview->GetChildActorClass() != PreviewActorClass)
	{
		CharacterPreview->SetChildActorClass(PreviewActorClass);
	}
}
