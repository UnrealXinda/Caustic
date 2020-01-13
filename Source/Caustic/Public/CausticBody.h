// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CausticTypes.h"
#include "GameFramework/Actor.h"
#include "Pass/SurfaceDepthPass.h"
#include "CausticBody.generated.h"

UCLASS()
class CAUSTIC_API ACausticBody : public AActor
{
	GENERATED_BODY()
	
public:	

	ACausticBody();

	virtual void Tick(float DeltaTime) override;

protected:	

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Caustic Body", meta = (ClampMin = 0.01))
	float CellSize;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Caustic Body", meta = (ClampMin = 0.0))
	float BodyWidth;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Caustic Body", meta = (ClampMin = 0.0))
	float BodyHeight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Caustic Body", meta = (ClampMin = 0.0))
	float BodyDepth;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Caustic Body")
	FLiquidParam LiquidParam;

	/** The water surface mesh component */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Components)
	class UBoxComponent* BoxCollisionComp;

	/** The water surface mesh component */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Components)
	class UProceduralMeshComponent* SurfaceMeshComp;

	/** The water body mesh component */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Components)
	class UProceduralMeshComponent* BodyMeshComp;

	/** Scene capture component that captures depth texture */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Components)
	class USceneCaptureComponent2D* DepthCaptureComp;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pass Debug Textures")
	class UTextureRenderTarget2D* SurfaceDepthPassDebugTexture;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pass Debug Textures")
	class UTextureRenderTarget2D* SurfaceHeightPassDebugTexture;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Pass Debug Textures")
	class UTextureRenderTarget2D* DepthRenderTarget;

	TUniquePtr<FSurfaceDepthPassRenderer> SurfaceDepthPassRenderer;

protected:

	UFUNCTION(BlueprintCallable)
	void GenerateSurfaceMesh();

	UFUNCTION(BlueprintCallable)
	void GenerateBodyMesh();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
};
