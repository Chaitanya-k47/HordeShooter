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
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("ERROR: Missing Arena Manager or Enemy Class!"));
		return;
	}

	//create the enemy pool.
	InitializePool();

	//start the timer to spawn the first wave
	GetWorldTimerManager().SetTimer(WaveTransitionTimerHandle, this, &AHordeWaveManager::StartNextWave, IntermissionTime, false);
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

void AHordeWaveManager::StartNextWave()
{
	if (CurrentWaveIndex >= Waves.Num())
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, TEXT("ALL WAVES CLEARED! YOU SURVIVED!"));
		return;
	}

	//read wave data
	FWaveConfig CurrentWave = Waves[CurrentWaveIndex];
	EnemiesLeftToSpawnThisWave = CurrentWave.EnemiesToSpawn;

	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, FString::Printf(TEXT("WAVE %d STARTING!"), CurrentWaveIndex + 1));

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
		
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("WAVE CLEARED! Get ready..."));

		GetWorldTimerManager().SetTimer(WaveTransitionTimerHandle, this, &AHordeWaveManager::StartNextWave, IntermissionTime, false);
	}
}



