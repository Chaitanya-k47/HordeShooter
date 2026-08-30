// Fill out your copyright notice in the Description page of Project Settings.


#include "BlitzkriegerEnemy.h"
#include "ArenaManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

ABlitzkriegerEnemy::ABlitzkriegerEnemy()
{
	//set custom stats
	MaxHealth = 100.0f;
	AttackRange = 10000.0f;
	FleeRange = 1500.0f;

    bAlwaysFacePlayer = true; //use the 4-way strafing blendSpace
	bStopToAttack = true;
	bStrafeDuringCooldown = true;

	AttackCooldown = 4.0f;
	
	WalkSpeed = 600.0f;
	SprintSpeed = 1200.0f;

	GetCharacterMovement()->RotationRate = FRotator(0.0f, 200.0f, 0.0f);
}

void ABlitzkriegerEnemy::PerformMeleeAttack()
{
    Super::PerformMeleeAttack();
}

void ABlitzkriegerEnemy::CastLightningOnPlayer()
{
    if(bIsDead || bIsStunned) return;

    AActor* ArenaActor = UGameplayStatics::GetActorOfClass(GetWorld(), AArenaManager::StaticClass());

    if(ArenaActor)
	{
		AArenaManager* Arena = Cast<AArenaManager>(ArenaActor);
		Arena->StartLightningWave(NumberOfLightningsInAWave);
	}
}
