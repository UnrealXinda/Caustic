// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "SurfaceDepthPass.h"
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
#include "Components/PrimitiveComponent.h"

BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FSurfaceDepthComputeShaderParameters, )
	SHADER_PARAMETER(float, MinDepth)
	SHADER_PARAMETER(float, MaxDepth)
END_GLOBAL_SHADER_PARAMETER_STRUCT()
IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FSurfaceDepthComputeShaderParameters, "SurfaceDepthUniform");

class FSurfaceDepthComputeShader : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FSurfaceDepthComputeShader);

public:

	FSurfaceDepthComputeShader() {}
	FSurfaceDepthComputeShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		InputDepthTexture.Bind(Initializer.ParameterMap, TEXT("InputDepthTexture"));
		OutputDepthTexture.Bind(Initializer.ParameterMap, TEXT("OutputDepthTexture"));
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	void BindShaderTextures(FRHICommandList& RHICmdList, FUnorderedAccessViewRHIRef OutputTextureUAV, FShaderResourceViewRHIRef InputTextureSRV)
	{
		FRHIComputeShader* ComputeShaderRHI = GetComputeShader();

		if (InputDepthTexture.IsBound())
		{
			RHICmdList.SetShaderResourceViewParameter(ComputeShaderRHI, InputDepthTexture.GetBaseIndex(), InputTextureSRV);
		}

		if (OutputDepthTexture.IsBound())
		{
			RHICmdList.SetUAVParameter(ComputeShaderRHI, OutputDepthTexture.GetBaseIndex(), OutputTextureUAV);
		}
	}

	void SetShaderParameters(FRHICommandList& RHICmdList, const FSurfaceDepthComputeShaderParameters& Parameters)
	{
		FRHIComputeShader* ComputeShaderRHI = GetComputeShader();
		SetUniformBufferParameterImmediate(RHICmdList, ComputeShaderRHI, GetUniformBufferParameter<FSurfaceDepthComputeShaderParameters>(), Parameters);

	}

private:

	FShaderResourceParameter InputDepthTexture;
	FShaderResourceParameter OutputDepthTexture;
};
IMPLEMENT_SHADER_TYPE(, FSurfaceDepthComputeShader, TEXT("/Plugin/Caustic/SurfaceDepthComputeShader.usf"), TEXT("ComputeSurfaceDepth"), SF_Compute);

FSurfaceDepthPassRenderer::FSurfaceDepthPassRenderer() :
	bInitiated(false)
{

}

FSurfaceDepthPassRenderer::~FSurfaceDepthPassRenderer()
{
#define SafeReleaseTextureResource(Texture)  \
	do {                                     \
		if (Texture.IsValid()) {             \
			Texture->Release();              \
		}                                    \
	} while(0);

	SafeReleaseTextureResource(InputDepthTexture);
	SafeReleaseTextureResource(InputDepthTextureSRV);
	SafeReleaseTextureResource(OutputDepthTexture);
	SafeReleaseTextureResource(OutputDepthTextureUAV);
	SafeReleaseTextureResource(OutputDepthTextureSRV);
}

void FSurfaceDepthPassRenderer::InitPass(const FSurfaceDepthPassConfig& InConfig)
{
	if (!bInitiated)
	{
		FRHIResourceCreateInfo CreateInfo;
		OutputDepthTexture = RHICreateTexture2D(InConfig.TextureWidth, InConfig.TextureHeight, PF_FloatRGBA, 1, 1, TexCreate_ShaderResource | TexCreate_UAV, CreateInfo);
		OutputDepthTextureUAV = RHICreateUnorderedAccessView(OutputDepthTexture);
		OutputDepthTextureSRV = RHICreateShaderResourceView(OutputDepthTexture, 0);
		
		InputDepthTexture = RHICreateTexture2D(InConfig.TextureWidth, InConfig.TextureHeight, PF_R16F, 1, 1, TexCreate_ShaderResource, CreateInfo);
		InputDepthTextureSRV = RHICreateShaderResourceView(InputDepthTexture, 0);

		if (InConfig.DebugTextureRef)
		{
			DebugTextureRHIRef = InConfig.DebugTextureRef->TextureReference.TextureReferenceRHI->GetTextureReference()->GetReferencedTexture();
		}

		Config = InConfig;

		bInitiated = true;
	}
}

void FSurfaceDepthPassRenderer::Render(FRHITexture* DepthTextureRef)
{
	if (IsValidPass())
	{
		ENQUEUE_RENDER_COMMAND(SurfaceDepthPass)
		(
			[DepthTextureRef, this](FRHICommandListImmediate& RHICmdList)
			{
				check(IsInRenderingThread());

				// Copy depth texture
				FRHICopyTextureInfo CopyInfo;
				RHICmdList.CopyTexture(DepthTextureRef, InputDepthTexture, CopyInfo);

				// Bind shader textures
				TShaderMapRef<FSurfaceDepthComputeShader> SurfaceDepthComputeShader(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
				RHICmdList.SetComputeShader(SurfaceDepthComputeShader->GetComputeShader());
				SurfaceDepthComputeShader->BindShaderTextures(RHICmdList, OutputDepthTextureUAV, InputDepthTextureSRV);

				// Bind shader uniform
				FSurfaceDepthComputeShaderParameters UniformParam;
				UniformParam.MinDepth = Config.MinDepth;
				UniformParam.MaxDepth = Config.MaxDepth;
				SurfaceDepthComputeShader->SetShaderParameters(RHICmdList, UniformParam);

				// Dispatch shader
				int ThreadGroupCountX = StaticCast<int>(Config.TextureWidth / 32);
				int ThreadGroupCountY = StaticCast<int>(Config.TextureHeight / 32);

				DispatchComputeShader(RHICmdList, *SurfaceDepthComputeShader, ThreadGroupCountX, ThreadGroupCountY, 1);

				if (DebugTextureRHIRef)
				{
					RHICmdList.CopyToResolveTarget(OutputDepthTexture, DebugTextureRHIRef, FResolveParams());
				}
			}
		);
	}
}

bool FSurfaceDepthPassRenderer::IsValidPass() const
{
	bool bValid = !!InputDepthTexture;
	bValid &= !!InputDepthTextureSRV;
	bValid &= !!OutputDepthTexture;
	bValid &= !!OutputDepthTextureUAV;
	bValid &= !!OutputDepthTextureSRV;

	return bValid;
}

