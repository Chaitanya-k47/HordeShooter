// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HordeShooterCasing.generated.h"

class UStaticMeshComponent;
class USoundBase;

UCLASS()
class HORDESHOOTER_API AHordeShooterCasing : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AHordeShooterCasing();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void ActivateAndEject(const FTransform& StartTransform, FVector ShooterVelocity);

	void DeactivateCasing();


protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* CasingMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	USoundBase* ShellBounceSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	float ShellEjectionImpulse = 155.f;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnHit(
		UPrimitiveComponent* HitComp, //self component that was hit
		AActor* OtherActor, //other actor that was involved in the hit
		UPrimitiveComponent* OtherComp, //other actor's component that was involved in the hit
		FVector NormalImpulse, //the impulse applied to the hit component as a result of the hit
		const FHitResult& Hit
	);

	
private:
	bool bHasBounced = false;
	FTimerHandle DeactivateTimerHandle; //timer to return to the pool
	
};
