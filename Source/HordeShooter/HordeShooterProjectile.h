// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HordeShooterProjectile.generated.h"

class USphereComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class UProjectileMovementComponent;
class UAudioComponent;
class USoundBase;

UCLASS()
class HORDESHOOTER_API AHordeShooterProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AHordeShooterProjectile();

	void ActivateProjectile(const FVector& StartLocation, const FVector& Direction, AActor* Shooter, FLinearColor PlasmaColor, float InDamage);
	void DeactivateProjectile();

	bool bIsActive = false;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UNiagaraComponent* CoreVFX;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UNiagaraComponent* TrailVFX;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UAudioComponent* FlightAudioComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UProjectileMovementComponent* ProjectileMovement;

	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	USoundBase* ProjectileHitSound;

	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	UNiagaraSystem* ProjectileHitVFX;

	UPROPERTY(EditDefaultsOnly, Category = "Projectile Stats")
	float Damage = 30.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Projectile Stats")
	float MaxLifespan = 5.0f; //failsafe it flies into void

	UPROPERTY(EditDefaultsOnly, Category = "Projectile Stats")
	float CollisionSphereRadius = 70.0f; // at 70.0f the niagara vfx parameter SizeMultiplier = 1.

	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

private:
	FTimerHandle FailsafeDeactivateTimer;
	FTimerHandle RibbonDecayTimer;

	void ReturnToPool();

};
