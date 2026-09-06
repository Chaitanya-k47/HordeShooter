// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerProgressionComponent.h"
#include "HordeShooterCharacter.h"

// Sets default values for this component's properties
UPlayerProgressionComponent::UPlayerProgressionComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}

void UPlayerProgressionComponent::ApplyUpgrade(EPlayerUpgradeType UpgradeType)
{
	AHordeShooterCharacter* Player = Cast<AHordeShooterCharacter>(GetOwner());
	if(!Player) return;
	
	switch(UpgradeType)
	{
		case EPlayerUpgradeType::WeaponDamage:
			DamageUpgradeLevel++;
			//dynamically scales at run time, hence just upgrade the level
			break;

		case EPlayerUpgradeType::MaxHealth:
			HealthUpgradeLevel++;
			//push the increment directly to player character
			Player->UpgradeMaxHealth(HealthIncreasePerLevel);
			break;

		case EPlayerUpgradeType::AmmoCapacity:
			AmmoUpgradeLevel++;
			Player->UpgradeAmmoCapacity(GetAmmoMultiplier());
			break;
	}
}

float UPlayerProgressionComponent::GetDamageMultiplier() const
{
    return 1.f + (DamageUpgradeLevel * DamageIncreasePerLevel);
}

float UPlayerProgressionComponent::GetAmmoMultiplier() const
{
    return 1.f + (AmmoUpgradeLevel * AmmoIncreasePerLevel);
}


// Called when the game starts
void UPlayerProgressionComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UPlayerProgressionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

