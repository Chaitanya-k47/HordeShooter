// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "DamageableInterface.h"

#include "HordeShooterEnemy.generated.h"

UCLASS()
class HORDESHOOTER_API AHordeShooterEnemy : public ACharacter, public IDamageableInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AHordeShooterEnemy();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	//STATS:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float MaxHealth = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	float CurrentHealth;

	//DAMAGE INTERFACE:
	virtual void ReactToHit(float DamageAmount, const FVector& HitDirection) override;
	virtual void Die();

	//HYBRID BP HOOKS for ENEMY VARIENTS:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat")
	void OnHit(float DamageAmount); 
	virtual void OnHit_Implementation(float DamageAmount); //default c++ implementation.

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat")
	void OnDeath(); 
	virtual void OnDeath_Implementation(); //default c++ implementation.

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	//POOLING SYSTEM:
	// Called to revive the enemy without spawning a new one
	virtual void ResetEnemy();


private:
	bool bIsDead = false;
	FVector LastHitDirection;
};
