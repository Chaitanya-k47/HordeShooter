// Fill out your copyright notice in the Description page of Project Settings.


#include "HordeSpawnPoint.h"
#include "Components/ArrowComponent.h"

// Sets default values
AHordeSpawnPoint::AHordeSpawnPoint()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	SpawnArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("SpawnArrow"));
	RootComponent = SpawnArrow;

	//visible only in editor, not in game
	SpawnArrow->bTreatAsASprite = true;

}