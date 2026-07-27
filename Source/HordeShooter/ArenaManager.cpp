// Fill out your copyright notice in the Description page of Project Settings.


#include "ArenaManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "NavigationSystem.h"
#include "GameFramework/Character.h"

// Sets default values
AArenaManager::AArenaManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	GridMesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("GridMesh"));
	RootComponent = GridMesh;

	RampMesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("RampMesh"));
	RampMesh->SetupAttachment(RootComponent);

	//allows collision to update when instances move
	GridMesh->bUseDefaultCollision = true;
	RampMesh->bUseDefaultCollision = true;
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

	//block dimensions:
	if(GridMesh->GetStaticMesh())
	{
		FBox CubeBox = GridMesh->GetStaticMesh()->GetBoundingBox();
		MeshSizeX = CubeBox.GetSize().X;
		MeshSizeY = CubeBox.GetSize().Y;
		MeshSizeZ = CubeBox.GetSize().Z;
		
		//max Z coordinate on the unscaled cube.
		CachedCubeMaxZ = CubeBox.Max.Z;
		CachedCubeLocalCenter = CubeBox.GetCenter();
	}

	//ramp dimensions:
	if (RampMesh->GetStaticMesh())
	{
		FBox RBox = RampMesh->GetStaticMesh()->GetBoundingBox();
		RampMeshX = RBox.GetSize().X;
		RampMeshY = RBox.GetSize().Y;
		RampMeshZ = RBox.GetSize().Z;

		CachedRampLocalCenter = RBox.GetCenter(); 
		CachedRampMinZ = RBox.Min.Z;
	}

	//calculate max wall height:
	float MaxBlockHeight = BlockLocZ + (MaxStairSteps * BlockSize); //the maximum height a block will ever reach from the spawning location BlockLocZ.
	
	//this block is to be scaled along Z, hence the dimension in Z to be scaled upto:
	float TotalPillarHeight = (MaxStairSteps * BlockSize) + BlockSize; //adding a block more to the height for safety.

	//calculate the factor to scale the ramp to the size of block
	CachedRampScale = FVector((BlockSize/RampMeshX), (BlockSize/RampMeshY), BlockSize/RampMeshZ);

	//calculate absolute centre of the grid:
	float CenterX = ((GridSizeX - 1) * BlockSize)/2.0f;
	float CenterY = ((GridSizeY - 1) * BlockSize)/2.0f;
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

	//animating pillars and ramps:
	if(bIsTransitioning)
	{
		bool bStillMoving = false;
		
		//PILLARS:
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
	
		//RAMPS:
		for (int32 i = 0; i < CurrentRampTransforms.Num(); ++i)
		{
			FVector CurrentLoc = CurrentRampTransforms[i].GetLocation();
			FVector TargetLoc = TargetRampTransforms[i].GetLocation();

			// Smoothly slide the ramps up!
			CurrentLoc.Z = FMath::FInterpTo(CurrentLoc.Z, TargetLoc.Z, DeltaTime, TransitionSpeed);
			CurrentRampTransforms[i].SetLocation(CurrentLoc);

			if(!FMath::IsNearlyEqual(CurrentLoc.Z, TargetLoc.Z, 1.0f)) bStillMoving = true;
		}

		//PUSH THE DATA TO THE GPU
		//params: (start index, array of Transforms, Rebuild Navigation?)
		GridMesh->BatchUpdateInstancesTransforms(0, InstanceTransforms, true, true);

		if (CurrentRampTransforms.Num() > 0)
		{
			RampMesh->BatchUpdateInstancesTransforms(0, CurrentRampTransforms, true, true);
		}

		//if target height reached by all blocks:
		if(!bStillMoving)
		{
			bIsTransitioning = false;

			//arena geometry is ready, tell the nav mesh to beging building:
			//safely release the lock and force the rebuild instantly
			if (UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld()))
			{
				NavSys->RemoveNavigationBuildLock(
					ENavigationBuildLock::Custom, 
					UNavigationSystemV1::ELockRemovalRebuildAction::Rebuild
				); 
			}

		}
	}
}

