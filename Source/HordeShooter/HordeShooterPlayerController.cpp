// Fill out your copyright notice in the Description page of Project Settings.


#include "HordeShooterPlayerController.h"
#include "HordeShooterHUDWidget.h"


void AHordeShooterPlayerController::BeginPlay()
{
    Super::BeginPlay();
    
    if(PlayerHUDClass)
    {
        PlayerHUDWidget = CreateWidget<UHordeShooterHUDWidget>(this, PlayerHUDClass);
        
        if(PlayerHUDWidget)
        {
            PlayerHUDWidget->AddToViewport();
        }
    }
}
