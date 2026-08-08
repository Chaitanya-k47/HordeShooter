// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EnemyAIController.generated.h"

/**
 * 
 */
 
class AHordeShooterEnemy;

// 1 byte scoped enum (AI states):
UENUM(BlueprintType)
enum class EAIState : uint8
{
	Idle,
	Chasing,
	Attacking
};

UCLASS()
class HORDESHOOTER_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	AEnemyAIController();

	void WakeAI();
	void SleepAI();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void OnPossess(APawn* InPawn) override;

private:
	UPROPERTY()
	AHordeShooterEnemy* ControlledEnemy;

	UPROPERTY()
	APawn* PlayerTarget;

	EAIState CurrentState = EAIState::Idle;

	//polling timer to find the player pawn on level load.
	FTimerHandle FindPlayerTimer;
	void FindPlayer();

	//AI controller tick timer. runs 10 times a sec instead of running every frame. (less expensive)
	FTimerHandle AITickTimer;
	void UpdateAILogic();

	bool CheckLineOfSight();

	UFUNCTION()
	void OnEnemyAttackFinished();

	void MoveToPlayer();
	FVector LastEdgeLocation = FVector::ZeroVector;

	FVector LastKnownPlayerLocation = FVector::ZeroVector;
	float PathUpdateThresholdSquared = 40000.f; //squared distance threshold for updating path to player (200 units)
	
};
