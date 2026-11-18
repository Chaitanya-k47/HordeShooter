// Fill out your copyright notice in the Description page of Project Settings.


#include "MenuGameMode.h"
#include "MenuPlayerController.h"

AMenuGameMode::AMenuGameMode()
{
    //No playable pawn spawned in L_MainMenu; camera stays fixed on the CineCameraActor
    DefaultPawnClass = nullptr;
    PlayerControllerClass = AMenuPlayerController::StaticClass();
}
