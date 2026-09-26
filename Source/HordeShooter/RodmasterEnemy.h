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

UENUM(BlueprintType)
enum class EExplosionType : uint8
{
	Slam, //rodmaster slam mechanic
	Overcharge //overcharge blast
};

UCLASS()
class HORDESHOOTER_API ARodmasterEnemy : public AHordeShooterEnemy
{
	GENERATED_BODY()

public:
	ARodmasterEnemy();

	virtual void Tick(float DeltaTime) override;

	//attack fn override for distance specific attacks
	virtual void PerformAttack() override;

	//override damage to handle bullet resistance and overcharge:
	virtual bool ReactToHit(float DamageAmount, const FVector& HitImpulse, FName HitBoneName, FName DamageSource = NAME_None) override;

	//oerride activation to reset overcharge meter when pooled
	virtual void ActivateEnemy(const FTransform& SpawnTransform, const TArray<float>& DifficultyMultipliers) override;

    UPROPERTY(BlueprintReadWrite)
    bool bIsSlamJumping = false; 

    virtual void Landed(const FHitResult& Hit) override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable)
	void ExecuteSlam(); //bind this in anim notify for slam montages

	UFUNCTION(BlueprintCallable) //bind this in anim notify for slam montages
	void ExecuteSlamJump();

	UFUNCTION(BlueprintCallable)//bind this in anim notify for slam montages
    void PauseSlamMontage();

	UFUNCTION(BlueprintCallable)
	void ExecuteOverchargeExplosion(); //bind this in anim notify of overcharge montages

	UFUNCTION(BlueprintCallable)//bind this in anim notify for slam montages
    void ExecutePlasmaShot();

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

	UPROPERTY(EditDefaultsOnly, Category = "Rodmaster|Overcharge")
	float MaxGlowIntensity = 50.0f;


	//OVERCHARGE EXPLOSION:
	UPROPERTY(EditDefaultsOnly, Category = "Rodmaster|Overcharge")
	float OverchargeExplosionRadius = 800.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Rodmaster|Overcharge")
	float OverchargeExplosionDamage = 200.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Rodmaster|Overcharge")
	float OverchargeExplosionImpulse = 400000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Rodmaster|Overcharge")
	UNiagaraSystem* OverchargeExplosionVFX;

	UPROPERTY(EditDefaultsOnly, Category = "Rodmaster|Overcharge")
	USoundBase* OverchargeExplosionSFX;

	UPROPERTY(EditDefaultsOnly, Category = "Rodmaster|Overcharge")
	TArray<UAnimMontage*> OverchargeExplosionMontages;


	//ATTACK:
	//long range attack damage is to be set on 'AttackDamage' variable on parent class

	//close quarter PowerMOve
	UPROPERTY(EditDefaultsOnly, Category = "Rodmaster|Combat|Stats")
	float CloseSlamRange = 600.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Rodmaster|Combat|Stats")
	float CloseSlamDamage = 50.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Rodmaster|Combat|Stats")
	float LaunchSpeed = 1500.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Rodmaster|Combat|Stats")
	float CloseSlamImpulse = 200000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Rodmaster|Combat|Effects")
	UNiagaraSystem* CloseSlamVFX;

	UPROPERTY(EditDefaultsOnly, Category = "Rodmaster|Combat|Effects")
	USoundBase* CloseSlamSFX;

	UPROPERTY(EditDefaultsOnly, Category = "Rodmaster|Combat|Effects")
	TSubclassOf<UCameraShakeBase> SlamCameraShake;

	UPROPERTY(EditDefaultsOnly, Category = "Rodmaster|Combat|Effects")
	float SlamShakeInnerRadius = 300.f; // Inside this radius, the shake is at 100% power

	UPROPERTY(EditDefaultsOnly, Category = "Rodmaster|Combat|Effects")
	float SlamShakeOuterRadius = 1500.f; // Shake fades to 0% at this distance


	UPROPERTY(EditDefaultsOnly, Category = "Rodmaster|Combat")
	TArray<UAnimMontage*> RangedPlasmaMontages;

	UPROPERTY(EditDefaultsOnly, Category = "Rodmaster|Combat")
	UAnimMontage* CloseSlamMontage;


private:
	float BaseWalkSpeed;
	float BaseAttackDamage; //long range
	float BaseCloseSlamDamage;

	bool bIsExploding = false;

	void TriggerExplosion(EExplosionType ExplosionType);
	void UpdateOverchargeVisuals(float OverchargeRatio);
	void StartOverchargeSequence();

	float OriginalMeshZ;
	float TargetMeshZ;
	bool bIsLandingRecovery = false;

	UPROPERTY()
	class UMaterialInstanceDynamic* DynamicGlowMat;
};
