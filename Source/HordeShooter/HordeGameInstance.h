// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "HordeGameInstance.generated.h"

class UHordeSaveGame;
class UInputMappingContext;

/**
 * 
 */
UCLASS()
class HORDESHOOTER_API UHordeGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:

	virtual void Init() override;

	UFUNCTION(BlueprintCallable, Category = "Settings")
	float GetMouseSensitivity() const;

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void SetMouseSensitivity(float NewSensitivity);

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void SaveGameData();

	UFUNCTION(BlueprintCallable, Category = "UI")
	bool HasViewedArsenal() const;

	UFUNCTION(BlueprintCallable, Category = "UI")
	void MarkArsenalViewed();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Levels")
	FName ArenaLevelName = FName("Arena");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Levels")
	FName MainMenuLevelName = FName("L_MainMenu");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* DefaulMappingContext;


private:

	UPROPERTY()
	UHordeSaveGame* CachedSaveGame = nullptr;

	UPROPERTY()
	FString SaveSlotName = FString("HordeShooterSaveSlot");

	void LoadOrCreateSaveGame();
};
