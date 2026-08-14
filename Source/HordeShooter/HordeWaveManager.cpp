// Fill out your copyright notice in the Description page of Project Settings.


#include "HordeWaveManager.h"
#include "HordeShooterEnemy.h"
#include "HordeSpawnPoint.h"
#include "Kismet/GameplayStatics.h"
#include "ArenaManager.h"

// Sets default values
AHordeWaveManager::AHordeWaveManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void AHordeWaveManager::BeginPlay()
{
	Super::BeginPlay();

	//find all the spawn points in the level.
	AActor* ArenaActor = UGameplayStatics::GetActorOfClass(GetWorld(), AArenaManager::StaticClass());
	CachedArenaManager = Cast<AArenaManager>(ArenaActor);
	
	if (!CachedArenaManager || !EnemyClassToSpawn)
	{
		return;
	}

	//debug:
	GetWorldTimerManager().SetTimer(DebugTelemetryTimer, this, &AHordeWaveManager::PrintPoolStats, 0.2f, true);

	//create the enemy pool.
	InitializePool();

	//Bind to Arena layout delegate:
	CachedArenaManager->OnArenaLayoutFinished.AddDynamic(this, &AHordeWaveManager::OnArenaReady);

}

void AHordeWaveManager::InitializePool()
{
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	//create the pool:
	for(int32 i = 0; i < PoolSize; i++)
	{
		AHordeShooterEnemy* NewEnemy = GetWorld()->SpawnActor<AHordeShooterEnemy>(EnemyClassToSpawn, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
		if(NewEnemy)
		{
			NewEnemy->OnEnemyKilled.AddDynamic(this, &AHordeWaveManager::OnEnemyDied); //bind to enemy death event
			EnemyPool.Add(NewEnemy);
		}
	}
}

void AHordeWaveManager::OnArenaReady()
{
	GetWorldTimerManager().SetTimer(WaveTransitionTimerHandle, this, &AHordeWaveManager::StartNextWave, IntermissionTime, false);
}

void AHordeWaveManager::StartNextWave()
{
	if (CurrentWaveIndex >= Waves.Num())
	{

		return;
	}

	//read wave data
	FWaveConfig CurrentWave = Waves[CurrentWaveIndex];
	EnemiesLeftToSpawnThisWave = CurrentWave.EnemiesToSpawn;

	//start spawn timer:
	GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &AHordeWaveManager::SpawnSingleEnemy, CurrentWave.SpawnDelay, true);
}

void AHordeWaveManager::SpawnSingleEnemy()
{
	//if done spawning all the enemies in a wave:
	if (EnemiesLeftToSpawnThisWave <= 0)
	{
		GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
		return;
	}

	//performance check: if too many enemies on the screen, skip this tick
	if (ActiveLivingEnemies >= MaxEnemiesOnScreen)
	{
		return; 
	}

	//find an inactive enemy in the pool:
	for(AHordeShooterEnemy* Enemy : EnemyPool)
	{
		if(Enemy && !Enemy->bIsActive)
		{
			//Ask arena manager for random spawn point.
			FTransform SpawnTransform = CachedArenaManager->GetRandomSpawnPoint();

			Enemy->ActivateEnemy(SpawnTransform);
			ActiveLivingEnemies++;
			EnemiesLeftToSpawnThisWave--;
			return;
		}
	}
}

void AHordeWaveManager::OnEnemyDied()
{
	ActiveLivingEnemies--;

	//if all the enemies in the wave have been spawned and all active nemies are dead i.e. wave is cleared.
	if (ActiveLivingEnemies <= 0 && EnemiesLeftToSpawnThisWave <= 0)
	{
		CurrentWaveIndex++;

		FTimerHandle ArenaShiftTimer;
		GetWorldTimerManager().SetTimer(ArenaShiftTimer, [this]()
		{
			if(CachedArenaManager) CachedArenaManager->BeginNewLayoutGeneration();
		}, 2.0f, false);
	}

	//Wave replenishment
	else if(EnemiesLeftToSpawnThisWave > 0 && ActiveLivingEnemies < MaxEnemiesOnScreen)
	{
		float RandomReplenishDelay = FMath::RandRange(0.2f, 0.8f);
		FTimerHandle ReplenishTimer;
		GetWorldTimerManager().SetTimer(ReplenishTimer, this, &AHordeWaveManager::SpawnSingleEnemy, RandomReplenishDelay, false);
	}	
}


void AHordeWaveManager::PrintPoolStats()
{
	if (!GEngine) return;

	int32 AliveCount = 0;
	int32 DeadRagdollCount = 0;
	int32 InactivePoolCount = 0;

	for (AHordeShooterEnemy* Enemy : EnemyPool)
	{
		if (!Enemy) continue;

		if (Enemy->bIsActive && !Enemy->bIsDead)
		{
			AliveCount++;
		}
		else if (Enemy->bIsActive && Enemy->bIsDead)
		{
			DeadRagdollCount++;
		}
		else if (!Enemy->bIsActive)
		{
			InactivePoolCount++;
		}
	}

	// Use Keys 1-5 so the messages overwrite themselves cleanly instead of spamming down the screen
	GEngine->AddOnScreenDebugMessage(1, 0.3f, FColor::Green,  FString::Printf(TEXT("[POOL] Alive & Active: %d"), AliveCount));
	GEngine->AddOnScreenDebugMessage(2, 0.3f, FColor::Red,    FString::Printf(TEXT("[POOL] Dead & Ragdolling: %d"), DeadRagdollCount));
	GEngine->AddOnScreenDebugMessage(3, 0.3f, FColor::Cyan,   FString::Printf(TEXT("[POOL] Inactive (Ready): %d"), InactivePoolCount));
	GEngine->AddOnScreenDebugMessage(4, 0.3f, FColor::Yellow, FString::Printf(TEXT("[WAVE] Left to Spawn: %d"), EnemiesLeftToSpawnThisWave));
	GEngine->AddOnScreenDebugMessage(5, 0.3f, FColor::Orange, FString::Printf(TEXT("[WAVE] Max Screen Budget: %d"), MaxEnemiesOnScreen));
}



