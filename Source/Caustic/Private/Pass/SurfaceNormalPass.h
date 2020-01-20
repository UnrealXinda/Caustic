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

	void Render(FShaderResourceViewRHIRef HeightTextureSRV);

	bool IsValidPass() const;

	FORCEINLINE FShaderResourceViewRHIRef GetNormalTextureSRV() const { return OutputNormalTextureSRV; }

private:

	FTexture2DRHIRef           OutputNormalTexture;
	FUnorderedAccessViewRHIRef OutputNormalTextureUAV;
	FShaderResourceViewRHIRef  OutputNormalTextureSRV;

	FRHITexture*               NormalDebugTextureRHIRef;

	FSurfaceNormalPassConfig   Config;
	bool                       bInitiated;
};
