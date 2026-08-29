// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "DamageableInterface.h"

#include "HordeShooterEnemy.generated.h"

class UAnimMontage;
class UAudioComponent;
class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAttackFinishedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEnemyKilledSignature);

UCLASS()
class HORDESHOOTER_API AHordeShooterEnemy : public ACharacter, public IDamageableInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AHordeShooterEnemy();


	//STATS:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Stats")
	float MaxHealth = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Stats")
	float CurrentHealth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|AI")
	float AttackRange = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|AI")
	float FleeRange = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|AI")
	float AttackDamage = 10.f;

	//AI behaviour flags:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|AI")
	bool bAlwaysFacePlayer = false; // True = uses 4-way strafing animations. False = uses 1-way forward animation

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|AI")
	bool bStopToAttack = false; // True = halts to play attack anim. False = plays attack anim while moving

	//Maps specific bone name to damage multipliers.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Stats")
	TMap<FName, float> BoneDamageMultipliers;
  

	//Action Montages (C++):
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Animation")
	TArray<UAnimMontage*> AttackMontages;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Animation")
	TArray<UAnimMontage*> HitReactionMontages;


	//Audio:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
	UAudioComponent* SprintAudioComp;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Audio")
	USoundBase* AttackSound;


	//STATE flags (read by AI controller):
	bool bIsAttacking = false;
	bool bIsStunned = false;
	bool bIsDead = false;


	//COMBAT ACTIONS:
	virtual void PerformMeleeAttack();
	void PlayHitReaction();

	UFUNCTION(BlueprintCallable)
	void ExecuteAttack(); //called by anim notify to apply damage to player


	//broadcast to AI controller when attack anim finishes.
	FOnAttackFinishedSignature OnAttackFinished;

	//broadcast to wave manager when enemy dies.
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnEnemyKilledSignature OnEnemyKilled;

	//POOLING SYSTEM:
	bool bIsActive = false; //used by the horde wave manager to track if the enemy is currently active

	void ActivateEnemy(const FTransform& SpawnTransform);
	void DeactivateEnemy();


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


	//DAMAGE INTERFACE:
	virtual bool ReactToHit(float DamageAmount, const FVector& HitImpulse, FName HitBoneName) override;
	virtual void Die();


	//Callback when c++ action montages finishes
	UFUNCTION()
	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted); 


	//HYBRID BP HOOKS for ENEMY VARIENTS:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat")
	void OnHit(float DamageAmount); 
	virtual void OnHit_Implementation(float DamageAmount); //default c++ implementation.

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat")
	void OnDeath(); 
	virtual void OnDeath_Implementation(); //default c++ implementation.


private:
	FVector LastHitImpulse;
	FName LastHitBoneName; 
	FTimerHandle DespawnTimerHandle;

};
