// Fill out your copyright notice in the Description page of Project Settings.


#include "ArenaManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "GameFramework/Character.h"

// Sets default values
AArenaManager::AArenaManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	GridMesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("GridMesh"));
	RootComponent = GridMesh;
	
	//allows collision to update when instances move
	GridMesh->bUseDefaultCollision = true;
}

// Called when the game starts or when spawned
void AArenaManager::BeginPlay()
{
	Super::BeginPlay();
	
	//initialize the tracking arrays:
	int32 TotalBlocks = GridSizeX * GridSizeY;
	CurrentHeights.Init(0.f, TotalBlocks);
	TargetHeights.Init(0.f, TotalBlocks);
	InstanceTransforms.Init(FTransform::Identity, TotalBlocks);

	float MeshSizeX = 100.f;
	float MeshSizeY = 100.f;
	float MeshSizeZ = 100.f;

	if(GridMesh->GetStaticMesh())
	{
		FBox BoundingBox = GridMesh->GetStaticMesh()->GetBoundingBox();
		MeshSizeX = BoundingBox.GetSize().X;
		MeshSizeY = BoundingBox.GetSize().Y;
		MeshSizeZ = BoundingBox.GetSize().Z;
	}

	//calculate max wall height:
	float MaxBlockHeight = BlockLocZ + (MaxStairSteps * BlockSize); //the maximum height a block will ever reach from the spawning location BlockLocZ.
	
	//this block is to be scaled along Z, hence the dimension in Z to be scaled upto:
	float TotalPillarHeight = (MaxStairSteps * BlockSize) + BlockSize; //adding a block more to the height for safety.

	//calculate absolute centre of the grid:
	float CenterX = ((GridSizeX - 1) * BlockSize) / 2.0f;
	float CenterY = ((GridSizeY - 1) * BlockSize) / 2.0f;
	FVector ArenaCenter = FVector(CenterX, CenterY, 0.0f) + GetActorLocation();

	//autp position the player:
	ACharacter* PlayerChar = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	if (PlayerChar)
	{
		PlayerSpawnHeight += TotalPillarHeight;
		PlayerChar->SetActorLocation(FVector(ArenaCenter.X, ArenaCenter.Y, PlayerSpawnHeight));
	}

	//resize nav mesh:
	TArray<AActor*> NavVolumes;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ANavMeshBoundsVolume::StaticClass(), NavVolumes);
	if (NavVolumes.Num() > 0 && NavVolumes[0])
	{
		ANavMeshBoundsVolume* NavVolume = Cast<ANavMeshBoundsVolume>(NavVolumes[0]);
		
		//the default Unreal Engine brush is 200x200x200 units.
		//divide the desired size by 200 to get the exact scale.
		float ScaleX = (GridSizeX * BlockSize) / 200.0f;
		float ScaleY = (GridSizeY * BlockSize) / 200.0f;

		//dist between min and max height for the AI or player to navigate.
		float PlayableHeight = (MaxStairSteps * BlockSize);

		float ScaleZ = (PlayableHeight + NavMeshCeilingPadding) / 200.0f;

		//nav mesh Z centre: exact middle between the lowest floor and the highest wall
		float CenterZ = BlockLocZ + (PlayableHeight / 2.0f);

		FVector NavCenter = FVector(CenterX, CenterY, CenterZ) + GetActorLocation();

		NavVolume->SetActorLocation(NavCenter);
		NavVolume->SetActorScale3D(FVector(ScaleX, ScaleY, ScaleZ));
	}


	//generate flat grid:
	for(int32 X = 0; X < GridSizeX; ++X)
	{
		for(int32 Y = 0; Y < GridSizeY; ++Y)
		{
			FVector Location = FVector(X * BlockSize, Y * BlockSize, BlockLocZ) + GetActorLocation();
			FVector BlockScale = FVector(BlockSize/MeshSizeX, BlockSize/MeshSizeY, TotalPillarHeight/MeshSizeZ);

			FTransform BlockTransform(FRotator::ZeroRotator, Location, BlockScale);

			GridMesh->AddInstance(BlockTransform);

			//save it to InstanceTransforms array:
			int32 Index = (X * GridSizeY) + Y;
			InstanceTransforms[Index] = BlockTransform;
		}
	}
}

// Called every frame
void AArenaManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if(bIsTransitioning)
	{
		bool bStillMoving = false;
		
		//smoothly transition current heights to target heights
		for(int32 i = 0; i < CurrentHeights.Num(); ++i)
		{
			CurrentHeights[i] = FMath::FInterpTo(CurrentHeights[i], TargetHeights[i], DeltaTime, TransitionSpeed);

			//update the stored instance transforms:
			FVector CurrentLoc = InstanceTransforms[i].GetLocation();
			CurrentLoc.Z = CurrentHeights[i];
			InstanceTransforms[i].SetLocation(CurrentLoc);

			//if target not reached yet:
			if(!FMath::IsNearlyEqual(CurrentHeights[i], TargetHeights[i], 1.0f)) bStillMoving = true;
		}

		//give the data to GPU to handle:
		//params: (start index, array of Transforms, Rebuild Navigation?)
		GridMesh->BatchUpdateInstancesTransforms(0, InstanceTransforms, true, true);
	
		//if target height reached by all blocks:
		if(!bStillMoving) bIsTransitioning = false;
	}
}

//Procedural generation:
void AArenaManager::GenerateNewLayout()
{
	float MaxBlockHeight = BlockLocZ + (MaxStairSteps * BlockSize);

	for(int32 X = 0; X < GridSizeX; ++X)
	{
		for(int32 Y = 0; Y < GridSizeY; ++Y)
		{
			int32 Index = (X * GridSizeY) + Y;

			//make the walls tall:
			if(X == 0 || X == GridSizeX - 1 || Y == 0 || Y == GridSizeY - 1)
			{
				TargetHeights[Index] = MaxBlockHeight;
			}
			else //pick a random height in increments of blocksize
			{
				int32 HeightMultiplier = FMath::RandRange(0, MaxStairSteps);
				TargetHeights[Index] = BlockLocZ + (BlockSize * HeightMultiplier);
			}
		}
	}

	bIsTransitioning = true; //tell tick to start moving blocks
}

