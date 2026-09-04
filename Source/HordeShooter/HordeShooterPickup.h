// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HordeShooterPickup.generated.h"

class AHordeShooterCharacter;
class USphereComponent;
class UNiagaraComponent;

UENUM(BlueprintType)
enum class EPickupSize : uint8
{
	Small, //25%
	Medium, //50%
	Large //75%
};

UCLASS()
class HORDESHOOTER_API AHordeShooterPickup : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AHordeShooterPickup();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void ActivatePickup(const FVector& SpawnLocation, EPickupSize InSize);
	void DeactivatePickup();

	bool bIsActive = false;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	USphereComponent* VacuumSphere;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UNiagaraComponent* PickupVFX;

	UPROPERTY(EditDefaultsOnly, Category = "Pickup Config")
	float VacuumSpeed = 4000.f;

	UFUNCTION()
	void OnVacuumOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	EPickupSize CurrentSize;
	bool bIsHoming = false;

	UPROPERTY()
	AHordeShooterCharacter* TargetPlayer = nullptr;

	FTimerHandle FailsafeDeactivateTimer;
};
