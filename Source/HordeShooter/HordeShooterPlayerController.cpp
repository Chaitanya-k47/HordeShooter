// Fill out your copyright notice in the Description page of Project Settings.


#include "HordeShooterPlayerController.h"
#include "HordeShooterHUDWidget.h"
#include "Kismet/GameplayStatics.h"


void AHordeShooterPlayerController::BeginPlay()
{
    Super::BeginPlay();
    
    bShowMouseCursor = false;
    FInputModeGameOnly InputMode;
    SetInputMode(InputMode);

    if(PlayerHUDClass)
    {
        PlayerHUDWidget = CreateWidget<UHordeShooterHUDWidget>(this, PlayerHUDClass);
        
        if(PlayerHUDWidget)
        {
            PlayerHUDWidget->AddToViewport();
        }
    }
}

void AHordeShooterPlayerController::ShowGameOverScreen()
{
    if(PlayerHUDWidget)
    {
        PlayerHUDWidget->ShowGameOver();
    }

    //pause the game so enemies stop attacking
	UGameplayStatics::SetGamePaused(GetWorld(), true);

    //show cursor and give input to UI
    bShowMouseCursor = true;
	FInputModeUIOnly InputMode;
    SetInputMode(InputMode);
}

void AHordeShooterPlayerController::ShowUpgradeScreen()
{
    UGameplayStatics::SetGamePaused(GetWorld(), true);

	bShowMouseCursor = true;
	FInputModeUIOnly InputMode;
	SetInputMode(InputMode);

    if(PlayerHUDWidget) PlayerHUDWidget->ShowUpgradeScreen();
}

void AHordeShooterPlayerController::PlayAnnouncerSound(USoundBase* Sound)
{
    if(Sound)
    {
        UGameplayStatics::PlaySound2D(GetWorld(), Sound, 1.0f, 1.0f, 0.0f, AnnouncerConcurrency);
    }
}

void AHordeShooterPlayerController::AddKill(FName DamageSource, bool bWasHeadshot, bool bIsAirborne, bool bIsLowHealth)
{
    SpreeCount++; //ruin spree count when player takes damage
    MultiKillCount++; //ruin multikill count when combo window is missed

    USoundBase* SoundToPlay = nullptr;

    //CONTEXT KILLS (lowest priority)
    if(bIsAirborne) SoundToPlay = Sound_TopGun;
    else if(bIsLowHealth) SoundToPlay = Sound_Retribution;
    else if(bWasHeadshot) SoundToPlay = Sound_HeadShot;
    else if(DamageSource == FName("Slam")) SoundToPlay = Sound_Pancake;
    else if(DamageSource == FName("Melee")) SoundToPlay = Sound_JackHammer;
    else if(DamageSource == FName("AltFire")) SoundToPlay = Sound_Eradication;
   

    //MULTIKILL (overrides context kills)
    if(MultiKillCount == 2) SoundToPlay = Sound_DoubleKill;
	else if(MultiKillCount == 3) SoundToPlay = Sound_MultiKill;
	else if(MultiKillCount == 4) SoundToPlay = Sound_MegaKill;
	else if(MultiKillCount == 5) SoundToPlay = Sound_UltraKill;
	else if(MultiKillCount == 6) SoundToPlay = Sound_MonsterKill;
	else if(MultiKillCount == 7) SoundToPlay = Sound_Outstanding;
	else if(MultiKillCount == 8) SoundToPlay = Sound_Unreal;
    else if(MultiKillCount == 9) SoundToPlay = Sound_BloodBath;
    else if(MultiKillCount >= 10) SoundToPlay = Sound_ComboKing;

    //SURVIVAL SPREE (overrides multikills)
    if(SpreeCount == 5) SoundToPlay = Sound_KillingSpree;  
	else if(SpreeCount == 10) SoundToPlay = Sound_Rampage;
	else if(SpreeCount == 15) SoundToPlay = Sound_Dominating;
	else if(SpreeCount == 20) SoundToPlay = Sound_Unstoppable;
    else if(SpreeCount == 25) SoundToPlay = Sound_GodLike;
    else if(SpreeCount >= 30 && (SpreeCount % 5 == 0)) SoundToPlay = Sound_Massacre; //for every 5 kills after 30 and on 30.

    //FIRST BLOOD (absolute highest priority)
    if(!bHasFirstBlood)
	{
		bHasFirstBlood = true;
		SoundToPlay = Sound_FirstBlood;
	}

    //play the sound:
    if(SoundToPlay) PlayAnnouncerSound(SoundToPlay);

    //refresh the combo timer:
    //if window missed all ResetMultiKill()
    GetWorldTimerManager().SetTimer(MultiKillTimerHandle, this, &AHordeShooterPlayerController::ResetMultiKill, MultiKillWindow, false);
}

void AHordeShooterPlayerController::ResetMultiKill()
{
	MultiKillCount = 0;
    GetWorldTimerManager().ClearTimer(MultiKillTimerHandle);
}

void AHordeShooterPlayerController::ResetSpree()
{
	SpreeCount = 0;
}

