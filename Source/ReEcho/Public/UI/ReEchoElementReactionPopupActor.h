#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ReEchoElementReactionPopupActor.generated.h"

class UMaterialBillboardComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class USceneComponent;
class UTexture2D;
struct FReEchoElementReactionResolvedEvent;

/** Displays one disposable world-space label for an authoritative element-reaction event. */
UCLASS()

class REECHO_API AReEchoElementReactionPopupActor : public AActor
{
	GENERATED_BODY()

public:
	AReEchoElementReactionPopupActor();

	virtual void Tick(float DeltaSeconds) override;

	static AReEchoElementReactionPopupActor*
	SpawnReactionPopup(UWorld* World, const FVector& WorldLocation, FName ReactionBehaviorId);
	static const TCHAR* GetReactionTexturePath(FName ReactionBehaviorId);
	static const TCHAR* GetPopupMaterialPath();
	static const TCHAR* GetPopupBlueprintClassPath();
	static bool ShouldDisplayForTarget(const FReEchoElementReactionResolvedEvent& Event, const AActor* Target);
	static float CalculateOpacity(float LifeProgress, float FadeStartFraction, float FadeCurveExponent);

private:
	bool InitializePopup(FName ReactionBehaviorId);
	UTexture2D* ResolveReactionTexture(FName ReactionBehaviorId) const;
	void RefreshBillboard(UTexture2D& Texture);
	void FacePlayerCamera();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UMaterialBillboardComponent> Visual;

	UPROPERTY()
	TObjectPtr<UTexture2D> BurnTexture;

	UPROPERTY()
	TObjectPtr<UTexture2D> VaporizeTexture;

	UPROPERTY()
	TObjectPtr<UTexture2D> GrowthTexture;

	UPROPERTY()
	TObjectPtr<UTexture2D> ConductTexture;

	UPROPERTY()
	TObjectPtr<UTexture2D> EnhanceTexture;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> PopupMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> RuntimeMaterial;

	UPROPERTY(EditDefaultsOnly,
	          BlueprintReadOnly,
	          Category = "Element Reaction Popup|Animation",
	          meta = (AllowPrivateAccess = "true", ClampMin = "0.05", Units = "s"))
	float DisplayDuration = 0.85f;

	UPROPERTY(EditDefaultsOnly,
	          BlueprintReadOnly,
	          Category = "Element Reaction Popup|Layout",
	          meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float SpawnHeightCm = 105.0f;

	UPROPERTY(EditDefaultsOnly,
	          BlueprintReadOnly,
	          Category = "Element Reaction Popup|Layout",
	          meta = (AllowPrivateAccess = "true", ClampMin = "1.0", Units = "cm"))
	float WorldHeightCm = 105.0f;

	UPROPERTY(EditDefaultsOnly,
	          BlueprintReadOnly,
	          Category = "Element Reaction Popup|Animation",
	          meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float RiseHeightCm = 90.0f;

	UPROPERTY(EditDefaultsOnly,
	          BlueprintReadOnly,
	          Category = "Element Reaction Popup|Animation",
	          meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "0.95"))
	float FadeStartFraction = 0.15f;

	UPROPERTY(EditDefaultsOnly,
	          BlueprintReadOnly,
	          Category = "Element Reaction Popup|Animation",
	          meta = (AllowPrivateAccess = "true", ClampMin = "0.01"))
	float FadeCurveExponent = 1.3f;

	UPROPERTY(EditDefaultsOnly,
	          BlueprintReadOnly,
	          Category = "Element Reaction Popup|Animation",
	          meta = (AllowPrivateAccess = "true", ClampMin = "0.01"))
	float StartScale = 0.9f;

	UPROPERTY(EditDefaultsOnly,
	          BlueprintReadOnly,
	          Category = "Element Reaction Popup|Animation",
	          meta = (AllowPrivateAccess = "true", ClampMin = "0.01"))
	float EndScale = 1.0f;

	UPROPERTY(EditDefaultsOnly,
	          BlueprintReadOnly,
	          Category = "Element Reaction Popup|Layout",
	          meta = (AllowPrivateAccess = "true"))
	int32 TranslucentSortPriority = 30;

	float ElapsedTime = 0.0f;
	FVector StartLocation = FVector::ZeroVector;
};
