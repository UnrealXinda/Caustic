// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "RHI/Public/RHIResources.h"
#include "RHI/Public/RHICommandList.h"

struct FSurfaceDepthShaderParameters
{
	FMatrix Model;
	FMatrix View;
	FMatrix Projection;
	float   MinDepth;  // Near surface clip
	float   MaxDepth;  // Far surface clip
};

class FSurfaceDepthPassRenderer
{

	void Render(
		ERHIFeatureLevel::Type              FeatureLevel,
		FRHICommandListImmediate&           RHICmdList,
		FVector2D                           RenderTargetSize,
		const FMatrix&                      View,
		const FMatrix&                      Projection,
		const float                         MinDepth,

		TArray<class UPrimitiveComponent*>& Components
	);
};
