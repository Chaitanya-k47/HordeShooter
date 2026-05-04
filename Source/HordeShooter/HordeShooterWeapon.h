// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HordeShooterWeapon.generated.h"

//DELEGATE declaration:
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAmmoChangedSignature, int32, CurrentAmmo, int32, MagSize);

class USceneComponent;
class USkeletalMeshComponent;
class AHordeShooterCharacter;
class UAnimMontage;

UCLASS()
class HORDESHOOTER_API AHordeShooterWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AHordeShooterWeapon();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:

	//Components and State:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
	USkeletalMeshComponent* Mesh;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "State")
	AHordeShooterCharacter* CurrentOwner;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "State")
	bool bIsEquipped = false;


	//WEAPON STATS:
	UPROPERTY(EditDefaultsOnly, Category = "Weapon Stats")
	float BaseDamage = 25.f;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Stats")
	float WeaponRange = 10000.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Weapon Stats")
	float FireRate = 0.1f; //time in seconds between shots, 0.1: 10 shots per second

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Stats")
	bool bIsAutomatic = true;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Stats")
	float ShotImpulse = 5000.f; //force applied to enemies on kill or hit.


	//AMMO SYSTEM:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon Stats|Ammo")
	int32 MagSize = 30;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon Stats|Ammo")
	int32 CurrentAmmo;


	//ANIMATIONS:
	UPROPERTY(EditDefaultsOnly, Category = "Animations")
	UAnimMontage* ArmsFireMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Animations")
	UAnimMontage* ArmsFireMontage_Aimed;

	UPROPERTY(EditDefaultsOnly, Category = "Animations")
	UAnimMontage* ArmsReloadMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Animations")
	UAnimMontage* WeaponFireMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Animations")
	UAnimMontage* WeaponReloadMontage;


	//CORE FUNCTIONS:
	//these are called by character class
	virtual void StartFire();
	virtual void StopFire();
	virtual void Reload();


	//RELOAD AND FIRE HELPERS:
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnAmmoChangedSignature OnAmmoChanged;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Stats|Ammo")
	float ReloadTime = 2.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon Stats|State")
	bool bIsReloading = false;


protected:
	//this fn performs raycast.
	virtual void PerformFire();


private:

	//fire:
	FTimerHandle FireTimerHandle;
	bool bCanFire = true;
	void ResetFireCooldown();

	//reload:
	FTimerHandle ReloadTimerHandle;
	void FinishReload();

};
