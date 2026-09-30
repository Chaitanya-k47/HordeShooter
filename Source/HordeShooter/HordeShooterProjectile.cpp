// Fill out your copyright notice in the Description page of Project Settings.


#include "HordeShooterProjectile.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "DamageableInterface.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"   

// Sets default values
AHordeShooterProjectile::AHordeShooterProjectile()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	RootComponent = CollisionSphere;
	CollisionSphere->InitSphereRadius(CollisionSphereRadius);

	CoreVFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("CoreVFX"));
	CoreVFX->SetupAttachment(RootComponent);
	CoreVFX->bAutoActivate = false;

	TrailVFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("TrailVFX"));
	TrailVFX->SetupAttachment(RootComponent);
	TrailVFX->bAutoActivate = false;

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));

	FlightAudioComp = CreateDefaultSubobject<UAudioComponent>(TEXT("FlightAudioComp"));
	FlightAudioComp->SetupAttachment(RootComponent);
	FlightAudioComp->bAutoActivate = false;

	//optimization:
	CollisionSphere->SetCollisionProfileName(TEXT("Custom"));
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision); //starts asleep
	CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block); //blocks and ramps
	CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block); //player

	ProjectileMovement->UpdatedComponent = CollisionSphere;
	ProjectileMovement->InitialSpeed = 2000.f;
	ProjectileMovement->MaxSpeed = 2000.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->ProjectileGravityScale = 0.0f; //flies straight(no gravity)
	ProjectileMovement->bAutoActivate = false;
}

// Called when the game starts or when spawned
void AHordeShooterProjectile::BeginPlay()
{
	Super::BeginPlay();
	CollisionSphere->OnComponentHit.AddDynamic(this, &AHordeShooterProjectile::OnHit);
	DeactivateProjectile();
	
}

void AHordeShooterProjectile::ActivateProjectile(const FVector &StartLocation, const FVector &Direction, AActor* Shooter, FLinearColor PlasmaColor, float InDamage, float InSpeed)
{
	bIsActive = true;

	GetWorldTimerManager().ClearTimer(RibbonDecayTimer);

	//deactivate old stuff
	TrailVFX->DeactivateImmediate();
	CoreVFX->DeactivateImmediate();
	
	SetActorHiddenInGame(false);
	SetActorLocationAndRotation(StartLocation, Direction.Rotation(), false, nullptr, ETeleportType::TeleportPhysics);

	Damage = InDamage;
	if(Shooter) CollisionSphere->IgnoreActorWhenMoving(Shooter, true);

	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	
	CoreVFX->Activate(true); 
	CoreVFX->SetFloatParameter(FName("SizeMultiplier"), (CollisionSphereRadius / 70.0f));
	CoreVFX->SetVariableLinearColor(FName("EnergyColour"), PlasmaColor);

	TrailVFX->SetFloatParameter(FName("SizeMultiplier"), (CollisionSphereRadius / 70.0f));
	TrailVFX->SetVariableLinearColor(FName("EnergyColour"), PlasmaColor);
	TrailVFX->Activate(true);

	if (FlightAudioComp->Sound) FlightAudioComp->Play();

	ProjectileMovement->InitialSpeed = InSpeed;
	ProjectileMovement->MaxSpeed = InSpeed;
	ProjectileMovement->SetUpdatedComponent(CollisionSphere);
	ProjectileMovement->Velocity = Direction * ProjectileMovement->InitialSpeed;
	ProjectileMovement->Activate();

	GetWorldTimerManager().SetTimer(FailsafeDeactivateTimer, this, &AHordeShooterProjectile::DeactivateProjectile, MaxLifespan, false);
}

void AHordeShooterProjectile::DeactivateProjectile()
{
	if(!bIsActive) return;
	bIsActive = false;

	GetWorldTimerManager().ClearTimer(FailsafeDeactivateTimer);

	ProjectileMovement->Deactivate();
	ProjectileMovement->Velocity = FVector::ZeroVector;
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FlightAudioComp->FadeOut(0.1f, 0.0f);
	
	CoreVFX->SetFloatParameter(FName("SizeMultiplier"), 1.0f);
	CoreVFX->SetVariableLinearColor(FName("EnergyColour"), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
	CoreVFX->DeactivateImmediate();

	TrailVFX->SetFloatParameter(FName("SizeMultiplier"), 1.0f);
	TrailVFX->SetVariableLinearColor(FName("EnergyColour"), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
	TrailVFX->Deactivate();

	GetWorldTimerManager().SetTimer(RibbonDecayTimer, this, &AHordeShooterProjectile::ReturnToPool, 0.4f, false);
}

void AHordeShooterProjectile::ReturnToPool()
{
	TrailVFX->DeactivateImmediate();
	SetActorHiddenInGame(true);
	SetActorLocation(FVector(0.0f, 0.0f, -10000.0f), false, nullptr, ETeleportType::TeleportPhysics);
}

void AHordeShooterProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if(!bIsActive) return;

	if(ProjectileHitSound) UGameplayStatics::PlaySoundAtLocation(GetWorld(), ProjectileHitSound, Hit.ImpactPoint);
	if(ProjectileHitVFX)
	{
		UNiagaraComponent* Impact = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ProjectileHitVFX, Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
		if(Impact)
		{
			Impact->SetFloatParameter(FName("SizeMultiplier"), CollisionSphereRadius / 70.0f); 
		}
	}

	if(OtherActor && OtherActor->GetClass()->ImplementsInterface(UDamageableInterface::StaticClass()))
	{
		IDamageableInterface* Damageable = Cast<IDamageableInterface>(OtherActor);
		if(Damageable)
		{
			FVector PushDirection = ProjectileMovement->Velocity.GetSafeNormal();
			PushDirection.Z += 0.2f;
			Damageable->ReactToHit(Damage, PushDirection * 15000.0f, NAME_None, FName("Plasma"));
		}
	}

	DeactivateProjectile();
}


