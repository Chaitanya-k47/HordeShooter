// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HordeShooterHUDWidget.generated.h"

class UTextBlock;
class UWidget;
class UProgressBar;
class UButton;

UCLASS()
class HORDESHOOTER_API UHordeShooterHUDWidget : public UUserWidget
{
	GENERATED_BODY()


protected:

	// meta=(BindWidget) forces UE to link this C++ variable to the UI element with exact same name
	UPROPERTY(meta = (BindWidget))
	UTextBlock* AmmoInMagazine;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TotalAmmo;

	UPROPERTY(meta = (BindWidget))
	UWidget* CrossHair;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* HealthBar;

	UPROPERTY(meta = (BindWidget))
	UWidget* GameOverPanel; //a border/canvas containing "Game Over" text and the button

	UPROPERTY(meta = (BindWidget))
	UButton* RestartButton;

	virtual void NativeConstruct() override;

public:

	UFUNCTION()
	void UpdateAmmo(int32 CurrentAmmo, int32 MagSize);

	UFUNCTION()
	void UpdateHealth(float CurrentHealth, float MaxHealth);
	
	void ToggleCrosshair(bool bShow);
	void ShowGameOver();

private:

	UFUNCTION()
	void OnRestartClicked();

};
