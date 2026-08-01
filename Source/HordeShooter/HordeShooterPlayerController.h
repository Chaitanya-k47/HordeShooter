// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "HordeShooterPlayerController.generated.h"

class UHordeShooterHUDWidget;

UCLASS()
class HORDESHOOTER_API AHordeShooterPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


protected:
	//UI CONFIG:
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UHordeShooterHUDWidget> PlayerHUDClass;

public:
	UPROPERTY()
	UHordeShooterHUDWidget* PlayerHUDWidget; //actual pointer to store the created widget instance

	void ShowGameOverScreen();
	
};
