// Fill out your copyright notice in the Description page of Project Settings.


#include "HordeShooterPlayerController.h"
#include "HordeShooterHUDWidget.h"
#include "Kismet/GameplayStatics.h"


void AHordeShooterPlayerController::BeginPlay()
{
    Super::BeginPlay();
    
    bShowMouseCursor = false;
    FInputModeGameOnly InputMode;
    SetInputMode(InputMode);

    if(PlayerHUDClass)
    {
        PlayerHUDWidget = CreateWidget<UHordeShooterHUDWidget>(this, PlayerHUDClass);
        
        if(PlayerHUDWidget)
        {
            PlayerHUDWidget->AddToViewport();
        }
    }
}

void AHordeShooterPlayerController::ShowGameOverScreen()
{
    if(PlayerHUDWidget)
    {
        PlayerHUDWidget->ShowGameOver();
    }

    //pause the game so enemies stop attacking
	UGameplayStatics::SetGamePaused(GetWorld(), true);

    //show cursor and give input to UI
    bShowMouseCursor = true;
	FInputModeUIOnly InputMode;
    SetInputMode(InputMode);
}

void AHordeShooterPlayerController::SetSlideFX(bool bIsSliding)
{
	if (PlayerHUDWidget)
	{
		PlayerHUDWidget->ToggleSlideFX(bIsSliding);
	}
}