//Procedural generation:
void AArenaManager::GenerateNewLayout()
{
	RampMesh->ClearInstances();
	CurrentRampTransforms.Empty();
	TargetRampTransforms.Empty();

	//initialize trackers:
	ValidSpawnPoints.Empty();
	BlockHasRamp.Init(false, GridSizeX * GridSizeY);

	//pick a random seed offset for the noise so every generated layout is unique:
	FVector2D NoiseOffset = FVector2D(FMath::RandRange(-10000.f, 10000.f), FMath::RandRange(-10000.f, 10000.f));

	//centre index of grid:
	int32 CenterX = GridSizeX/2;
	int32 CenterY = GridSizeY/2;

	//GENERATE PERLIN NOISE ARENA:
	for(int32 X = 0; X < GridSizeX; ++X)
	{
		for(int32 Y = 0; Y < GridSizeY; ++Y)
		{
			int32 Index = (X * GridSizeY) + Y;

			//symmetry rule:
			/*
				ex. for GridSize = 8
				Sample[0] == Sample[7]
				Sample[1] == Sample[6]
				Sample[2] == Sample[5]
				Sample[3] == Sample[4]
				Sample[i] == Sample[GridSize-1-i]

				bilateral condition:
					if(i >= Centre) mirror i.e. copy from (GridSize-1-i)
					else dont mirror and keep it i

				ex. i = 0, 1, 2, 3, 4, 5, 6, 7
					Centre = GridSize/2 = 4
					hence the samples become: 0, 1, 2, 3 ,3, 2, 3, 0
			*/
			int32 SampleX = X;
			int32 SampleY = Y;
			if(bUseSymmetry)
			{
				SampleX = (X >= CenterX) ? (GridSizeX-1-X) : X;
				SampleY = (Y >= CenterY) ? (GridSizeY-1-Y) : Y;
			}

			//Generate perlin noise:
			/*
				SampleX, SampleY: which tile of arena is being generated.
				NoiseFrequency: how "zoomed in" or "zoomed out" the terrain features are (large sweeping hills vs. choppy terrain), basically the sample scale.
				NoiseOffset: where arena is cut from the infinite Perlin noise landscape, giving a different layout each generation
				PerlinNoise2D(): samples that infinite smooth field and returns a value between approximately -1 and +1
			*/
			float RawNoise = FMath::PerlinNoise2D(
				FVector2D(
					SampleX * NoiseFrequency + NoiseOffset.X,
					SampleY * NoiseFrequency + NoiseOffset.Y
				)
			);

			//Normalize the noise between 0 and 1.
			/*
				formula: for [-1, 1] to [0, 1]
				normalized noise = (Raw - RawMin)/(RawMax - RawMin)
			*/
			float NormalizedNoise = (RawNoise + 1.f) / 2.f;

			//threshold: if its too low make the height zero i.e. flat ground
			if(NormalizedNoise < 0.2f) NormalizedNoise = 0.f;

			//quantization: turn smooth gradients into sharp blocky transition
			int32 HeightMultiplier = FMath::RoundToInt(NormalizedNoise * MaxStairSteps);
			HeightMultiplier = FMath::Clamp(HeightMultiplier, 0, MaxStairSteps);

			TargetHeights[Index] = BlockLocZ + (BlockSize * HeightMultiplier);
		}
	}

	//ACCESSIBILITY PASS:
	//if a cliff is a 2 step drop, the player cant jump it and ramps wont spawn
	//this loop pulls steep cliffs down so they gently slope, guaranteeing ramps can connect everywhere
	if(bEnforceAccessibility)
	{
		//the while loop iterates through the whole grid in multiple passes till the accessibility is enforced everywhere.

		bool bNeedsSmoothing = true;
		while(bNeedsSmoothing)
		{
			bNeedsSmoothing = false;

			//now loop through the ENTIRE grid, including the edges
			for (int32 X = 0; X < GridSizeX; ++X)
			{
				for (int32 Y = 0; Y < GridSizeY; ++Y)
				{
					int32 Index = (X * GridSizeY) + Y;
					float CurrentH = TargetHeights[Index];

					//safely collect valid neighbors(prevent checking outside array bounds)
					TArray<int32> Neighbors;
					if (X + 1 < GridSizeX)
						Neighbors.Add(((X + 1) * GridSizeY) + Y); //forward

					if (X - 1 >= 0)
						Neighbors.Add(((X - 1) * GridSizeY) + Y); //backward

					if (Y + 1 < GridSizeY)
						Neighbors.Add((X * GridSizeY) + (Y + 1)); //right

					if (Y - 1 >= 0)
						Neighbors.Add((X * GridSizeY) + (Y - 1)); // Left

					for (int32 N_Index : Neighbors)
					{
						//if this block is more than 1 step higher than its neighbor pull it down.
						if (CurrentH > TargetHeights[N_Index] + BlockSize)
						{
							TargetHeights[Index] = TargetHeights[N_Index] + BlockSize;
							bNeedsSmoothing = true;
						}
					}
				}
			}
		}
	}

	//SPAWN RAMPS:
	for(int32 X = 0; X < GridSizeX; ++X)
	{
		for(int32 Y = 0; Y < GridSizeY; ++Y)
		{
			int32 CurrentIndex = (X * GridSizeY) + Y;
			float CurrentH = TargetHeights[CurrentIndex]; //TargetHeights stores the final height of the block

			//spawn ramp on top of current block only when the neighbour is one block higher
			float RampTargetHeight = CurrentH + BlockSize;

			//CHECK NEIGHBORS:
			//check forward(+X)
			if((X + 1 < GridSizeX) && TargetHeights[((X + 1) * GridSizeY) + Y] == RampTargetHeight)
			{
				SpawnRampTarget(X, Y, CurrentH, 0.0f+90); //0 degrees = facing +X
				BlockHasRamp[CurrentIndex] = true;
			}

			//check backward(-X)
			else if ((X - 1 >= 0) && TargetHeights[((X - 1) * GridSizeY) + Y] == RampTargetHeight)
			{
				SpawnRampTarget(X, Y, CurrentH, 180.0f+90);
				BlockHasRamp[CurrentIndex] = true;
			}

			//check right(+Y)
			else if((Y + 1 < GridSizeY) && TargetHeights[(X * GridSizeY) + (Y + 1)] == RampTargetHeight)
			{
				SpawnRampTarget(X, Y, CurrentH, 90.0f+90);
				BlockHasRamp[CurrentIndex] = true;
			}

			//check left(-Y)
			else if((Y - 1 >= 0) && TargetHeights[(X * GridSizeY) + (Y - 1)] == RampTargetHeight)
			{
				SpawnRampTarget(X, Y, CurrentH, -90.0f+90);
				BlockHasRamp[CurrentIndex] = true;
			}
		}
	}

	//CACHE VALID SPAWN POINTS:
	for (int32 X = 0; X < GridSizeX; ++X)
	{
		for (int32 Y = 0; Y < GridSizeY; ++Y)
		{
			int32 Index = (X * GridSizeY) + Y;

			if(!BlockHasRamp[Index])
			{
				float BaseZ = TargetHeights[Index];
				float CubeScaleZ = ((MaxStairSteps * BlockSize) + BlockSize) / MeshSizeZ;
				float RoofZ = BaseZ + (CachedCubeMaxZ * CubeScaleZ) + GetActorLocation().Z;

				FVector CubePivotXY = FVector(X * BlockSize, Y * BlockSize, 0.0f) + GetActorLocation();
				FVector CubeScaleXY = FVector(BlockSize / MeshSizeX, BlockSize / MeshSizeY, 1.0f);
				FVector ScaledCubeOffset = CachedCubeLocalCenter * CubeScaleXY;
				FVector TrueCellCenterXY = CubePivotXY + FVector(ScaledCubeOffset.X, ScaledCubeOffset.Y, 0.0f);

				//exact centre of a grid cell, 100 units above floor
				FVector SpawnLoc = FVector(TrueCellCenterXY.X, TrueCellCenterXY.Y, RoofZ + 100.0f);
				
				//randomize yaw
				FRotator SpawnRot = FRotator(0.0f, FMath::RandRange(0.0f, 360.0f), 0.0f);

				ValidSpawnPoints.Add(FTransform(SpawnRot, SpawnLoc, FVector(1.0f)));
			}
		}
	}

	//LOCK THE NAV MESH:
	//temporarily tell the engine lock navmesh building:
	if (UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld()))
	{
		NavSys->AddNavigationBuildLock(ENavigationBuildLock::Custom);
	}

	bIsTransitioning = true;//tell tick to start moving blocks
}

