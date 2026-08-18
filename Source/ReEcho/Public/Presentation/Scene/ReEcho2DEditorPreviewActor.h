#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ReEcho2DEditorPreviewActor.generated.h"

class UChildActorComponent;
class USceneComponent;

/** Editor-only host that previews an exact runtime Gameplay Blueprint without entering PIE. */
UCLASS()

class REECHO_API AReEcho2DEditorPreviewActor : public AActor
{
	GENERATED_BODY()

public:
	AReEcho2DEditorPreviewActor();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Preview")
	TObjectPtr<USceneComponent> PreviewRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Preview")
	TObjectPtr<UChildActorComponent> CharacterPreview;

	/** 与 GameMode 运行时生成路径共用的 Gameplay Blueprint 类。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Preview")
	TSubclassOf<AActor> PreviewActorClass;
};
