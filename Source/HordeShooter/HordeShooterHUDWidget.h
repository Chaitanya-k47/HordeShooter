// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HordeShooterHUDWidget.generated.h"

class UTextBlock;
class UWidget;
class UProgressBar;
class UButton;
class UCanvasPanel;

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

	UPROPERTY(meta = (BindWidget))
	UProgressBar* DashBar1; //left Bar

	UPROPERTY(meta = (BindWidget))
	UProgressBar* DashBar2; //right Bar

	UPROPERTY(meta = (BindWidget))
	UCanvasPanel* IndicatorCanvas;

	//bp class for sub widget that shows damage direction indicator
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> DamageIndicatorClass;

	virtual void NativeConstruct() override;

public:

	UFUNCTION()
	void UpdateAmmo(int32 CurrentAmmo, int32 TotalAmmoReserve);

	UFUNCTION()
	void UpdateHealth(float CurrentHealth, float MaxHealth);

	UFUNCTION()
	void UpdateDashBars(int32 AvailableDashes, float TimeRemaining, float TotalCooldown, bool bCanDash);
	
	void ToggleCrosshair(bool bShow);
	void ShowGameOver();

	void ShowDamageIndicator(float Angle);

private:

	UFUNCTION()
	void OnRestartClicked();

};
