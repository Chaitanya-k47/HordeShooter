// Fill out your copyright notice in the Description page of Project Settings.


#include "HordeShooterEnemy.h"

// Sets default values
AHordeShooterEnemy::AHordeShooterEnemy()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AHordeShooterEnemy::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AHordeShooterEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AHordeShooterEnemy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

