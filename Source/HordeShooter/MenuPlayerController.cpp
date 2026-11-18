// Fill out your copyright notice in the Description page of Project Settings.


#include "MenuPlayerController.h"
#include "HordeMainMenuWidget.h"
#include "Kismet/GameplayStatics.h"

AMenuPlayerController::AMenuPlayerController()
{
    //Prevents PlayerController from snapping ViewTarget back to (0,0,0) when DefaultPawnClass is nullptr
    bAutoManageActiveCameraTarget = false;

    bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void AMenuPlayerController::BeginPlay()
{
    Super::BeginPlay();

    //lock camera onto the CineCameraActor tagged as "MenuCamera" in L_MainMenu
    TArray<AActor*> FoundCameras;
    UGameplayStatics::GetAllActorsWithTag(GetWorld(), MenuCameraTag, FoundCameras);
    if(FoundCameras.Num() > 0 && FoundCameras[0])
    {
        SetViewTargetWithBlend(FoundCameras[0], 0.0f);
    }

    //spawn main menu widget
    if(MainMenuClass)
    {
        MainMenuWidget = CreateWidget<UHordeMainMenuWidget>(this, MainMenuClass);
        if(MainMenuWidget)
        {
            MainMenuWidget->AddToViewport();
        }
    }

    //Must be GameAndUI (NOT UIOnly) so 3D Actor OnClicked events on the pedestal enemy fire!
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	if (MainMenuWidget)
	{
		InputMode.SetWidgetToFocus(MainMenuWidget->TakeWidget());
	}
	SetInputMode(InputMode);
}