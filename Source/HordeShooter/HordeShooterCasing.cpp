// Fill out your copyright notice in the Description page of Project Settings.


#include "HordeShooterCasing.h"
#include "Components/StaticMeshComponent.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AHordeShooterCasing::AHordeShooterCasing()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	CasingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CasingMesh"));
	RootComponent = CasingMesh;

	CasingMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore); //ignore camera collisions
	CasingMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore); //ignore player collisions
	CasingMesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Ignore); 
	CasingMesh->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Ignore);
	CasingMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	CasingMesh->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
}

// Called when the game starts or when spawned
void AHordeShooterCasing::BeginPlay()
{
	Super::BeginPlay();
	
	//Bind OnHit() fn to mesh's hit event.
	CasingMesh->OnComponentHit.AddDynamic(this, &AHordeShooterCasing::OnHit);

	//start game with casing asleep.
	DeactivateCasing();
}

// Called every frame
void AHordeShooterCasing::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AHordeShooterCasing::ActivateAndEject(const FTransform& StartTransform, FVector ShooterVelocity)
{
	//reset state:
	bHasBounced = false;
	GetWorldTimerManager().ClearTimer(DeactivateTimerHandle);

	//teleport to the spawn location and activate physics:
	SetActorTransform(StartTransform, false, nullptr, ETeleportType::TeleportPhysics);
	CasingMesh->SetHiddenInGame(false);
	CasingMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CasingMesh->SetSimulatePhysics(true);
	CasingMesh->SetNotifyRigidBodyCollision(true);

	//clear any old physics state:
	CasingMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
	CasingMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);

	//apply new momentum:
	CasingMesh->SetPhysicsLinearVelocity(ShooterVelocity); //inherit momentum from shooter
	FVector EjectionDirection = GetActorForwardVector() + FMath::VRand() * 0.1f; //randomize the direction a bit
	CasingMesh->AddImpulse(EjectionDirection * ShellEjectionImpulse, NAME_None, true); //add impulse(velocity change = true means mass of casing is ignored)
	CasingMesh->SetPhysicsAngularVelocityInDegrees(FMath::VRand() * 2000.f); //add random spin

	//set timer to return to pool after 3 seconds:
	GetWorldTimerManager().SetTimer(DeactivateTimerHandle, this, &AHordeShooterCasing::DeactivateCasing, 3.f, false);
}

void AHordeShooterCasing::DeactivateCasing()
{
	//Put the casing to sleep and hide it
	CasingMesh->SetSimulatePhysics(false);
	CasingMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CasingMesh->SetHiddenInGame(true);
	SetActorLocation(FVector(0.f, 0.f, -10000.f)); 
}

void AHordeShooterCasing::OnHit(
		UPrimitiveComponent* HitComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp, 
		FVector NormalImpulse,
		const FHitResult& Hit
	)
{
	if(!bHasBounced && ShellBounceSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), ShellBounceSound, GetActorLocation());
		bHasBounced = true; //only play sound on first bounce
	}
}