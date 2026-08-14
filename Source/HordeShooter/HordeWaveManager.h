// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HordeWaveManager.generated.h"

class AHordeShooterEnemy;
class AArenaManager;

USTRUCT(BlueprintType)
struct FWaveConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 EnemiesToSpawn = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SpawnDelay = 1.f; //time in between spawing a single enemy. 

};

UCLASS()
class HORDESHOOTER_API AHordeWaveManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AHordeWaveManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	//HORDE SETTINGS:
	UPROPERTY(EditAnywhere, Category = "Horde Setup")
	TSubclassOf<AHordeShooterEnemy> EnemyClassToSpawn;

	UPROPERTY(EditAnywhere, Category = "Horde Setup")
	int32 PoolSize = 30; //total instances in the pool.

	UPROPERTY(EditAnywhere, Category = "Horde Setup")
	int32 MaxEnemiesOnScreen = 15; //performance cap.

	UPROPERTY(EditAnywhere, Category = "Horde Setup")
	TArray<FWaveConfig> Waves; //array of waves/wave configs.

	UPROPERTY(EditAnywhere, Category = "Horde Setup")
	float IntermissionTime = 5.f;


private:
	//POOLING AND TRACKING:
	UPROPERTY()
	TArray<AHordeShooterEnemy*> EnemyPool;
	
	UPROPERTY()
	AArenaManager* CachedArenaManager;

	int32 CurrentWaveIndex = 0;
	int32 EnemiesLeftToSpawnThisWave = 0;
	int32 ActiveLivingEnemies = 0;

	//TIMERS AND LOGIC:
	FTimerHandle SpawnTimerHandle;
	FTimerHandle WaveTransitionTimerHandle;

	void InitializePool();

	UFUNCTION()
	void OnArenaReady();

	void StartNextWave();
	
	void SpawnSingleEnemy();

	//callback for enemy death. to be bound to a delegate in enemy class.
	UFUNCTION()
	void OnEnemyDied();

	// --- POOL DEBUGGER ---
	FTimerHandle DebugTelemetryTimer;
	void PrintPoolStats();
};
