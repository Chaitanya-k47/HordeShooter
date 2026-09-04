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
	
	if(!CachedArenaManager || PoolConfiguration.Num() == 0) return;

	//create the enemy pools.
	InitializePools();

	//create the ammo pickup pool:
	if(AmmoPickupClass)
	{
		FActorSpawnParameters SpawnParams;
		for(int32 i = 0; i < PickupPoolSize; i++)
		{
			AHordeShooterPickup* NewPickup = GetWorld()->SpawnActor<AHordeShooterPickup>(AmmoPickupClass, FVector(0,0,-10000), FRotator::ZeroRotator, SpawnParams);
			if (NewPickup)
			{
				AmmoPickupPool.Add(NewPickup);
			}
		}
	}

	//Bind to Arena layout delegate:
	CachedArenaManager->OnArenaLayoutFinished.AddDynamic(this, &AHordeWaveManager::OnArenaReady);

	//debug:
	GetWorldTimerManager().SetTimer(DebugTelemetryTimer, this, &AHordeWaveManager::PrintPoolStats, 0.2f, true);
}

void AHordeWaveManager::InitializePools()
{
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	//loop through enemy class to initialize their pools
	for(const auto& PoolKVP : PoolConfiguration)
	{
		TSubclassOf<AHordeShooterEnemy> EnemyClass = PoolKVP.Key;
		int32 AmountToSpawn = PoolKVP.Value;

		if(!EnemyClass) return;

		FEnemyPoolArray NewPool;
		for(int32 i = 0; i < AmountToSpawn; i++)
		{
			AHordeShooterEnemy* NewEnemy = GetWorld()->SpawnActor<AHordeShooterEnemy>(EnemyClass, FVector(0,0,-10000), FRotator::ZeroRotator, SpawnParams);
			if(NewEnemy)
			{
				NewEnemy->OnEnemyKilled.AddDynamic(this, &AHordeWaveManager::OnEnemyDied); //bind to enemy death event
				NewPool.Pool.Add(NewEnemy);
			}
		}

		//add this pool to "pool dictionary"
		EnemyPools.Add(EnemyClass, NewPool);
	}

}

void AHordeWaveManager::OnArenaReady()
{
	GetWorldTimerManager().SetTimer(WaveTransitionTimerHandle, this, &AHordeWaveManager::StartNextWave, IntermissionTime, false);
}

void AHordeWaveManager::StartNextWave()
{
	if(CurrentWaveIndex >= Waves.Num())
	{
		return;
	}

	//read wave data
	FWaveConfig CurrentWave = Waves[CurrentWaveIndex];
	ShuffledSpawnStack.Empty();

	//add all the requested enemies to spawn queue
	for(const auto& EnemyKVP : CurrentWave.EnemyCounts)
	{
		TSubclassOf<AHordeShooterEnemy> EnemyClass = EnemyKVP.Key;
		int32 Count = EnemyKVP.Value;

		for(int32 i = 0; i < Count; i++)
		{
			ShuffledSpawnStack.Add(EnemyClass);
		}
	}

	//shuffle the spawn queue
	for(int32 i = ShuffledSpawnStack.Num() - 1; i > 0; i--)
	{
		int32 RandomIndex = FMath::RandRange(0, i);
		ShuffledSpawnStack.Swap(i, RandomIndex);
	}

	//start spawn timer:
	GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &AHordeWaveManager::SpawnSingleEnemy, CurrentWave.SpawnDelay, true);
	
	if(CachedArenaManager) CachedArenaManager->SetCombatActive(true);
}

