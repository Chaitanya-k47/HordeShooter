// Fill out your copyright notice in the Description page of Project Settings.


#include "HordeGameInstance.h"
#include "HordeSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/GameUserSettings.h"

void UHordeGameInstance::Init()
{
    Super::Init();

    LoadOrCreateSaveGame();

    //Ensure saved scalability settings apply on boot
    if(GEngine)
    {
        if(UGameUserSettings* UserSettings = GEngine->GetGameUserSettings())
        {
            UserSettings->LoadSettings();
            UserSettings->ApplySettings(false);
        }
    }
}

void UHordeGameInstance::LoadOrCreateSaveGame()
{
    if(UGameplayStatics::DoesSaveGameExist(SaveSlotName, 0))
    {
        CachedSaveGame = Cast<UHordeSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, 0));
    }

    if(!CachedSaveGame)
    {
        CachedSaveGame = Cast<UHordeSaveGame>(UGameplayStatics::CreateSaveGameObject(UHordeSaveGame::StaticClass()));
        SaveGameData();
    }
}

float UHordeGameInstance::GetMouseSensitivity() const
{
   return CachedSaveGame ? CachedSaveGame->MouseSensitivity : 1.0f;
}

void UHordeGameInstance::SetMouseSensitivity(float NewSensitivity)
{
    if(CachedSaveGame)
    {
        CachedSaveGame->MouseSensitivity = FMath::Clamp(NewSensitivity, 0.1f, 5.0f);
        SaveGameData();
    }
}

void UHordeGameInstance::SaveGameData()
{
    if(CachedSaveGame)
    {
        UGameplayStatics::SaveGameToSlot(CachedSaveGame, SaveSlotName, 0);
    }
}

bool UHordeGameInstance::HasViewedArsenal() const
{
    return CachedSaveGame ? CachedSaveGame->bHasViewedArsenal : false;
}

void UHordeGameInstance::MarkArsenalViewed()
{
    if(CachedSaveGame && !CachedSaveGame->bHasViewedArsenal)
    {
        CachedSaveGame->bHasViewedArsenal = true;
        SaveGameData();
    }
}


