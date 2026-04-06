// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "HordeShooterCharacter.generated.h"

class UInputAction;
class UInputMappingContext;
class UCameraComponent;
class USkeletalMeshComponent;
class USceneComponent;
class HordeShooterWeapon;

UCLASS()
class HORDESHOOTER_API AHordeShooterCharacter : public ACharacter
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
	UInputAction* PauseAction;


public:
	//first person camera component:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* FirstPersonCamera;

	//arms mesh:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh")
	USkeletalMeshComponent* CharacterArms;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	USceneComponent* Pivot;


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
	float SlideGravityMultiplier = 4000.f; //how fast you accelerate down a slope

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Slide")
	float MaxSlideSpeed = 2500.f; //hHard cap


	//Weapon config:
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TArray<TSubclassOf<AHordeShooterWeapon>> DefaultWeaponClasses; //default weapon classes to spawn with and add to inventory

	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "Weapon")
	TArray<AHordeShooterWeapon*> Inventory;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "Weapon")
	AHordeShooterWeapon* CurrentEquippedWeapon;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "Weapon")
	int32 CurrentWeaponIndex = 0; //start with primary weapon equipped

	UPROPERTY(BlueprintReadOnly)
	bool bIsAiming = false;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void EquipWeapon(AHordeShooterWeapon* NewWeapon);
	
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

	//switch weapon callback:
	void SwitchWeapon(const FInputActionValue& Value);

	//Aim callback:
	void StartAiming();
	void StopAiming();

	//Pause callback:(later shift this to player controller)
	void TogglePause();

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

	float DefaultHalfHeight;
	float CrouchedHalfHeight;
	float TargetHalfHeight;

	bool bIsSliding = false;
};
