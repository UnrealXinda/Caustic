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
		NormalPassSampler.Bind(Initializer.ParameterMap, TEXT("NormalPassSampler"));
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5);
	}

	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << InputHeightTexture << OutputNormalTexture;
		return bShaderHasOutdatedParameters;
	}

	void BindShaderTextures(FRHICommandList& RHICmdList, FUnorderedAccessViewRHIRef OutputTextureUAV, FShaderResourceViewRHIRef InputTextureSRV)
	{
		FRHIComputeShader* ComputeShaderRHI = GetComputeShader();
		FRHISamplerState* SamplerStateLinear = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();

		SetUAVParameter(RHICmdList, ComputeShaderRHI, OutputNormalTexture, OutputTextureUAV);
		SetSRVParameter(RHICmdList, ComputeShaderRHI, InputHeightTexture, InputTextureSRV);
		SetSamplerParameter(RHICmdList, ComputeShaderRHI, NormalPassSampler, SamplerStateLinear);
	}

	void UnbindShaderTextures(FRHICommandList& RHICmdList)
	{
		FRHIComputeShader* ComputeShaderRHI = GetComputeShader();

		SetUAVParameter(RHICmdList, ComputeShaderRHI, OutputNormalTexture, FUnorderedAccessViewRHIRef());
		SetSRVParameter(RHICmdList, ComputeShaderRHI, InputHeightTexture, FShaderResourceViewRHIRef());
	}

private:

	FShaderResourceParameter InputHeightTexture;
	FShaderResourceParameter OutputNormalTexture;
	FShaderResourceParameter NormalPassSampler;
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

		NormalDebugTextureRHIRef = Caustic::GetRHITextureFromRenderTarget(InConfig.NormalDebugTextureRef);

		Config = InConfig;

		bInitiated = true;
	}
}

void FSurfaceNormalPassRenderer::Render(FShaderResourceViewRHIRef HeightTextureSRV)
{
	if (IsValidPass())
	{
		ENQUEUE_RENDER_COMMAND(SurfaceNormalPassCommand)
		(
			[HeightTextureSRV, this](FRHICommandListImmediate& RHICmdList)
			{
				check(IsInRenderingThread());

				// Bind shader textures
				TShaderMapRef<FSurfaceNormalComputeShader> SurfaceNormalComputeShader(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
				RHICmdList.SetComputeShader(SurfaceNormalComputeShader->GetComputeShader());
				SurfaceNormalComputeShader->BindShaderTextures(RHICmdList, OutputNormalTextureUAV, HeightTextureSRV);

				// Dispatch shader
				const int ThreadGroupCountX = StaticCast<int>(Config.TextureWidth / 32);
				const int ThreadGroupCountY = StaticCast<int>(Config.TextureHeight / 32);
				DispatchComputeShader(RHICmdList, *SurfaceNormalComputeShader, ThreadGroupCountX, ThreadGroupCountY, 1);

				// Unbind shader textures
				SurfaceNormalComputeShader->UnbindShaderTextures(RHICmdList);

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
	bValid &= !!OutputNormalTextureUAV;
	bValid &= !!OutputNormalTextureSRV;

	return bValid;
}