void AHordeWaveManager::SpawnSingleEnemy()
{
	//if done spawning all the enemies in a wave:
	if(ShuffledSpawnStack.Num() <= 0)
	{
		GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
		return;
	}

	//performance check: if too many enemies on the screen, skip this tick
	if (ActiveLivingEnemies >= MaxEnemiesOnScreen)
	{
		return; 
	}

	//peek at the next class needed to spawn
	TSubclassOf<AHordeShooterEnemy> ClassToSpawn = ShuffledSpawnStack.Last();

	//find an inactive enemy in the pool of this class:
	if(EnemyPools.Contains(ClassToSpawn))
	{
		for(AHordeShooterEnemy* Enemy : EnemyPools[ClassToSpawn].Pool)
		{
			if(Enemy && !Enemy->bIsActive && CachedArenaManager)
			{
				ShuffledSpawnStack.Pop();

				//Ask arena manager for random spawn point.
				FTransform SpawnTransform = CachedArenaManager->GetRandomSpawnPoint();
				Enemy->ActivateEnemy(SpawnTransform);
				ActiveLivingEnemies++;
				return;
			}
		}
	}
}

void AHordeWaveManager::OnEnemyDied()
{
	ActiveLivingEnemies--;

	//if all the enemies in the wave have been spawned and all active nemies are dead i.e. wave is cleared.
	if (ActiveLivingEnemies <= 0 && ShuffledSpawnStack.Num() <= 0)
	{
		CurrentWaveIndex++;

		if(CachedArenaManager) CachedArenaManager->SetCombatActive(false);

		FTimerHandle ArenaShiftTimer;
		GetWorldTimerManager().SetTimer(ArenaShiftTimer, CachedArenaManager, &AArenaManager::BeginNewLayoutGeneration, 2.0f, false);
	}

	//Wave replenishment
	else if(ShuffledSpawnStack.Num() > 0 && ActiveLivingEnemies < MaxEnemiesOnScreen)
	{
		float RandomReplenishDelay = FMath::RandRange(0.2f, 0.8f);
		FTimerHandle ReplenishTimer;
		GetWorldTimerManager().SetTimer(ReplenishTimer, this, &AHordeWaveManager::SpawnSingleEnemy, RandomReplenishDelay, false);
	}	
}

void AHordeWaveManager::SpawnAmmoDrop(const FVector& Location, EPickupSize Size)
{
	//find first inactive pickup
	for (AHordeShooterPickup* Pickup : AmmoPickupPool)
	{
		if (Pickup && !Pickup->bIsActive)
		{
			Pickup->ActivatePickup(Location, Size);
			return;
		}
	}
	
	// (Optional) If pool is exhausted, steal the oldest active one, or just ignore. 
	// 50 pool size should be enough for an arena.
}

void AHordeWaveManager::PrintPoolStats()
{
	if (!GEngine) return;

	int32 AliveCount = 0;
	int32 DeadRagdollCount = 0;
	int32 InactivePoolCount = 0;

	// 1. Loop through every class pool in our dictionary
	for (const auto& PoolKVP : EnemyPools)
	{
		// 2. Loop through the actual zombies inside that specific pool
		for (AHordeShooterEnemy* Enemy : PoolKVP.Value.Pool)
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
	}

	GEngine->AddOnScreenDebugMessage(1, 0.3f, FColor::Green,  FString::Printf(TEXT("[POOL] Alive & Active: %d"), AliveCount));
	GEngine->AddOnScreenDebugMessage(2, 0.3f, FColor::Red,    FString::Printf(TEXT("[POOL] Dead & Ragdolling: %d"), DeadRagdollCount));
	GEngine->AddOnScreenDebugMessage(3, 0.3f, FColor::Cyan,   FString::Printf(TEXT("[POOL] Inactive (Ready): %d"), InactivePoolCount));
	
	// Print the remaining elements in our Stack!
	GEngine->AddOnScreenDebugMessage(4, 0.3f, FColor::Yellow, FString::Printf(TEXT("[WAVE] Left to Spawn: %d"), ShuffledSpawnStack.Num()));
	GEngine->AddOnScreenDebugMessage(5, 0.3f, FColor::Orange, FString::Printf(TEXT("[WAVE] Max Screen Budget: %d"), MaxEnemiesOnScreen));
}


