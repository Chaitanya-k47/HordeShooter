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
class UNiagaraSystem;
class UNiagaraComponent;
class UMaterialInterface;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
	UNiagaraComponent* BarrelSmokeComp;


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

	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	float BarrelSmokeDuration = 3.0f; // How long the smoke lingers after shooting


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


	//FX:
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	UNiagaraSystem* MuzzleFlashSystem;

	// UPROPERTY(EditDefaultsOnly, Category = "Effects")
	// UNiagaraSystem* SmokeTrailSystem;

	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	UNiagaraSystem* TracerSystem;

	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	UNiagaraSystem* ImpactSystem;

	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	UMaterialInterface* BulletHoleDecal;
	
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	int32 TotalDecalVariations = 64;


	//SFX
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	USoundBase* ShootSound;

	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	USoundBase* ImpactSound;
	

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

	bool bCanFire = true;


protected:
	//this fn performs raycast.
	virtual void PerformFire();


private:

	//fire:
	FTimerHandle FireTimerHandle;
	
	void ResetFireCooldown();

	//reload:
	FTimerHandle ReloadTimerHandle;
	void FinishReload();

	//decals:
	TArray<int32> AvailableDecalIndices;

	//helper to get a random unique decal index for each shot.
	int32 GetUniqueDecalIndex();

	//barrel smoke:
	FTimerHandle SmokeTimerHandle;
	void StopBarrelSmoke();

};
