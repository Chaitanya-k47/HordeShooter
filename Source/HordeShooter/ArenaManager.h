// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ArenaManager.generated.h"

class UInstancedStaticMeshComponent;

UENUM()
enum class EArenaTransitionState : uint8
{
	Idle,
	Flattening,
	Rising
};

UCLASS()
class HORDESHOOTER_API AArenaManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AArenaManager();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	//procedural generation:
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Arena")
	void BeginNewLayoutGeneration(); //starts flattening the current layout.

	UFUNCTION(BlueprintCallable, Category = "Arena")
	FTransform GetRandomSpawnPoint() const;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	//ARENA CONFIG:
	UPROPERTY(VisibleAnywhere, Category = "Arena")
	UInstancedStaticMeshComponent* GridMesh;

	UPROPERTY(VisibleAnywhere, Category = "Arena")
	UInstancedStaticMeshComponent* RampMesh;


	UPROPERTY(EditAnywhere, Category = "Arena Config")
	int32 GridSizeX = 20; //20 bloacks wide.

	UPROPERTY(EditAnywhere, Category = "Arena Config")
	int32 GridSizeY = 20; //20 bloacks long.

	UPROPERTY(EditAnywhere, Category = "Arena Config")
	float BlockSize = 400.f; // a block of 4x4x4 meter cube.

	UPROPERTY(EditAnywhere, Category = "Arena Config")
	float TransitionSpeed = 2.f; //speed of movement of blocks

	UPROPERTY(EditAnywhere, Category = "Arena Config")
	float BlockLocZ = -2000.f; //how deepdown the blocks will be spawned below origin when initializing the grid

	//how many stair Steps high can the arena go
	UPROPERTY(EditAnywhere, Category = "Arena Config")
	int32 MaxStairSteps = 3; 

	//how high above the grid should the player spawn
	UPROPERTY(EditAnywhere, Category = "Arena Config")
	float PlayerSpawnHeight = 1000.0f;

	//how high the NavMesh should extend above the tallest wall to allow jumping/spawning
	UPROPERTY(EditAnywhere, Category = "Arena Config")
	float NavMeshCeilingPadding = 1000.0f;

	//PROCEDURAL NOISE SETTING:
	UPROPERTY(EditAnywhere, Category = "Arena Config|Procedural")
	float NoiseFrequency = 0.1f; //lower=wider hills, higher=choppy terrain

	UPROPERTY(EditAnywhere, Category = "Arena Config|Procedural")
	bool bUseSymmetry = true; //mirrors the arena
	
	UPROPERTY(EditAnywhere, Category = "Arena Config|Procedural")
	bool bEnforceAccessibility = true; //smooths steep cliffs so ramps can always connect

private:
	//tracks current and target height of every single block:
	TArray<float> CurrentHeights;
	TArray<float> TargetHeights;

	//tracks current and target height of every ramp:
	TArray<FTransform> CurrentRampTransforms;
	TArray<FTransform> TargetRampTransforms;

	//batch update the transforms of block instances instead of updating them 1by1.
	TArray<FTransform> InstanceTransforms;

	//helper for ramp spawning
	void SpawnRampTarget(int32 X, int32 Y, float BaseZ, float YawRotation);

	// --- RAMP/CUBE MATH CACHE ---
	FVector CachedRampScale;
	FVector CachedRampLocalCenter; 
	float CachedRampMinZ;

	FVector CachedCubeLocalCenter;
	float CachedCubeMaxZ;

	//block mesh dimensions default:
	float MeshSizeX = 100.f;
	float MeshSizeY = 100.f;
	float MeshSizeZ = 100.f;

	//ramp mesh dimensions default:
	float RampMeshX = 100.0f;
	float RampMeshY = 100.0f;
	float RampMeshZ = 100.0f;

	//tracks which grid cell has got a ramp
	TArray<bool> BlockHasRamp;

	//stores perfect centre for safe flat block for spawning:
	TArray<FTransform> ValidSpawnPoints;

	//Arena state tracker:
	EArenaTransitionState TransitionState = EArenaTransitionState::Idle;

	//generates new layout and starts transitioning to it.
	void GenerateAndRiseNewLayout();
};
