// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CausticTypes.generated.h"

USTRUCT(BlueprintType)
struct CAUSTIC_API FLiquidParam
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0.01))
	float Velocity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0.01))
	float Viscosity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ForceFactor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Refraction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0.0, ClampMax = 1.0))
	float AttenuationCoefficient;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 DepthTextureWidth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 DepthTextureHeight;
};