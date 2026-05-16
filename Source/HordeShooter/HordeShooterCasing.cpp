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

	CasingMesh->SetSimulatePhysics(true);
	CasingMesh->SetEnableGravity(true);
	CasingMesh->SetNotifyRigidBodyCollision(true); //enable hit events
}

// Called when the game starts or when spawned
void AHordeShooterCasing::BeginPlay()
{
	Super::BeginPlay();
	
	//Bind OnHit() fn to mesh's hit event.
	CasingMesh->OnComponentHit.AddDynamic(this, &AHordeShooterCasing::OnHit);

	//tell casing to despawn:
	SetLifeSpan(3.f);
}

// Called every frame
void AHordeShooterCasing::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

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

void AHordeShooterCasing::EjectCasing(FVector ShooterVelocity)
{
	//inherit momentum from shooter:
	CasingMesh->SetPhysicsLinearVelocity(ShooterVelocity);

	FVector EjectionDirection = GetActorForwardVector() + FMath::VRand() * 0.1f; //randomize the direction a bit

	//add impulse(velocity change = true means mass of casing is ignored)
	CasingMesh->AddImpulse(EjectionDirection * ShellEjectionImpulse, NAME_None, true);

	//add random spin:
	CasingMesh->SetPhysicsAngularVelocityInDegrees(FMath::VRand() * 2000.f);
}

