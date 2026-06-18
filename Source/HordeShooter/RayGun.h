// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HordeShooterWeapon.h"
#include "RayGun.generated.h"

class UNiagaraComponent;
class UAudioComponent;
class UMaterialInstanceDynamic;

/**
 * 
 */
UCLASS()
class HORDESHOOTER_API ARayGun : public AHordeShooterWeapon
{
	GENERATED_BODY()


public:
	ARayGun();

	virtual void Tick(float DeltaTime) override;

	//override base classes:
	virtual void StartFire() override;
	virtual void StopFire() override;
	virtual void StartAltFire() override;
	virtual void StopAltFire() override;
	virtual void OnHolstered() override;


protected:

	virtual void BeginPlay() override;

	//RAYGUN FX:
	//primary fire(continuous beam):
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
	UNiagaraComponent* BeamComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
	UNiagaraComponent* BeamImpactComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
	UNiagaraComponent* BeamMuzzleGlowComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
	UAudioComponent* BeamAudioComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
	UAudioComponent* BeamImpactAudioComp;
	
	UPROPERTY(EditDefaultsOnly, Category = "Effects|Beam")
	UMaterialInterface* ScorchDecalMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "Effects|Beam")
	float ScorchDecalSize = 12.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Effects|Beam")
	float ScorchSpawnDistance = 12.0f; //how far the laser must drag to spawn a new mark

	UPROPERTY(EditDefaultsOnly, Category = "Effects|Beam")
	float ScorchLifespan = 5.0f;

	//alt fire (charged AOE blast):
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
	UNiagaraComponent* ChargeOrbComp;

	UPROPERTY(EditDefaultsOnly, Category = "Effects|Alt Fire")
	UNiagaraSystem* AoEBlastEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Effects|Alt Fire")
	UNiagaraSystem* AltFireBeamSystem;

	UPROPERTY(EditDefaultsOnly, Category = "Effects|Alt Fire")
	UNiagaraSystem* AltFireMuzzleFlash;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
	UAudioComponent* ChargeAudioComp;

	UPROPERTY(EditDefaultsOnly, Category = "Effects|Alt Fire")
	USoundBase* AltFireDischargeSound;

	UPROPERTY(EditDefaultsOnly, Category = "Effects|Alt Fire")
	USoundBase* AltFireBlastSound;

	
	//RAYGUN STATS:
	//primary fire(continuous beam):
	UPROPERTY(EditDefaultsOnly, Category = "Weapon Stats|Beam")
	float BeamDamageRate = 0.1f; //damage applied every 0.1s while beam is active(10 times a sec.)

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Stats|Beam")
	float BeamDamagePerBeamTick = 15.f; //damage applied every time BeamDamageRate ticks(150 DPS)

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Stats|Beam")
	int32 BeamAmmoCostPerBeamTick = 1;

	//alt fire (charged AOE blast):
	UPROPERTY(EditDefaultsOnly, Category = "Weapon Stats|Alt Fire")
	float ChargeTimeRequired = 1.5f; //hold 1.5 sec before shooting AltFire.
	
	UPROPERTY(EditDefaultsOnly, Category = "Weapon Stats|Alt Fire")
	float AltFireDamage = 150.f;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Stats|Alt Fire")
	float AltFireRadius = 400.f;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Stats|Alt Fire")
	int32 AltFireAmmoCost = 5;


	//RAYGUN ANIMATIONS:
	UPROPERTY(EditDefaultsOnly, Category = "Animations|Beam")
	UAnimMontage* ArmsBeamLoopMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Animations|Alt Fire")
	UAnimMontage* ArmsChargeLoopMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Animations|Alt Fire")
	UAnimMontage* ArmsDischargeMontage;


private:
	//State Trackers:
	bool bIsFiringBeam = false;
	bool bIsCharging = false;

	FTimerHandle BeamDamageTimerHandle;

	UPROPERTY()
	AActor* CurrentBeamTarget = nullptr; //the actor currently being hit by the beam

	void PerformBeamTick();
	
	float CurrentChargeTime = 0.f;
	void PerformAltFire();

	FVector LastScorchLocation = FVector::ZeroVector;

	UPROPERTY()
	UMaterialInstanceDynamic* DynamicWeaponMat;

	float CurrentGlow = 0.0f;

	//alt fire cooldown
	bool bCanAltFire = true;
	FTimerHandle AltFireCooldownTimerHandle;
	void ResetAltFireCooldown();
};
