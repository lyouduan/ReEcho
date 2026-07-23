#pragma once

#include "AbilitySystemInterface.h"
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "ReEchoPlayerPawn.generated.h"

class AReEchoEnemyActor;
class AReEchoHealthBarActor;
class AReEchoWeaponActor;
class UAbilitySystemComponent;
class UBillboardComponent;
class UCameraComponent;
class UFloatingPawnMovement;
class UGameplayAbility;
class UReEchoCombatantComponent;
class UReEchoRecorderComponent;
class USphereComponent;
class UStaticMeshComponent;

enum class EReEchoWeaponSlot : uint8;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FReEchoActiveSkill, FVector, Position, FName, SkillId);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FReEchoWeaponChanged, FName, WeaponId);

UCLASS(Blueprintable)

class REECHO_API AReEchoPlayerPawn : public APawn, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AReEchoPlayerPawn();

	virtual void Tick(float DeltaSeconds) override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	FString GetEquippedWeaponLabel() const;
	bool ExecuteBasicAttackAbility();
	bool ExecuteActiveAttackAbility();
	bool ExecuteSelectWeaponSlot1Ability();
	bool ExecuteSelectWeaponSlot2Ability();
	bool ExecuteSelectWeaponSlot3Ability();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMeshComponent> Shape;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMeshComponent> GroundShadow;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UBillboardComponent> CharacterSprite;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UFloatingPawnMovement> Movement;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	TObjectPtr<UAbilitySystemComponent> AbilitySystem;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UReEchoCombatantComponent> Combatant;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UReEchoRecorderComponent> Recorder;

	UPROPERTY(BlueprintAssignable)
	FReEchoActiveSkill OnActiveSkill;

	UPROPERTY(BlueprintAssignable)
	FReEchoWeaponChanged OnWeaponChanged;

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
	void MoveForward(float Value);
	void MoveRight(float Value);
	void ActivateSkill();
	void BasicAttack();
	void TogglePauseMenu();
	void SelectWeaponSlot1();
	void SelectWeaponSlot2();
	void SelectWeaponSlot3();
	void ConfigureMouseInput();
	void UpdateMouseAim();
	void UpdateFixedCamera();
	void GrantStartupAbilities();
	bool TryActivatePlayerAbility(TSubclassOf<UGameplayAbility> AbilityClass);
	bool ExecuteSelectWeaponAbility(EReEchoWeaponSlot WeaponSlot, FName WeaponId);

	UPROPERTY()
	TObjectPtr<AReEchoHealthBarActor> HealthBar;

	UPROPERTY()
	TObjectPtr<AReEchoWeaponActor> Weapon;

	bool bMouseInputConfigured = false;
};