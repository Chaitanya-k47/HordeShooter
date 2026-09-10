// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HordeShooterPickup.generated.h"

class AHordeShooterCharacter;
class USphereComponent;
class UNiagaraComponent;
class USoundBase;

UENUM(BlueprintType)
enum class EPickupType : uint8
{
	Ammo, //Red
	Health, //Green
	Surge //Blue
};

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

	void ActivatePickup(const FVector& SpawnLocation, EPickupType InType, EPickupSize InSize);
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
	FLinearColor RedEnergyColour = FLinearColor(1.0f, 0.0f, 0.0f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Pickup Config")
	FLinearColor GreenEnergyColour = FLinearColor(0.0f, 1.0f, 0.0f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Pickup Config")
	FLinearColor BlueEnergyColour = FLinearColor(0.0f, 0.0f, 1.0f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Pickup Config")
	USoundBase* AmmoPickupSound;

	UPROPERTY(EditDefaultsOnly, Category = "Pickup Config")
	USoundBase* HealthPickupSound;
	
	UPROPERTY(EditDefaultsOnly, Category = "Pickup Config")
	USoundBase* SurgePickupSound;

	UPROPERTY(EditDefaultsOnly, Category = "Pickup Config")
	float VacuumSpeed = 4000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Pickup Config")
	float VacuumSphereRadius = 400.f;

	UFUNCTION()
	void OnVacuumOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	//bp handles colour according to type	
	void OnPickupActivated(EPickupType Type, EPickupSize Size);

private:
	EPickupType CurrentType;
	EPickupSize CurrentSize;

	//reward logic:
	void GrantReward();

	bool bIsHoming = false;

	UPROPERTY()
	AHordeShooterCharacter* TargetPlayer = nullptr;

	FTimerHandle FailsafeDeactivateTimer;
};
