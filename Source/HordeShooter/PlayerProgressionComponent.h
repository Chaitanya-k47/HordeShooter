// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerProgressionComponent.generated.h"

UENUM(BlueprintType)
enum class EPlayerUpgradeType : uint8
{
	WeaponDamage,
	MaxHealth,
	AmmoCapacity
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class HORDESHOOTER_API UPlayerProgressionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UPlayerProgressionComponent();

	//called by UI widget when player select and upgrade type:
	UFUNCTION(BlueprintCallable, Category = "Progression")
	void ApplyUpgrade(EPlayerUpgradeType UpgradeType);

	//multiplier getters for dynamic stat calculations:
	float GetDamageMultiplier() const;
	float GetAmmoMultiplier() const;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	//CONFIGURATION:
	UPROPERTY(EditDefaultsOnly, Category = "Progression|Config")
	float DamageIncreasePerLevel = 0.1f; //10% increase per level(every time player upgrades this stat)

	UPROPERTY(EditDefaultsOnly, Category = "Progression|Config")
	float HealthIncreasePerLevel = 25.f; //25 health increase per level(every time player upgrades this stat)

	UPROPERTY(EditDefaultsOnly, Category = "Progression|Config")
	float AmmoIncreasePerLevel = 0.25f; //25% ammo increase per level(every time player upgrades this stat)


	//INTERNAL TRACKERS:
	//trackers for how many times player has upgraded each stat(i.e. the upgrade level for each stat):
	UPROPERTY(VisibleAnywhere, Category = "Progression|Stats")
	int32 DamageUpgradeLevel = 0;

	UPROPERTY(VisibleAnywhere, Category = "Progression|Stats")
	int32 HealthUpgradeLevel = 0;

	UPROPERTY(VisibleAnywhere, Category = "Progression|Stats")
	int32 AmmoUpgradeLevel = 0;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
