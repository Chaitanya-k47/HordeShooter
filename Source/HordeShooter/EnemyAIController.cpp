// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyAIController.h"

#include "Kismet/GameplayStatics.h"

AEnemyAIController::AEnemyAIController()
{
    
}

void AEnemyAIController::BeginPlay()
{
    Super::BeginPlay();

    // APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    // if(PlayerPawn)
    // {
    //     SetFocus(PlayerPawn);
    // }
}

void AEnemyAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    //wait a short time before trying to find the player, to ensure the player has spawned
    GetWorldTimerManager().SetTimer(FindPlayerTimer, this, &AEnemyAIController::FindPlayer, 0.1f, true);
}

void AEnemyAIController::FindPlayer()
{
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	
	if(PlayerPawn)
	{
		SetFocus(PlayerPawn);

        GetWorldTimerManager().ClearTimer(FindPlayerTimer);//player found stop the timer.
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("AI SUCCESS: Locked onto Player!"));
	}
	else
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("AI Waiting for player to spawn..."));
	}
}
