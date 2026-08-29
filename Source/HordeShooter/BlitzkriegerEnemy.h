// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HordeShooterEnemy.h"
#include "BlitzkriegerEnemy.generated.h"

/**
 * 
 */
UCLASS()
class HORDESHOOTER_API ABlitzkriegerEnemy : public AHordeShooterEnemy
{
	GENERATED_BODY()
	
public:

	ABlitzkriegerEnemy();

	//we ovverride the attack funtion from base class
	//name is misleading in case of this enemy type as it CASTS a Spell rather than melee-attack
	virtual void PerformMeleeAttack() override;

	//called by anim notify inside the attack montages:
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void CastLightningOnPlayer();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Stats")
	int32 NumberOfLightningsInAWave = 2;
};
