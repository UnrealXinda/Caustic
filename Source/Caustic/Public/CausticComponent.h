// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "CausticComponent.generated.h"

UCLASS(hidecategories = (Object, LOD, Physics, Collision), editinlinenew, meta = (BlueprintSpawnableComponent), ClassGroup = Rendering, DisplayName = "CausticComponent")
class CAUSTIC_API UCausticComponent : public UProceduralMeshComponent
{
	GENERATED_BODY()

public:	

	UCausticComponent(const FObjectInitializer& ObjectInitializer);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Components, meta = (ClampMin = 0.01))
	float CellSize;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Components, meta = (ClampMin = 0.0))
	float BodyWidth;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Components, meta = (ClampMin = 0.0))
	float BodyHeight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Components, meta = (ClampMin = 0.0))
	float BodyDepth;

protected:
	
	UFUNCTION(BlueprintCallable)
	void GenerateSurfaceMesh();

	UFUNCTION(BlueprintCallable)
	void GenerateBodyMesh();

	virtual void BeginPlay() override;

};
