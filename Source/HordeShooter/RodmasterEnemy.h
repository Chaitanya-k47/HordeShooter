// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HordeShooterEnemy.h"

#include "RodmasterEnemy.generated.h"

/**
 * 
 */

class UNiagaraSystem;
class USoundBase;

UCLASS()
class HORDESHOOTER_API ARodmasterEnemy : public AHordeShooterEnemy
{
	GENERATED_BODY()

public:
	ARodmasterEnemy();

	//attack fn override for distance specific attacks
	virtual void PerformAttack() override;

	//override damage to handle bullet resistance and overcharge:
	virtual bool ReactToHit(float DamageAmount, const FVector& HitImpulse, FName HitBoneName, FName DamageSource = NAME_None) override;

	//oerride activation to reset overcharge meter when pooled
	virtual void ActivateEnemy(const FTransform& SpawnTransform, const TArray<float>& DifficultyMultipliers) override;

protected:
	virtual void BeginPlay() override;

	//BULLET RESISTANCE:
	UPROPERTY(EditDefaultsOnly, Category = "Rodmaster|Stats")
	float BulletDamageMultiplier = 0.25f; //for value x, takes ((1-x)*100)% less damage. Keep between 0 to 1.


	//OVERCHARGE MECHANIC:
	UPROPERTY(EditDefaultsOnly, Category = "Rodmaster|Overcharge")
	float MaxOvercharge = 600.0f; //how much energy it needs to explode(Keep this a little less than its Max Health)

	UPROPERTY(VisibleAnywhere, Category = "Rodmaster|Overcharge")
	float CurrentOvercharge = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Rodmaster|Overcharge")
	float MaxSpeedMultiplier = 2.0f; //2x faster at 99% charge

	UPROPERTY(EditDefaultsOnly, Category = "Rodmaster|Overcharge")
	float MaxDamageMultiplier = 2.0f; //hits 2x harder at 99% charge


	//OVERCHARGE EXPLOSION:
	UPROPERTY(EditDefaultsOnly, Category = "Rodmaster|Overcharge")
	float ExplosionRadius = 800.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Rodmaster|Overcharge")
	float ExplosionDamage = 200.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Rodmaster|Overcharge")
	float ExplosionImpulse = 400000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Rodmaster|Overcharge")
	UNiagaraSystem* ExplosionVFX;

	UPROPERTY(EditDefaultsOnly, Category = "Rodmaster|Overcharge")
	USoundBase* ExplosionSFX;


	//ATTACK:
	//long range attack damage is to be set on 'AttackDamage' variable on parent class

	UPROPERTY(EditDefaultsOnly, Category = "Rodmaster|Combat|Stats")
	float CloseSlamRange = 600.0f;

	//close quarter melee attack
	UPROPERTY(EditDefaultsOnly, Category = "Rodmaster|Combat|Stats")
	float CloseMeleeDamage = 10.0f; 

	//close quarter PowerMOve
	UPROPERTY(EditDefaultsOnly, Category = "Rodmaster|Combat|Stats")
	float CloseSlamDamage = 50.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Rodmaster|Combat")
	TArray<UAnimMontage*> RangedPlasmaMontages;

	UPROPERTY(EditDefaultsOnly, Category = "Rodmaster|Combat")
	TArray<UAnimMontage*> CloseSlamMontages;


private:
	float BaseWalkSpeed;
	float BaseAttackDamage; //long range
	float BaseCloseMeleeDamage;
	float BaseCloseSlamDamage;

	bool bIsExploding = false;

	void TriggerOverchargeExplosion();
	void UpdateOverchargeVisuals(float OverchargeRatio);

};
