// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CausticBody.generated.h"

UCLASS()
class CAUSTIC_API ACausticBody : public AActor
{
	GENERATED_BODY()
	
public:	

	ACausticBody();

	virtual void Tick(float DeltaTime) override;

protected:	

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Components, meta = (ClampMin = 0.01))
	float CellSize;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Components, meta = (ClampMin = 0.0))
	float BodyWidth;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Components, meta = (ClampMin = 0.0))
	float BodyHeight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Components, meta = (ClampMin = 0.0))
	float BodyDepth;

	/** The water surface mesh component */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Components)
	class UProceduralMeshComponent* SurfaceComp;

	/** The water body mesh component */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Components)
	class UProceduralMeshComponent* BodyComp;

protected:

	UFUNCTION(BlueprintCallable)
	void GenerateSurfaceMesh();

	UFUNCTION(BlueprintCallable)
	void GenerateBodyMesh();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
};
