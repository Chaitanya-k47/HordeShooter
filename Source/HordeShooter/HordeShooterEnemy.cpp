// Fill out your copyright notice in the Description page of Project Settings.


#include "HordeShooterEnemy.h"

#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"

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
	MoveComp->bOrientRotationToMovement = false;
	MoveComp->bUseControllerDesiredRotation = true;
	MoveComp->RotationRate = FRotator(0.f, 600.f, 0.f);

}

// Called when the game starts or when spawned
void AHordeShooterEnemy::BeginPlay()
{
	Super::BeginPlay();
	ResetEnemy();

	if(GetMesh() && GetMesh()->GetAnimInstance())
	{
		GetMesh()->GetAnimInstance()->OnMontageEnded.AddDynamic(this, &AHordeShooterEnemy::OnMontageEnded);
	}
	
}

// Called every frame
// void AHordeShooterEnemy::Tick(float DeltaTime)
// {
// 	Super::Tick(DeltaTime);

// }

void AHordeShooterEnemy::ResetEnemy()
{
	CurrentHealth = MaxHealth;
	bIsDead = false;
	bIsStunned = false;
	bIsAttacking = false;
	
	// Reset Physics & Collision for when we pull them from the Object Pool
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	
	GetMesh()->SetSimulatePhysics(false);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	GetMesh()->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::SnapToTargetIncludingScale);
}

//COMBAT ACTIONS:
void AHordeShooterEnemy::PerformMeleeAttack()
{
	if(bIsAttacking || bIsStunned || bIsDead || AttackMontages.Num() == 0) return;

	bIsAttacking = true;
	int32 RandomIndex = FMath::RandRange(0, AttackMontages.Num() - 1);

	if(GetMesh()->GetAnimInstance())
	{
		GetMesh()->GetAnimInstance()->Montage_Play(AttackMontages[RandomIndex]);
	}
}

void AHordeShooterEnemy::PlayHitReaction()
{
	if(bIsDead || HitReactionMontages.Num() == 0) return;

	bIsStunned = true;
	int32 RandomIndex = FMath::RandRange(0, HitReactionMontages.Num() - 1);
	
	if(GetMesh()->GetAnimInstance())
	{
		GetMesh()->GetAnimInstance()->Montage_Play(HitReactionMontages[RandomIndex]);
	}
}

void AHordeShooterEnemy::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if(AttackMontages.Contains(Montage))
	{
		bIsAttacking = false;
		OnAttackFinished.Broadcast(); //tell AI controller that the attack is finished.
	}
	else if(HitReactionMontages.Contains(Montage))
	{
		bIsStunned = false;
	}
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
	else
	{
		PlayHitReaction();
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

