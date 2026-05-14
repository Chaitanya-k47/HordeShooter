// Fill out your copyright notice in the Description page of Project Settings.


#include "HordeShooterWeapon.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneComponent.h"
#include "HordeShooterCharacter.h"
#include "Camera/CameraComponent.h"
#include "Animation/AnimMontage.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"


// Sets default values
AHordeShooterWeapon::AHordeShooterWeapon()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
}


// Called when the game starts or when spawned
void AHordeShooterWeapon::BeginPlay()
{
	Super::BeginPlay();

	CurrentAmmo = MagSize;
	
	if(!CurrentOwner && !bIsEquipped)
	{
		Mesh->SetVisibility(false);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}


// Called every frame
void AHordeShooterWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}


void AHordeShooterWeapon::StartFire()
{
	if(!bIsEquipped || !bCanFire || bIsReloading) return;

	if(CurrentAmmo<=0) Reload();

	//shoot the first bullet immediately
	PerformFire();

	if(bIsAutomatic)
		GetWorldTimerManager().SetTimer(FireTimerHandle, this, &AHordeShooterWeapon::PerformFire, FireRate, true);
	
}


void AHordeShooterWeapon::StopFire()
{
	GetWorldTimerManager().ClearTimer(FireTimerHandle);
}


void AHordeShooterWeapon::PerformFire()
{
	if(CurrentAmmo <= 0)
	{
		StopFire();
		Reload();
		return;
	}

	if(!bCanFire)
	{
		StopFire();
		return;
	}

	if(!CurrentOwner || !CurrentOwner->FirstPersonCamera) return;

	//consume ammo
	CurrentAmmo--;

	//broadcast
	OnAmmoChanged.Broadcast(CurrentAmmo, MagSize);

	if (CurrentOwner && CurrentOwner->CharacterArms)
	{
		if (ArmsFireMontage)
		{
			UAnimInstance* ArmsAnimInstance = CurrentOwner->CharacterArms->GetAnimInstance();
			if (ArmsAnimInstance)
			{
				UAnimMontage* MotageToPlay = CurrentOwner->bIsAiming ? ArmsFireMontage_Aimed : ArmsFireMontage;
				ArmsAnimInstance->Montage_Play(MotageToPlay);
			}
		}

		if (WeaponFireMontage && Mesh->GetAnimInstance())
		{
			Mesh->GetAnimInstance()->Montage_Play(WeaponFireMontage);
		}
	}

	//Muzzle Flash:
	if (MuzzleFlashSystem)
	{
		UNiagaraFunctionLibrary::SpawnSystemAttached(
			MuzzleFlashSystem,
			Mesh, // The weapon mesh
			FName("SOC_MuzzleFlash"), // The exact name of your socket!
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			true // Auto-destroy when the particle finishes playing
		);
	}

	//Shoot Sound
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), ShootSound, GetActorLocation());
	}

	//linetrace logic:
	FVector Start = CurrentOwner->FirstPersonCamera->GetComponentLocation();
	FVector ForwardVector = CurrentOwner->FirstPersonCamera->GetForwardVector();
	FVector End = Start + (ForwardVector * WeaponRange);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(CurrentOwner);

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		Start,
		End,
		ECC_Visibility,
		QueryParams
	);

	//Bullet Tracer:
	FVector TracerEndPoint = bHit ? HitResult.Location : End;
	if (TracerSystem)
	{
		// 1. Get the exact location of the gun barrel right now
		FTransform MuzzleTransform = Mesh->GetSocketTransform(FName("SOC_MuzzleFlash"));
		//FVector MuzzleLocation = Mesh->GetSocketLocation(FName("SOC_MuzzleFlash"));

		// 2. Spawn the beam starting at the muzzle
		UNiagaraComponent* TracerComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			TracerSystem,
			MuzzleTransform.GetLocation(),
			MuzzleTransform.Rotator(),
			FVector(1.f),
			true, // Auto destroy
			true, // Auto activate
			ENCPoolMethod::None,
			true // Pre-cull
		);

		// 3. Tell the Niagara System where the wall is!
		if (TracerComp)
		{
			TracerComp->SetVectorParameter(FName("TraceStart"), MuzzleTransform.GetLocation());
			TracerComp->SetVectorParameter(FName("TraceEnd"), TracerEndPoint);
		}
	}

	//apply damage:
	if(bHit && HitResult.GetActor())
	{
		//TODO: DamagableInterface Logic here.
	}

	if(!bIsAutomatic)
	{
		//if not automatic start cooldown immediately after shot
		bCanFire = false;
		GetWorldTimerManager().SetTimer(FireTimerHandle, this, &AHordeShooterWeapon::ResetFireCooldown, FireRate, false);
	}
}


void AHordeShooterWeapon::ResetFireCooldown()
{
	bCanFire = true;
}


void AHordeShooterWeapon::Reload()
{
	if(CurrentAmmo == MagSize || bIsReloading) return;
	
	bIsReloading = true;
	StopFire(); //cannot fire while reloading

	//reload time logic:
	float DynamicReloadTime = ReloadTime;
	if(CurrentOwner && CurrentOwner->CharacterArms)
	{
		if(ArmsReloadMontage)
		{
			UAnimInstance* ArmsAnimInstance = CurrentOwner->CharacterArms->GetAnimInstance();
			if(ArmsAnimInstance)
			{
				ArmsAnimInstance->Montage_Play(ArmsReloadMontage);
				
				//overwrite the timer duration with the exact length of the animation
				DynamicReloadTime = ArmsReloadMontage->GetPlayLength();
			}
		}

		if(WeaponReloadMontage && Mesh->GetAnimInstance())
		{
			Mesh->GetAnimInstance()->Montage_Play(WeaponReloadMontage);
		}
	}


	GetWorldTimerManager().SetTimer(ReloadTimerHandle, this, &AHordeShooterWeapon::FinishReload, DynamicReloadTime, false);
}


void AHordeShooterWeapon::FinishReload()
{
	CurrentAmmo = MagSize;
	bIsReloading = false;

	//broadcast
	OnAmmoChanged.Broadcast(CurrentAmmo, MagSize);
}








