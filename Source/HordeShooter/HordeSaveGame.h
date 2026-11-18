// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "HordeSaveGame.generated.h"

/**
 * 
 */
UCLASS()
class HORDESHOOTER_API UHordeSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	
	UPROPERTY(VisibleAnywhere, Category = "Settings");
	float MouseSensitivity = 1.0f;

	UPROPERTY(VisibleAnywhere, Category = "UI");
	bool bHasViewedArsenal = false;
};