void AArenaManager::SpawnRampTarget(int32 X, int32 Y, float BaseZ, float YawRotation)
{
	FRotator RampRot = FRotator(0.0f, YawRotation, 0.0f);

	//the cube pivot's placement in the world
	FVector CubePivotXY = FVector(X * BlockSize, Y * BlockSize, 0.0f) + GetActorLocation();

	//the TRUE physical center of the cube by applying its scaled offset
	FVector CubeScaleXY = FVector(BlockSize/MeshSizeX, BlockSize/MeshSizeY, 1.0f);
	FVector ScaledCubeOffset = CachedCubeLocalCenter * CubeScaleXY;

	//the actual deadcenter of the physical block geometry, no matter where the pivot is
	//i.e. cube pivot added to its geometric centre.
	FVector TrueCellCenterXY = CubePivotXY + FVector(ScaledCubeOffset.X, ScaledCubeOffset.Y, 0.0f);

	//the exact top of the pillar 
	//BaseZ = CurrentH = TargetHeights[Current block Index] i.e. the final height of the local centre of the cube/block/pillar
	//CubeScaleZ = Scale of the cube/pillar along Z axis
	//(CachedCubeMaxZ * CubeScaleZ) => returns the max Z coordinate of the scaled cube/pillar by multiplying original max Z with current cube scale in Z.
	//i.e. Z coordinate of scaled cube roof relative to the current cube centre BaseZ.
	//then we get the global Z coordinate of cube/pillar roof by adding BaseZ(local origin/centre of pillar relative to level origin i.e. the global coordinate) to (CachedCubeMaxZ * CubeScaleZ).
	float CubeScaleZ = ((MaxStairSteps * BlockSize) + BlockSize) / MeshSizeZ; // TotalPillarHeight / DefaultMeshSize
	float RoofZ = BaseZ + (CachedCubeMaxZ * CubeScaleZ);

	//PIVOT CORRECTION MATH
	//find how far offcenter the pivot is, and scale it up to BlockSize
	FVector ScaledCenterOffset = CachedRampLocalCenter * CachedRampScale;
	
	//Rotate that offset so it matches the direction the ramp is facing
	FVector RotatedCenterOffset = RampRot.RotateVector(ScaledCenterOffset);

	//THE FINAL TARGET COORDINATES
	//force the XY center of the ramp to match the center of the grid cell
	float TargetX = TrueCellCenterXY.X - RotatedCenterOffset.X;
	float TargetY = TrueCellCenterXY.Y - RotatedCenterOffset.Y;

	//force the absolute lowest point of the ramp to sit flush on the RoofZ
	float TargetZ = RoofZ - (CachedRampMinZ * CachedRampScale.Z) + GetActorLocation().Z;

	FVector TargetLoc = FVector(TargetX, TargetY, TargetZ);
	
	//start it deep underground
	FVector StartLoc = FVector(TargetX, TargetY, BlockLocZ - 2000.0f) + GetActorLocation().Z;

	FTransform StartTransform(RampRot, StartLoc, CachedRampScale);
	FTransform TargetTransform(RampRot, TargetLoc, CachedRampScale);

	RampMesh->AddInstance(StartTransform);
	
	CurrentRampTransforms.Add(StartTransform);
	TargetRampTransforms.Add(TargetTransform);
}

FTransform AArenaManager::GetRandomSpawnPoint() const
{
	if(ValidSpawnPoints.Num() > 0)
	{
		int32 RandomIndex = FMath::RandRange(0, ValidSpawnPoints.Num() - 1);
		return ValidSpawnPoints[RandomIndex];
	}

	return FTransform::Identity;
}
