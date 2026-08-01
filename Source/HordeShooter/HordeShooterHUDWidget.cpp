// Fill out your copyright notice in the Description page of Project Settings.


#include "HordeShooterHUDWidget.h"

#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"

void UHordeShooterHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

    if(HealthBar) HealthBar->SetVisibility(ESlateVisibility::Visible);
	if(AmmoInMagazine) AmmoInMagazine->SetVisibility(ESlateVisibility::Visible);
	if(TotalAmmo) TotalAmmo->SetVisibility(ESlateVisibility::Visible);
	ToggleCrosshair(true);

	//hide "GameOver" panel on start
	if(GameOverPanel)
	{
		GameOverPanel->SetVisibility(ESlateVisibility::Hidden);
	}

	//bind teh restart button to OnRestartClicked event
	if(RestartButton)
	{
		RestartButton->OnClicked.AddDynamic(this, &UHordeShooterHUDWidget::OnRestartClicked);
	}
}


void UHordeShooterHUDWidget::UpdateAmmo(int32 CurrentAmmo, int32 MagSize)
{
    if(AmmoInMagazine && TotalAmmo)
    {
        AmmoInMagazine->SetText(FText::AsNumber(CurrentAmmo));
        TotalAmmo->SetText(FText::AsNumber(MagSize));
    }
}


void UHordeShooterHUDWidget::ToggleCrosshair(bool bShow)
{
    if(CrossHair)
    {
        CrossHair->SetVisibility(bShow ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
    }
}


void UHordeShooterHUDWidget::UpdateHealth(float CurrentHealth, float MaxHealth)
{
    if(HealthBar)
    {
        HealthBar->SetPercent(FMath::Clamp(CurrentHealth/MaxHealth, 0.f, 1.f));
    }   
}

void UHordeShooterHUDWidget::ShowGameOver()
{
	if(GameOverPanel)
	{
        ToggleCrosshair(false);
        if(HealthBar) HealthBar->SetVisibility(ESlateVisibility::Hidden);
	    if(AmmoInMagazine) AmmoInMagazine->SetVisibility(ESlateVisibility::Hidden);
	    if(TotalAmmo) TotalAmmo->SetVisibility(ESlateVisibility::Hidden);

		GameOverPanel->SetVisibility(ESlateVisibility::Visible);
	}
}

void UHordeShooterHUDWidget::OnRestartClicked()
{
    UWorld* World = GetWorld();
    if(World)
    {
        if (APlayerController* PC = GetOwningPlayer())
		{
			PC->bShowMouseCursor = false;
			PC->SetInputMode(FInputModeGameOnly());
		}

        UGameplayStatics::SetGamePaused(World, false);
        
        FString SafeLevelName = UGameplayStatics::GetCurrentLevelName(World, true);
		UGameplayStatics::OpenLevel(World, FName(*SafeLevelName));
    }
}

