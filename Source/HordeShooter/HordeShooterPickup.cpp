// Fill out your copyright notice in the Description page of Project Settings.


#include "HordeShooterPickup.h"
#include "Components/SphereComponent.h"
#include "NiagaraComponent.h"
#include "HordeShooterCharacter.h"
#include "HordeShooterWeapon.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AHordeShooterPickup::AHordeShooterPickup()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false; //only tick when homing

	VacuumSphere = CreateDefaultSubobject<USphereComponent>(TEXT("VacuumSphere"));
	RootComponent = VacuumSphere;
	VacuumSphere->SetSphereRadius(VacuumSphereRadius);

	VacuumSphere->SetCollisionObjectType(ECC_WorldDynamic);
	VacuumSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	VacuumSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	VacuumSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	PickupVFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("PickupVFX"));
	PickupVFX->SetupAttachment(RootComponent);

}

// Called when the game starts or when spawned
void AHordeShooterPickup::BeginPlay()
{
	Super::BeginPlay();

	//bind to overlap event:
	VacuumSphere->OnComponentBeginOverlap.AddDynamic(this, &AHordeShooterPickup::OnVacuumOverlap);

	DeactivatePickup();
}

// Called every frame
void AHordeShooterPickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if(bIsHoming && TargetPlayer)
	{
		FVector CurrentLoc = GetActorLocation();
		FVector TargetLoc = TargetPlayer->GetActorLocation();

		FVector NewLoc = FMath::VInterpConstantTo(CurrentLoc, TargetLoc, DeltaTime, VacuumSpeed);
		SetActorLocation(NewLoc);

		//check if reached the player:
		if(FVector::DistSquared(NewLoc, TargetLoc) < 50.f * 50.f)
		{
			//grant Ammo
			if(TargetPlayer->GetInventory().Num() > 0)
			{
				float PickupPercentage = 0.25f;
				switch(CurrentSize)
				{
					case EPickupSize::Small:
						PickupPercentage = 0.25f;
						break;

					case EPickupSize::Medium:
						PickupPercentage = 0.5f;
						break;

					case EPickupSize::Large:
						PickupPercentage = 0.75f;
						break;

					default:
						break;
				}

				bool bWasConsumed = false;
				for(const auto& Weapon : TargetPlayer->GetInventory())
				{
					if(Weapon && Weapon->AddAmmo(PickupPercentage)) bWasConsumed = true;
				}

				if(bWasConsumed)
				{
					//play an audio cue directly on the player
					if(PickupSound) UGameplayStatics::PlaySound2D(GetWorld(), PickupSound);
					DeactivatePickup();
				}
				else
				{
					bIsHoming = false;
					TargetPlayer = nullptr;
					VacuumSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
					SetActorTickEnabled(false);
				}	
			}
		}
	}
}

void AHordeShooterPickup::ActivatePickup(const FVector& SpawnLocation, EPickupSize InSize)
{
	bIsActive = true;
	bIsHoming = false;
	CurrentSize = InSize;
	TargetPlayer = nullptr;

	SetActorLocation(SpawnLocation + FVector(0, 0, 50.f));

	VacuumSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	//scale the pickup based on size:
	float SizeMultiplier = 1.0f;
	switch(CurrentSize)
	{
		case EPickupSize::Small:
			SizeMultiplier = 1.f;
			break;

		case EPickupSize::Medium:
			SizeMultiplier = 2.f;
			break;

		case EPickupSize::Large:
			SizeMultiplier = 3.f;
			break;

		default:
			break;
	}
	
	PickupVFX->SetFloatParameter(FName("SizeMultiplier"), SizeMultiplier);
	PickupVFX->Activate(true);
	SetActorHiddenInGame(false);

	GetWorldTimerManager().SetTimer(FailsafeDeactivateTimer, this, &AHordeShooterPickup::DeactivatePickup, 10.f, false);
}

void AHordeShooterPickup::DeactivatePickup()
{
	bIsActive = false;
	bIsHoming = false;
	TargetPlayer = nullptr;
	SetActorTickEnabled(false);

	VacuumSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PickupVFX->SetFloatParameter(FName("SizeMultiplier"), 1.f);
	//PickupVFX->Deactivate();
	PickupVFX->DeactivateImmediate(); 
	SetActorHiddenInGame(true);

	GetWorldTimerManager().ClearTimer(FailsafeDeactivateTimer);
	SetActorLocation(FVector(0, 0, -10000.f));
}

void AHordeShooterPickup::OnVacuumOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if(!bIsHoming && OtherActor && OtherActor->IsA(AHordeShooterCharacter::StaticClass()))
	{
		AHordeShooterCharacter* Player = Cast<AHordeShooterCharacter>(OtherActor);

		//find atleast one weapon that needs ammo in the players inventory:
		bool bNeedsAmmo = false;
		for(const auto& Weapon : Player->GetInventory())
		{
			if(Weapon && Weapon->TotalAmmoReserve < Weapon->MaxAmmoReserve)
			{
				bNeedsAmmo = true;
				break;
			}
		}

		if(bNeedsAmmo)
		{
			TargetPlayer = Player;
			bIsHoming = true;
			VacuumSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			SetActorTickEnabled(true); //enable tick
		}
	}
}

