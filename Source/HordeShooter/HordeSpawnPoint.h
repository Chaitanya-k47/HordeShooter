// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HordeSpawnPoint.generated.h"

class UArrowComponent;

UCLASS()
class HORDESHOOTER_API AHordeSpawnPoint : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AHordeSpawnPoint();

protected:
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components");
	UArrowComponent* SpawnArrow;

};
