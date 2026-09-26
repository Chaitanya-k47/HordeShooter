// Fill out your copyright notice in the Description page of Project Settings.


#include "HordeShooterProjectile.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraComponent.h"
#include "DamageableInterface.h"

// Sets default values
AHordeShooterProjectile::AHordeShooterProjectile()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	RootComponent = CollisionSphere;
	CollisionSphere->InitSphereRadius(15.0f);

	ProjectileVFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ProjectileVFX"));
	ProjectileVFX->SetupAttachment(RootComponent);
	ProjectileVFX->bAutoActivate = false;

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));

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

void AHordeShooterProjectile::ActivateProjectile(const FVector &StartLocation, const FVector &Direction)
{
	bIsActive = true;

	SetActorLocationAndRotation(StartLocation, Direction.Rotation());

	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ProjectileVFX->Activate(true);
	SetActorHiddenInGame(false);

	ProjectileMovement->SetUpdatedComponent(CollisionSphere);
	ProjectileMovement->Velocity = Direction * ProjectileMovement->InitialSpeed;
	ProjectileMovement->Activate();

	GetWorldTimerManager().SetTimer(FailsafeDeactivateTimer, this, &AHordeShooterProjectile::DeactivateProjectile, MaxLifespan, false);
}

void AHordeShooterProjectile::DeactivateProjectile()
{
	bIsActive = false;

	ProjectileMovement->Deactivate();
	ProjectileMovement->Velocity = FVector::ZeroVector;
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	ProjectileVFX->DeactivateImmediate();
	SetActorHiddenInGame(true);

	GetWorldTimerManager().ClearTimer(FailsafeDeactivateTimer);
	SetActorLocation(FVector(0, 0, -10000.f));
}

void AHordeShooterProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if(!bIsActive) return;

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


