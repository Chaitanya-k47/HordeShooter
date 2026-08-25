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

public:
	UPROPERTY()
	UHordeShooterHUDWidget* PlayerHUDWidget; //actual pointer to store the created widget instance

	void ShowGameOverScreen();

	//ANNOUNCER SYSTEM:
	//called when an enemy dies
	void AddKill(bool bWasHeadshot, bool bWasSlam);

	//called when player takes damage, to ruin their spree
	void ResetSpree();


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	//UI CONFIG:
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UHordeShooterHUDWidget> PlayerHUDClass;

	//ANNOUNCER SYSTEM:
	UPROPERTY(EditDefaultsOnly, Category = "Audio|Announcer") USoundBase* Sound_FirstBlood;
	UPROPERTY(EditDefaultsOnly, Category = "Audio|Announcer") USoundBase* Sound_HeadShot;
	UPROPERTY(EditDefaultsOnly, Category = "Audio|Announcer") USoundBase* Sound_Pancake;
	UPROPERTY(EditDefaultsOnly, Category = "Audio|Announcer") USoundBase* Sound_DoubleKill;
	UPROPERTY(EditDefaultsOnly, Category = "Audio|Announcer") USoundBase* Sound_MultiKill;
	UPROPERTY(EditDefaultsOnly, Category = "Audio|Announcer") USoundBase* Sound_MegaKill;
	UPROPERTY(EditDefaultsOnly, Category = "Audio|Announcer") USoundBase* Sound_UltraKill;
	UPROPERTY(EditDefaultsOnly, Category = "Audio|Announcer") USoundBase* Sound_MonsterKill;
	UPROPERTY(EditDefaultsOnly, Category = "Audio|Announcer") USoundBase* Sound_Massacre;
	UPROPERTY(EditDefaultsOnly, Category = "Audio|Announcer") USoundBase* Sound_Unreal;
	UPROPERTY(EditDefaultsOnly, Category = "Audio|Announcer") USoundBase* Sound_KillingSpree;
	UPROPERTY(EditDefaultsOnly, Category = "Audio|Announcer") USoundBase* Sound_Rampage;
	UPROPERTY(EditDefaultsOnly, Category = "Audio|Announcer") USoundBase* Sound_Dominating;
	UPROPERTY(EditDefaultsOnly, Category = "Audio|Announcer") USoundBase* Sound_Unstoppable;
	UPROPERTY(EditDefaultsOnly, Category = "Audio|Announcer") USoundBase* Sound_GodLike;
	UPROPERTY(EditDefaultsOnly, Category = "Audio|Announcer") USoundBase* Sound_ComboKing;

	UPROPERTY(EditDefaultsOnly, Category = "Audio|Announcer")
	float MultiKillWindow = 3.f; //3 sec to chain a multikill


private:
	bool bHasFirstBlood = false;
	int32 MultiKillCount = 0;
	int32 SpreeCount = 0;

	FTimerHandle MultiKillTimerHandle;
	void ResetMultiKill();
	void PlayAnnouncerSound(USoundBase* Sound);

};
