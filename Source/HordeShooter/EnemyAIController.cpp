// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyAIController.h"

#include "HordeShooterEnemy.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"

AEnemyAIController::AEnemyAIController()
{
    
}

void AEnemyAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    ControlledEnemy = Cast<AHordeShooterEnemy>(InPawn);

    if(ControlledEnemy)
    {
        ControlledEnemy->OnAttackFinished.AddDynamic(this, &AEnemyAIController::OnEnemyAttackFinished);
        
        //wait a short time before trying to find the player, to ensure the player has spawned
        GetWorldTimerManager().SetTimer(FindPlayerTimer, this, &AEnemyAIController::FindPlayer, 0.1f, true);
    }
}

void AEnemyAIController::FindPlayer()
{
	PlayerTarget = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	
	if(PlayerTarget)
	{
        GetWorldTimerManager().ClearTimer(FindPlayerTimer);//player found stop the timer.
        CurrentState = EAIState::Chasing;

        //start the AI tick timer to update AI logic every 0.1 seconds
        GetWorldTimerManager().SetTimer(AITickTimer, this, &AEnemyAIController::UpdateAILogic, 0.1f, true);
	}
}

void AEnemyAIController::UpdateAILogic()
{
    if (!ControlledEnemy || !PlayerTarget || ControlledEnemy->bIsDead) 
	{
		StopMovement();
		return;
	}

    //priority Override: If getting hit/stunned or currently attacking, don't change states
	if (ControlledEnemy->bIsStunned || ControlledEnemy->bIsAttacking)
	{
		StopMovement();
		return;
	}

    //STATE: Chasing
    if(CurrentState == EAIState::Chasing)
    {
        MoveToActor(PlayerTarget, ControlledEnemy->AttackRange);

        float DistanceToPlayer = FVector::Dist(ControlledEnemy->GetActorLocation(), PlayerTarget->GetActorLocation());

        //check LOS if in range:
        if(DistanceToPlayer <= ControlledEnemy->AttackRange && CheckLineOfSight())
        {
            CurrentState = EAIState::Attacking;
        }
    }

    //STATE: Attacking
    else if(CurrentState == EAIState::Attacking)
    {
        StopMovement();       // Plant feet
		SetFocus(PlayerTarget); // Lock eyes on player
		
		//fire the C++ Montage
		ControlledEnemy->PerformMeleeAttack();
    }
}

bool AEnemyAIController::CheckLineOfSight()
{
	if(!ControlledEnemy || !PlayerTarget) return false;

	FHitResult Hit;
	FVector Start = ControlledEnemy->GetActorLocation();
	FVector End = PlayerTarget->GetActorLocation();
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(ControlledEnemy);

	bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);

	//if we hit the player, or hit nothing at all LOS is clear
	if(bHit && Hit.GetActor() == PlayerTarget) return true;
	if(!bHit) return true; 

	return false;
}

void AEnemyAIController::OnEnemyAttackFinished()
{
	//montage ended! Resume chasing.
	CurrentState = EAIState::Chasing;
	ClearFocus(EAIFocusPriority::Gameplay); //unlock the neck so NavMesh can steer again
}

void AEnemyAIController::BeginPlay()
{
    Super::BeginPlay();

    // APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    // if(PlayerPawn)
    // {
    //     SetFocus(PlayerPawn);
    // }
}

// void AEnemyAIController::Tick(float DeltaTime)
// {
//     Super::Tick(DeltaTime);

//     if(PlayerPawn)
//     {
//         MoveToActor(PlayerPawn, 150.f);
//     }
// }

