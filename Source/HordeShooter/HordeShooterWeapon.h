// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HordeShooterWeapon.generated.h"

class USceneComponent;
class USkeletalMeshComponent;
class AHordeShooterCharacter;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
	USkeletalMeshComponent* Mesh;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "State")
	AHordeShooterCharacter* CurrentOwner;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "State")
	bool bIsEquipped = false;

	
};
