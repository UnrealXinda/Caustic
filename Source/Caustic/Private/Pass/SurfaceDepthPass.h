// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CausticTypes.h"
#include "UObject/ObjectMacros.h"
#include "RHI/Public/RHIResources.h"
#include "RHI/Public/RHICommandList.h"

struct FSurfaceDepthPassConfig
{
	float                     MinDepth;
	float                     MaxDepth;
	uint32                    TextureWidth;
	uint32                    TextureHeight;
	UTextureRenderTarget2D*   DepthDebugTextureRef;
	UTextureRenderTarget2D*   HeightDebugTextureRef;
};

class FSurfaceDepthPassRenderer
{

public:
	
	FSurfaceDepthPassRenderer();
	~FSurfaceDepthPassRenderer();

	void InitPass(const FSurfaceDepthPassConfig& InConfig);

	void Render(const FLiquidParam& LiquidParam, class FRHITexture* DepthTextureRef);

	bool IsValidPass() const;

	FORCEINLINE FShaderResourceViewRHIRef GetDepthTextureSRV() const { return OutputDepthTextureSRV; }

	FORCEINLINE FShaderResourceViewRHIRef GetHeightTextureSRV() const { return OutputHeightTextureSRV; }

private:

	FTexture2DRHIRef           OutputDepthTexture;
	FUnorderedAccessViewRHIRef OutputDepthTextureUAV;
	FShaderResourceViewRHIRef  OutputDepthTextureSRV;

	FTexture2DRHIRef           OutputHeightTexture;
	FUnorderedAccessViewRHIRef OutputHeightTextureUAV;
	FShaderResourceViewRHIRef  OutputHeightTextureSRV;

	FTexture2DRHIRef           InputDepthTexture;
	FShaderResourceViewRHIRef  InputDepthTextureSRV;

	FTexture2DRHIRef           PrevDepthTexture;
	FShaderResourceViewRHIRef  PrevDepthTextureSRV;

	FRHITexture*               DepthDebugTextureRHIRef;
	FRHITexture*               HeightDebugTextureRHIRef;

	FSurfaceDepthPassConfig    Config;

	bool                       bInitiated;

private:

	void RenderSurfaceDepthPass(FRHICommandListImmediate& RHICmdList, const FLiquidParam& LiquidParam, class FRHITexture* DepthTextureRef);
	void RenderSurfaceHeightPass(FRHICommandListImmediate& RHICmdList, const FLiquidParam& LiquidParam);

	FVector4 EncodeLiquidParam(const FLiquidParam& LiquidParam) const;
};
