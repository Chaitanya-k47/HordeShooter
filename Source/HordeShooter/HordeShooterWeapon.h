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
class AHordeShooterCasing;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
	UStaticMeshComponent* Magazine;

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


	//AMMO SYSTEM:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon Stats|Ammo")
	int32 MagSize = 45;

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
	UAnimMontage* ArmsEquipMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Animations")
	UAnimMontage* ArmsHolsterMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Animations")
	UAnimMontage* WeaponFireMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Animations")
	UAnimMontage* WeaponReloadMontage;


	//FX:
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	UNiagaraSystem* MuzzleFlashSystem;

	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	UNiagaraSystem* TracerSystem;

	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	UNiagaraSystem* ImpactSystem;

	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	UMaterialInterface* BulletHoleDecal;
	
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	int32 TotalDecalVariations = 64;

	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TSubclassOf<AHordeShooterCasing> CasingClass;


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
	virtual void OnHolstered();


	//RELOAD AND FIRE HELPERS:
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnAmmoChangedSignature OnAmmoChanged;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Stats|Ammo")
	float ReloadTime = 2.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon Stats|State")
	bool bIsReloading = false;

	bool bCanFire = true;


	//ALT FIRE HELPER:
	//decides if the weapon uses rightclick for aim or alt fire.
	UPROPERTY(EditDefaultsOnly, Category = "Weapon Stats")
	bool bCanAim = true;

	//implementation empty in this class, child classes can override.
	virtual void StartAltFire();
	virtual void StopAltFire();

protected:
	//this fn performs raycast.
	virtual void PerformFire();

	//DYNAMIC SMOKE CONFIG:
	UPROPERTY(EditDefaultsOnly, Category = "Effects|Smoke")
	float SmokeEvalWindow = 0.5f; //after how long after shooting the gun barrel is considerd cool

	UPROPERTY(EditDefaultsOnly, Category = "Effects|Smoke")
	float SmokeMultiplier = 0.2f; //how mush smoke duration per bullet.

	//CASING POOL config:
	UPROPERTY(EditDefaultsOnly, Category = "Effects|Casings")
	int32 CasingPoolSize = 45;

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
	int32 BulletsFiredConsecutively = 0;
	FTimerHandle SmokeEvalTimerHandle;
	FTimerHandle SmokeTimerHandle;
	void EvaluateAndPlaySmoke();
	void StopBarrelSmoke();

	//casings pool array:
	UPROPERTY()
	TArray<AHordeShooterCasing*> CasingPool;
	int32 CurrentCasingIndex = 0;

};
