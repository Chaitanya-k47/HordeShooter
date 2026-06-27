// Fill out your copyright notice in the Description page of Project Settings.


#include "HordeShooterEnemy.h"

#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

// Sets default values
AHordeShooterEnemy::AHordeShooterEnemy()
{
	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	//let the AI Controller handle rotation, not the actor itself
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	MoveComp->bOrientRotationToMovement = true;
	MoveComp->bUseControllerDesiredRotation = false;
	MoveComp->RotationRate = FRotator(0.f, 600.f, 0.f);

}

// Called when the game starts or when spawned
void AHordeShooterEnemy::BeginPlay()
{
	Super::BeginPlay();
	ResetEnemy();
	
}

// Called every frame
void AHordeShooterEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AHordeShooterEnemy::ResetEnemy()
{
	CurrentHealth = MaxHealth;
	bIsDead = false;
	
	// Reset Physics & Collision for when we pull them from the Object Pool
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	
	GetMesh()->SetSimulatePhysics(false);
	GetMesh()->SetCollisionProfileName(TEXT("CharacterMesh"));
	GetMesh()->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::SnapToTargetIncludingScale);
	
	// Adjust these vectors if your specific enemy mesh imports sideways/floating
	GetMesh()->SetRelativeLocation(FVector(0, 0, -90));
	GetMesh()->SetRelativeRotation(FRotator(0, -90, 0));
}

void AHordeShooterEnemy::ReactToHit(float DamageAmount, const FVector& HitDirection)
{
	if(bIsDead) return;
	
	CurrentHealth -= DamageAmount;
	LastHitDirection = HitDirection;
	
	// Print to screen for easy debugging
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, FString::Printf(TEXT("Enemy Hit! Health: %f"), CurrentHealth));
	
	OnHit(DamageAmount); // Triggers Blueprint logic, then C++ default

	if(CurrentHealth <= 0.f)
	{
		Die();
	}
}

void AHordeShooterEnemy::Die()
{
	if(bIsDead) return;
	bIsDead = true;
	
	OnDeath(); // Triggers BP logic, then C++ default

	// Detach AI Controller
	DetachFromControllerPendingDestroy();

	// Temporary: Delete after 3 seconds. 
	// We will replace this later when we build the Horde Manager Object Pool!
	SetLifeSpan(3.0f);
}

void AHordeShooterEnemy::OnHit_Implementation(float DamageAmount)
{
	
}

void AHordeShooterEnemy::OnDeath_Implementation()
{
	// 1. Disable Capsule collision so players can walk over the corpse
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 2. Enable Ragdoll
	GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
	GetMesh()->SetSimulatePhysics(true);

	// 3. Add directional shot impulse! (Makes them fly backward from shotguns/lasers)
	float ShotForce = 5000.f; // Adjust this for heavier/lighter flying corpses
	GetMesh()->AddImpulse(LastHitDirection * ShotForce, NAME_None, true);
}

