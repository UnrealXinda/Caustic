// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "RHI/Public/RHIResources.h"
#include "RHI/Public/RHICommandList.h"

struct FSurfaceCausticPassConfig
{
	uint32 TextureWidth;
	uint32 TextureHeight;
	uint32 CellSize;
	float  FarClipZ;
	float  NearClipZ;
};

class FSurfaceCausticPassRenderer
{

public:

	FSurfaceCausticPassRenderer();
	~FSurfaceCausticPassRenderer();

	void InitPass(const FSurfaceCausticPassConfig& InConfig);

	void Render(const FLiquidParam& LiquidParam, FShaderResourceViewRHIRef HeightTextureSRV, UTextureRenderTarget2D* RenderTarget);

	bool IsValidPass() const;

private:

	FSurfaceCausticPassConfig         Config;
	bool                              bInitiated;

private:

	FMatrix GetMVPMatrix() const;
};
