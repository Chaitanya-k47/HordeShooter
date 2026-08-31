// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HordeWaveManager.generated.h"

class AHordeShooterEnemy;
class AArenaManager;

USTRUCT()
struct FEnemyPoolArray
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<AHordeShooterEnemy*> Pool;
};

USTRUCT(BlueprintType)
struct FWaveConfig
{
	GENERATED_BODY()

	//TMap<EnemyClass, Count>
	//ex. BP_Zombie: 10, BP_BlitzKrieger: 2
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<TSubclassOf<AHordeShooterEnemy>, int32> EnemyCounts;

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
	UPROPERTY(EditAnywhere, Category = "Horde Setup|Pool")
	TMap<TSubclassOf<AHordeShooterEnemy>, int32> PoolConfiguration;

	UPROPERTY(EditAnywhere, Category = "Horde Setup")
	int32 MaxEnemiesOnScreen = 15; //performance cap.

	UPROPERTY(EditAnywhere, Category = "Horde Setup")
	TArray<FWaveConfig> Waves; //array of waves/wave configs.

	UPROPERTY(EditAnywhere, Category = "Horde Setup")
	float IntermissionTime = 5.f;


private:
	//POOLING AND TRACKING
	UPROPERTY()
	AArenaManager* CachedArenaManager;

	//TMap<EnemyClass, Pool> i.e. an enemy class mapped to its individual pool. hence we have multiple pools for multiple enemy classes.
	UPROPERTY()
	TMap<TSubclassOf<AHordeShooterEnemy>, FEnemyPoolArray> EnemyPools;

	//shuffle bag stack for current wave spawning
	TArray<TSubclassOf<AHordeShooterEnemy>> ShuffledSpawnStack;

	int32 CurrentWaveIndex = 0;
	int32 ActiveLivingEnemies = 0;

	//TIMERS AND LOGIC:
	FTimerHandle SpawnTimerHandle;
	FTimerHandle WaveTransitionTimerHandle;

	void InitializePools();
	void StartNextWave();
	void SpawnSingleEnemy();

	UFUNCTION()
	void OnArenaReady();

	//callback for enemy death. to be bound to a delegate in enemy class.
	UFUNCTION()
	void OnEnemyDied();

	// --- POOL DEBUGGER ---
	FTimerHandle DebugTelemetryTimer;
	void PrintPoolStats();
};
