// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "DamageableInterface.h"

#include "HordeShooterCharacter.generated.h"

class UInputAction;
class UInputMappingContext;
class UCameraComponent;
class USkeletalMeshComponent;
class USceneComponent;
class HordeShooterWeapon;
class USoundBase;
class UAudioComponent;

UCLASS()
class HORDESHOOTER_API AHordeShooterCharacter : public ACharacter, public IDamageableInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AHordeShooterCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;


protected:
	//Input actions:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* DashAction;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* SlideAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* FireAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* SwitchWeaponAction;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* AimAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* ReloadAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* PauseAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* GenerateArenaAction;


public:
	//player health:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Stats")
	float MaxHealth = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player Stats")
	float CurrentHealth;

	//first person camera component:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* FirstPersonCamera;

	//arms mesh:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh")
	USkeletalMeshComponent* CharacterArms;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	USceneComponent* Pivot;

	//MOVEMENT SOUNDS
	UPROPERTY(EditDefaultsOnly, Category = "Sound|Movement")
	USoundBase* JumpSound;

	UPROPERTY(EditDefaultsOnly, Category = "Sound|Movement")
	USoundBase* DashSound;

	UPROPERTY(EditDefaultsOnly, Category = "Sound|Movement")
	USoundBase* FootstepSound;

	UPROPERTY(EditDefaultsOnly, Category = "Sound|Movement")
	float DistancePerFootstep = 250.f; //for pedometer implementation

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Sound|Movement")
	UAudioComponent* SlideAudioComponent;


protected:
	//FP camera FOV config:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|FOV")
	float IdleFOV = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|FOV")
	float RunFOV = 105.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|FOV")
	float DashFOV = 130.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|FOV")
	float RunFOVInterpSpeed = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|FOV")
	float DashFOVInterpSpeed = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Lag")
	float MaxDashCameraLag = 11.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Lag")
	float CameraLagInterpSpeed = 6.f;

	UPROPERTY(BlueprintReadOnly)
	FInputActionValue MouseInputValue;

	UPROPERTY(BlueprintReadOnly)
	FInputActionValue MovementInputValue;


	//CAMERA TILT CONFIG
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Tilt")
	float MaxRunCameraTilt = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Tilt")
	float MaxDashCameraTilt = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Tilt")
	float CameraTiltInterpSpeed = 8.0f;


	//Dash config:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Dash")
	float DashSpeed = 8300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Dash")
	float DashDuration = 0.16f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Dash")
	int32 MaxDashes = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Dash")
	float SingleDashCooldown = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Dash")
	float DoubleDashCooldown = 2.3f;


	//Slide config:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Slide")
	float SlideSpeed = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Slide")
	float SlideDropInterpSpeed = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Slide")
	float SlideGravityMultiplier = 4000.f; //how fast character accelerate down a slope

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Slide")
	float MaxSlideSpeed = 2500.f; //hHard cap

	UPROPERTY(BlueprintReadOnly)
	bool bIsSliding = false;


	//Weapon config:
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TArray<TSubclassOf<AHordeShooterWeapon>> DefaultWeaponClasses; //default weapon classes to spawn with and add to inventory

	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "Weapon")
	TArray<AHordeShooterWeapon*> Inventory;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "Weapon")
	AHordeShooterWeapon* CurrentEquippedWeapon;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "Weapon")
	int32 CurrentWeaponIndex = 0; //start with primary weapon equipped


public:
	UPROPERTY(BlueprintReadOnly)
	bool bIsAiming = false;

protected:
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void EquipWeapon(AHordeShooterWeapon* NewWeapon);


	//Weapon collision/clipping config:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float WallCheckDistance = 100.f;

public:
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Weapon")
	bool IsCloseToWall();

protected:
	//Input callbacks:
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

	//Dash callbacks:
	void Dash();
	void RechargeDash();

	virtual void Landed(const FHitResult& Hit) override;

	//Slide callbacks:
	void StartSlide();
	void StopSlide();

	//Fire weapon callback:
	void FireWeapon();
	void StopFiringWeapon();

	//switch weapon callback:
	void SwitchWeapon(const FInputActionValue& Value);

	//Aim callback:
	void StartAiming();
	void StopAiming();

	//Reload callback:
	void ReloadWeapon();

	//Pause callback:(later shift this to player controller)
	void TogglePause();

	//jump fn override.
	virtual void OnJumped_Implementation() override;

	//Damageable interface:
	virtual bool ReactToHit(float DamageAmount, const FVector& HitImpulse, FName HitBoneName) override;
	void PlayerDie();

	//Generate new arena layout callback:
	void GenerateArena();

private:
	//State variables:
	bool bIsDashing = false;

	FVector CurrentDashDirection;

	float DashTimer;
	float TargetFOV;
	
	FTimerHandle DashTimerHandle;

	int32 AvailableDashes;
	int32 AirDashesUsed;

	FVector BaseCameraLocation;
	FRotator BaseCameraRotation;

	float DefaultHalfHeight;
	float CrouchedHalfHeight;
	float TargetHalfHeight;

	float AccumulatedStepDistance = 0.f; //for footstep sfx
	void PlayFootstepSound();

	//weapon switch state machine variables:
	bool bIsSwitchingWeapons = false;
	FTimerHandle WeaponSwitchTimerHandle;

	UPROPERTY()
	AHordeShooterWeapon* PendingWeapon = nullptr; //used to store the weapon we want to equip while playing equip/holster animations

	void PerformWeaponSwitch(); //happens after holster anim finishes.(updates backend and UI)
	void FinishEquipping(); //happens after equip anim finishes.


};
