// Fill out your copyright notice in the Description page of Project Settings.


#include "BlitzkriegerEnemy.h"
#include "ArenaManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"

ABlitzkriegerEnemy::ABlitzkriegerEnemy()
{
	//set custom stats
	MaxHealth = 300.0f;
	AttackRange = 3000.0f;
	FleeRange = 1200.0f;

    bAlwaysFacePlayer = true; //use the 4-way strafing blendSpace
	bStopToAttack = true;
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
