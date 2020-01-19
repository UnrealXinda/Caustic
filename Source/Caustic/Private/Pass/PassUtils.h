// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CausticTypes.h"
#include "UObject/ObjectMacros.h"
#include "RHI/Public/RHIResources.h"
#include "RHI/Public/RHICommandList.h"

#define SafeReleaseTextureResource(Texture)  \
	do {                                     \
		if (Texture.IsValid()) {             \
			Texture->Release();              \
		}                                    \
	} while(0);

namespace Caustic
{
	inline FRHITexture* GetRHITextureFromRenderTarget(const UTextureRenderTarget2D* RenderTarget)
	{
		if (RenderTarget)
		{
			FTextureReferenceRHIRef OutputRenderTargetTextureRHI = RenderTarget->TextureReference.TextureReferenceRHI;

			checkf(OutputRenderTargetTextureRHI, TEXT("Can't get render target %d texture"));

			FRHITexture* RenderTargetTextureRef = OutputRenderTargetTextureRHI->GetTextureReference()->GetReferencedTexture();

			return RenderTargetTextureRef;
		}

		return nullptr;
	}
}