// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "RHI/Public/RHIResources.h"
#include "RHI/Public/RHICommandList.h"

struct FSurfaceNormalPassConfig
{
	uint32                    TextureWidth;
	uint32                    TextureHeight;
	UTextureRenderTarget2D*   NormalDebugTextureRef;
};

class FSurfaceNormalPassRenderer
{

public:

	FSurfaceNormalPassRenderer();
	~FSurfaceNormalPassRenderer();

	void InitPass(const FSurfaceNormalPassConfig& InConfig);

	//void Render(FShaderResourceViewRHIRef HeightTextureSRV);
	void Render(FTexture2DRHIRef HeightTextureRef);

	bool IsValidPass() const;

private:

	FTexture2DRHIRef           OutputNormalTexture;
	FUnorderedAccessViewRHIRef OutputNormalTextureUAV;
	FShaderResourceViewRHIRef  OutputNormalTextureSRV;

	FTexture2DRHIRef           InputHeightTexture;
	FShaderResourceViewRHIRef  InputHeightTextureSRV;

	FRHITexture*               NormalDebugTextureRHIRef;

	FSurfaceNormalPassConfig   Config;
	bool                       bInitiated;
};
