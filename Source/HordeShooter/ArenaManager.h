// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ArenaManager.generated.h"

class UInstancedStaticMeshComponent;
class UBoxComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnArenaLayoutFinishedSignature);

UENUM()
enum class EArenaTransitionState : uint8
{
	Idle,
	Flattening,
	Rising,
	WaitingForNavMesh
};

USTRUCT(BlueprintType)
struct FArenaFloatRange
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Range")
    float Min;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Range")
    float Max;

	FArenaFloatRange() : Min(0.0f), Max(1.0f) {}
	FArenaFloatRange(float InMin, float InMax) : Min(InMin), Max(InMax) {}
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

	UPROPERTY(BlueprintAssignable, Category = "Arena|Events")
	FOnArenaLayoutFinishedSignature OnArenaLayoutFinished;

	UFUNCTION(BlueprintCallable, Category = "Arena|Hazards")
	void StartLightningWave(int32 NumStrikes = 10);

	void ExecuteLightningStrike(FVector StrikeLocation);

	UFUNCTION(BlueprintCallable, Category = "Arena|Hazards")
	void SetCombatActive(bool bIsActive);

	UPROPERTY(BlueprintReadOnly, Category = "Arena|Hazards")
	bool bIsCombatActive = false;


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	//ARENA CONFIG:
	UPROPERTY(VisibleAnywhere, Category = "Arena")
	UInstancedStaticMeshComponent* GridMesh;

	UPROPERTY(VisibleAnywhere, Category = "Arena")
	UInstancedStaticMeshComponent* RampMesh;

	UPROPERTY(VisibleAnywhere, Category = "Arena")
	UBoxComponent* KillVolume;

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


	//anti camping:
	UPROPERTY(EditAnywhere, Category = "Arena Config|Anti-Camping")
	float MaxCampTime = 5.f; //seconds before the lightning strikes.

	UPROPERTY(EditAnywhere, Category = "Arena Config|Anti-Camping")
	float LightningPlayerDamage = 20.0f; 

	UPROPERTY(EditAnywhere, Category = "Arena Config|Anti-Camping")
	float LightningEnemyDamage = 5.0f;

	UPROPERTY(EditAnywhere, Category = "Arena Config|Anti-Camping")
	float LightningRadius = 600.0f;

	UPROPERTY(EditAnywhere, Category = "Arena Config|Anti-Camping")
	float LightningImpulse = 30000.0f;

	UPROPERTY(EditAnywhere, Category = "Arena Config|Anti-Camping")
	float LightningSpawnOffset = 5000.f;

	UPROPERTY(EditAnywhere, Category = "Arena Config|Anti-Camping")
	FArenaFloatRange LightningSpawnAngleRange = FArenaFloatRange(-90.f, 90.f);

	UPROPERTY(EditAnywhere, Category = "Arena Config|Anti-Camping")
	FArenaFloatRange LightningSpawnRadiusRange = FArenaFloatRange(400.f, 1000.f);

	UPROPERTY(EditAnywhere, Category = "Arena Config|Anti-Camping")
	FArenaFloatRange ConsecutiveStrikeDelayRange = FArenaFloatRange(0.1f, 0.4f);

	UPROPERTY(EditDefaultsOnly, Category = "Arena Config|Anti-Camping")
	class UNiagaraSystem* LightningVFX;

	UPROPERTY(EditDefaultsOnly, Category = "Arena Config|Anti-Camping")
	class UNiagaraSystem* LightningImpactVFX;

	UPROPERTY(EditDefaultsOnly, Category = "Arena Config|Anti-Camping")
	class UNiagaraSystem* LightningTelegraphVFX;

	UPROPERTY(EditDefaultsOnly, Category = "Arena Config|Anti-Camping")
	class USoundBase* LightningSound;

	UPROPERTY(EditDefaultsOnly, Category = "Arena Config|Anti-Camping")
	TSubclassOf<class UCameraShakeBase> LightningCameraShake;

	UPROPERTY(EditAnywhere, Category = "Arena Config|Anti-Camping")
	float LightningShakeInnerRadius = 600.0f; //max shake when inside. keep this same as damage radius

	UPROPERTY(EditAnywhere, Category = "Arena Config|Anti-Camping")
	float LightningShakeOuterRadius = 4000.0f; //shake fades out to 0 if player is further than this.

	UPROPERTY(EditAnywhere, Category = "Arena Config|Anti-Camping")
	float TelegraphTime = 1.0f; //how long the warning stays before it strikes

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

	UFUNCTION()
	void OnKillVolumeOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	//track player's position.
	int32 LastPlayerBlockIndex = -1;
	float PlayerCampTimer = 0.f;

	//lightning tracker:
	int32 RemainingLightningStrikes = 0;
	FTimerHandle LightningWaveTimerHandle;

	//recursive fn that fires strikes.
	void DropNextLightningInWave();

};
