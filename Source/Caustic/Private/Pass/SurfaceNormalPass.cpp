// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "SurfaceNormalPass.h"
#include "RenderCore/Public/GlobalShader.h"
#include "RenderCore/Public/ShaderParameterUtils.h"
#include "RenderCore/Public/ShaderParameterMacros.h"

#include "Public/GlobalShader.h"
#include "Public/PipelineStateCache.h"
#include "Public/RHIStaticStates.h"
#include "Public/SceneUtils.h"
#include "Public/SceneInterface.h"
#include "Public/ShaderParameterUtils.h"
#include "Public/Logging/MessageLog.h"
#include "Public/Internationalization/Internationalization.h"
#include "Public/StaticBoundShaderState.h"
#include "RHI/Public/RHICommandList.h"
#include "Pass/PassUtils.h"

class FSurfaceNormalComputeShader : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FSurfaceNormalComputeShader);

public:

	FSurfaceNormalComputeShader() {}
	FSurfaceNormalComputeShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		InputHeightTexture.Bind(Initializer.ParameterMap, TEXT("InputHeightTexture"));
		OutputNormalTexture.Bind(Initializer.ParameterMap, TEXT("OutputNormalTexture"));
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	void BindShaderTextures(FRHICommandList& RHICmdList, FUnorderedAccessViewRHIRef OutputTextureUAV, FShaderResourceViewRHIRef InputTextureSRV)
	{
		FRHIComputeShader* ComputeShaderRHI = GetComputeShader();

		if (InputHeightTexture.IsBound())
		{
			RHICmdList.SetShaderResourceViewParameter(ComputeShaderRHI, InputHeightTexture.GetBaseIndex(), InputTextureSRV);
		}

		if (OutputNormalTexture.IsBound())
		{
			RHICmdList.SetUAVParameter(ComputeShaderRHI, OutputNormalTexture.GetBaseIndex(), OutputTextureUAV);
		}
	}

private:

	FShaderResourceParameter InputHeightTexture;
	FShaderResourceParameter OutputNormalTexture;
};

IMPLEMENT_SHADER_TYPE(, FSurfaceNormalComputeShader, TEXT("/Plugin/Caustic/SurfaceNormalComputeShader.usf"), TEXT("ComputeSurfaceNormal"), SF_Compute);

FSurfaceNormalPassRenderer::FSurfaceNormalPassRenderer() :
	bInitiated(false)
{

}

FSurfaceNormalPassRenderer::~FSurfaceNormalPassRenderer()
{
	SafeReleaseTextureResource(OutputNormalTexture);
	SafeReleaseTextureResource(OutputNormalTextureSRV);
	SafeReleaseTextureResource(OutputNormalTextureUAV);
}

void FSurfaceNormalPassRenderer::InitPass(const FSurfaceNormalPassConfig& InConfig)
{
	if (!bInitiated)
	{
		FRHIResourceCreateInfo CreateInfo;
		uint32 TextureWidth = InConfig.TextureWidth;
		uint32 TextureHeight = InConfig.TextureHeight;

		OutputNormalTexture = RHICreateTexture2D(TextureWidth, TextureHeight, PF_FloatRGBA, 1, 1, TexCreate_ShaderResource | TexCreate_UAV, CreateInfo);
		OutputNormalTextureUAV = RHICreateUnorderedAccessView(OutputNormalTexture);
		OutputNormalTextureSRV = RHICreateShaderResourceView(OutputNormalTexture, 0);

		InputHeightTexture = RHICreateTexture2D(TextureWidth, TextureHeight, PF_FloatRGBA, 1, 1, TexCreate_ShaderResource, CreateInfo);
		InputHeightTextureSRV = RHICreateShaderResourceView(InputHeightTexture, 0);

		NormalDebugTextureRHIRef = Caustic::GetRHITextureFromRenderTarget(InConfig.NormalDebugTextureRef);

		Config = InConfig;

		bInitiated = true;
	}
}

//void FSurfaceNormalPassRenderer::Render(FShaderResourceViewRHIRef HeightTextureSRV)
void FSurfaceNormalPassRenderer::Render(FTexture2DRHIRef HeightTextureRef)
{
	if (IsValidPass())
	{
		ENQUEUE_RENDER_COMMAND(SurfaceNormalPass)
		(
			[HeightTextureRef, this](FRHICommandListImmediate& RHICmdList)
			{
				check(IsInRenderingThread());

				// Copy height texture
				FRHICopyTextureInfo CopyInfo;
				RHICmdList.CopyTexture(HeightTextureRef, InputHeightTexture, CopyInfo);

				// Bind shader textures
				TShaderMapRef<FSurfaceNormalComputeShader> SurfacsNormalComputeShader(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
				RHICmdList.SetComputeShader(SurfacsNormalComputeShader->GetComputeShader());
				SurfacsNormalComputeShader->BindShaderTextures(RHICmdList, OutputNormalTextureUAV, InputHeightTextureSRV);

				// Dispatch shader
				const int ThreadGroupCountX = StaticCast<int>(Config.TextureWidth / 32);
				const int ThreadGroupCountY = StaticCast<int>(Config.TextureHeight / 32);
				DispatchComputeShader(RHICmdList, *SurfacsNormalComputeShader, ThreadGroupCountX, ThreadGroupCountY, 1);

				// Debug drawing
				if (NormalDebugTextureRHIRef)
				{
					RHICmdList.CopyToResolveTarget(OutputNormalTexture, NormalDebugTextureRHIRef, FResolveParams());
				}
			}
		);
	}
}

bool FSurfaceNormalPassRenderer::IsValidPass() const
{
	bool bValid = !!OutputNormalTexture;
	bValid &= !!OutputNormalTextureSRV;

	return bValid;
}

