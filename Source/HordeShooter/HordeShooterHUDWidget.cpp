// Fill out your copyright notice in the Description page of Project Settings.


#include "HordeShooterHUDWidget.h"

#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
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

void UHordeShooterHUDWidget::UpdateDashBars(int32 AvailableDashes, float TimeRemaining, float TotalCooldown, bool bCanDash)
{
	if(!DashBar1 || !DashBar2) return;

	//set color based on whether the player can dash or not
	FLinearColor ActiveColor = FLinearColor(0.0f, 0.8f, 1.0f, 1.0f); //Cyan
	FLinearColor LockedColor = FLinearColor(0.3f, 0.3f, 0.3f, 1.0f); //Grey

	FLinearColor CurrentColor = bCanDash ? ActiveColor : LockedColor;
	DashBar1->SetFillColorAndOpacity(CurrentColor);
	DashBar2->SetFillColorAndOpacity(CurrentColor);

	//STATE 3: fully charged
	if(AvailableDashes == 2)
	{
		DashBar1->SetPercent(1.0f);
		DashBar2->SetPercent(1.0f);
		return;
	}

	//calculate overall progress percentage:
	float Progress = 1.f - (TimeRemaining/TotalCooldown);

	//STATE 2: one dash used
	if(AvailableDashes == 1)
	{
		DashBar1->SetPercent(1.0f); // Left bar stays full
		DashBar2->SetPercent(Progress); // Right bar fills up
	}

	//STATE 1: both dases used (long penalty cooldown)
	else if(AvailableDashes == 0)
	{
		//we split the progress into two halves, one for each bar
		if(Progress < 0.5f)
		{
			//DashBar1 fills up first and DashBar2 stays empty
			DashBar1->SetPercent(Progress * 2.0f);
			DashBar2->SetPercent(0.0f);
		}
		else
		{
			DashBar1->SetPercent(1.0f); //left bar stays full
			DashBar2->SetPercent((Progress - 0.5f) * 2.0f); //right bar fills up
		}
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

void UHordeShooterHUDWidget::ShowDamageIndicator(float Angle)
{
	if(DamageIndicatorClass && IndicatorCanvas)
	{
		//spawn sub-widget
		UUserWidget* IndicatorWidget = CreateWidget<UUserWidget>(this, DamageIndicatorClass);
		if(IndicatorWidget)
		{
			//add this subwidget to the specific canvas panel
			UCanvasPanelSlot* CanvasSlot = IndicatorCanvas->AddChildToCanvas(IndicatorWidget);
			if(CanvasSlot)
			{
				CanvasSlot->SetAnchors(FAnchors(0.5f));
				CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
				CanvasSlot->SetPosition(FVector2D(0.f, 0.f));
				CanvasSlot->SetAutoSize(true);
			}

			//rotate the widget to face teh damage source
			IndicatorWidget->SetRenderTransformAngle(Angle);
		}
	}
}
