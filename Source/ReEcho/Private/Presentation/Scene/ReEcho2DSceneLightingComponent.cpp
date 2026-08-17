#include "Presentation/Scene/ReEcho2DSceneLightingComponent.h"

#include "EngineUtils.h"
#include "PaperFlipbookComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Presentation/Scene/ReEchoArenaSceneActor.h"

UReEcho2DSceneLightingComponent::UReEcho2DSceneLightingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UReEcho2DSceneLightingComponent::BeginPlay()
{
	Super::BeginPlay();
	ResolveArenaScene();
	ApplyFootpointSorting();
}

void UReEcho2DSceneLightingComponent::TickComponent(const float DeltaTime,
                                                    const ELevelTick TickType,
                                                    FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!ArenaScene)
	{
		ResolveArenaScene();
	}
	ApplyFootpointSorting();
}

void UReEcho2DSceneLightingComponent::Configure(UPaperFlipbookComponent* InAnimationRenderer,
                                               UPrimitiveComponent* InGroundShadow)
{
	AnimationRenderer = InAnimationRenderer;
	GroundShadow = InGroundShadow;
}

void UReEcho2DSceneLightingComponent::ResolveArenaScene()
{
	if (!GetWorld())
	{
		return;
	}
	for (TActorIterator<AReEchoArenaSceneActor> It(GetWorld()); It; ++It)
	{
		ArenaScene = *It;
		break;
	}
}

void UReEcho2DSceneLightingComponent::ApplyFootpointSorting()
{
	const AActor* Owner = GetOwner();
	if (AnimationRenderer && ArenaScene && Owner)
	{
		const int32 FlipbookSortPriority = ArenaScene->CalculateFootpointSortPriority(Owner->GetActorLocation());
		AnimationRenderer->SetTranslucentSortPriority(FlipbookSortPriority);
		if (GroundShadow)
		{
			GroundShadow->SetTranslucentSortPriority(FlipbookSortPriority - 1);
		}
	}
}
