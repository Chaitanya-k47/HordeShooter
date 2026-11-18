// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MenuPlayerController.generated.h"

class UHordeMainMenuWidget;

UCLASS()
class HORDESHOOTER_API AMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

public:

	AMenuPlayerController();

	//pointer to created widget 
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	UHordeMainMenuWidget* MainMenuWidget = nullptr;

protected:

	virtual void BeginPlay() override;

	//class to use to create widget
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UHordeMainMenuWidget> MainMenuClass;
	
	//tag placed on the CineCameraActor in L_MainMenu
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	FName MenuCameraTag = FName("MenuCamera");

};
