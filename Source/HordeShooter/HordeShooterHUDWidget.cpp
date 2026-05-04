// Fill out your copyright notice in the Description page of Project Settings.


#include "HordeShooterHUDWidget.h"

#include "Components/TextBlock.h"


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
