// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "RHI/Public/RHIResources.h"
#include "RHI/Public/RHICommandList.h"

struct FSurfaceDepthPassConfig
{
	float                     MinDepth;
	float                     MaxDepth;
	uint32                    TextureWidth;
	uint32                    TextureHeight;
	UTextureRenderTarget2D*   DebugTextureRef;
};

class FSurfaceDepthPassRenderer
{

public:
	
	FSurfaceDepthPassRenderer();
	~FSurfaceDepthPassRenderer();

	void InitPass(const FSurfaceDepthPassConfig& InConfig);

	void Render(class FRHITexture* DepthTextureRef);

	bool IsValidPass() const;

private:

	FTexture2DRHIRef           OutputDepthTexture;
	FUnorderedAccessViewRHIRef OutputDepthTextureUAV;
	FShaderResourceViewRHIRef  OutputDepthTextureSRV;

	FTexture2DRHIRef           InputDepthTexture;
	FShaderResourceViewRHIRef  InputDepthTextureSRV;

	FRHITexture*               DebugTextureRHIRef;

	FSurfaceDepthPassConfig    Config;

	bool                       bInitiated;
};
