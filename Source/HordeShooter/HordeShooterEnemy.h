// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "DamageableInterface.h"

#include "HordeShooterEnemy.generated.h"

class UAnimMontage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAttackFinishedSignature);

UCLASS()
class HORDESHOOTER_API AHordeShooterEnemy : public ACharacter, public IDamageableInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AHordeShooterEnemy();

	// Called every frame
	// virtual void Tick(float DeltaTime) override;


	//STATS:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Stats")
	float MaxHealth = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Stats")
	float CurrentHealth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|AI")
	float AttackRange = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|AI")
	float AttackDamage = 10.f;
  

	//Action Montages (C++):
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Animation")
	TArray<UAnimMontage*> AttackMontages;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Animation")
	TArray<UAnimMontage*> HitReactionMontages;


	//STATE flags (read by AI controller):
	bool bIsAttacking = false;
	bool bIsStunned = false;
	bool bIsDead = false;


	//COMBAT ACTIONS:
	void PerformMeleeAttack();
	void PlayHitReaction();


	//broadcast to AI controller when attack anim finishes.
	FOnAttackFinishedSignature OnAttackFinished;


	//POOLING SYSTEM:
	// Called to revive the enemy without spawning a new one
	virtual void ResetEnemy();


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


	//DAMAGE INTERFACE:
	virtual void ReactToHit(float DamageAmount, const FVector& HitImpulse, FName HitBoneName) override;
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

};
