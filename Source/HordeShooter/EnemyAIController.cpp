// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyAIController.h"

#include "HordeShooterEnemy.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationSystem.h"

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

    float DistanceToPlayer = FVector::Dist(ControlledEnemy->GetActorLocation(), PlayerTarget->GetActorLocation());
    
    // GEngine->AddOnScreenDebugMessage(-1, 0.2f, FColor::Green, FString::Printf(TEXT("Distance to Player: %f"), DistanceToPlayer));
    // GEngine->AddOnScreenDebugMessage(-1, 0.2f, FColor::Green, FString::Printf(TEXT("AI State: %s"), *UEnum::GetValueAsString(CurrentState)));
    
   //STATE: Chasing
   //MoveToActor() if on nav mesh, if not then dont call MoveToActor() as it conflicts with MoveToLocation() and the enemy slides slowly. 
    if(CurrentState == EAIState::Chasing)
    {
        ControlledEnemy->GetCharacterMovement()->bOrientRotationToMovement = true;
        ControlledEnemy->GetCharacterMovement()->bUseControllerDesiredRotation = false;
        ClearFocus(EAIFocusPriority::Gameplay); //stop looking at the player while chasing.

        MoveToPlayer();

        //check LOS if in range:
        if(DistanceToPlayer <= ControlledEnemy->AttackRange && CheckLineOfSight())
        {
            //transition to attack
            CurrentState = EAIState::Attacking;
        }
    }

    //STATE: Attacking
    else if(CurrentState == EAIState::Attacking)
    {
        ControlledEnemy->GetCharacterMovement()->bOrientRotationToMovement = false;
        ControlledEnemy->GetCharacterMovement()->bUseControllerDesiredRotation = true;
        SetFocus(PlayerTarget); 

        ControlledEnemy->PerformMeleeAttack();

        MoveToPlayer();
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

void AEnemyAIController::MoveToPlayer()
{
    UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
    if(NavSys)
    {
        //Check if the player is on the nav mesh.
        //logic: if the 50x50x50 box(extent of projection) around the player overlaps with the nave mesh at any point, then bIsPlayerOnNavMesh = true. 
        FNavLocation NavCheck;
        bool bIsPlayerOnNavMesh = NavSys->ProjectPointToNavigation(PlayerTarget->GetActorLocation(), NavCheck, FVector(50.f, 50.f, 50.f));

        if (bIsPlayerOnNavMesh)
        {   
            //call MoveToActor
            MoveToActor(PlayerTarget, 100.f);
            LastEdgeLocation = FVector::ZeroVector; // Reset our edge tracker
        }
        else
        {
            //player is off the mesh
            //donot call MoveToActor use MoveToLocation().
            if (NavSys->ProjectPointToNavigation(PlayerTarget->GetActorLocation(), NavCheck, FVector(2000.f, 2000.f, 2000.f)))
            {
                //only update the path if the player moved significantly along the edge
                if (FVector::Dist(LastEdgeLocation, NavCheck.Location) > 50.0f)
                {
                    LastEdgeLocation = NavCheck.Location;
                    MoveToLocation(LastEdgeLocation, 100.f);
                }
            }
        }
    }
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

