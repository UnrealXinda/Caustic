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
#include "Pass/PassUtils.h"

BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FSurfaceDepthComputeShaderParameters, )
	SHADER_PARAMETER(float, MinDepth)
	SHADER_PARAMETER(float, MaxDepth)
	SHADER_PARAMETER(float, ForceFactor)
END_GLOBAL_SHADER_PARAMETER_STRUCT()
IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FSurfaceDepthComputeShaderParameters, "SurfaceDepthUniform");

BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FSurfaceHeightComputeShaderParameters, )
	SHADER_PARAMETER(FVector4, LiquidParam)
	SHADER_PARAMETER(float, AttenuationCoefficient)
END_GLOBAL_SHADER_PARAMETER_STRUCT()
IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FSurfaceHeightComputeShaderParameters, "SurfaceHeightUniform");

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

class FSurfaceHeightComputeShader : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FSurfaceHeightComputeShader);

public:

	FSurfaceHeightComputeShader() {}
	FSurfaceHeightComputeShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		CurDepthTexture.Bind(Initializer.ParameterMap, TEXT("CurDepthTexture"));
		PrevDepthTexture.Bind(Initializer.ParameterMap, TEXT("PrevDepthTexture"));
		OutputHeightTexture.Bind(Initializer.ParameterMap, TEXT("OutputHeightTexture"));
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	void BindShaderTextures(
		FRHICommandList& RHICmdList,
		FUnorderedAccessViewRHIRef OutputHeightTextureUAV,
		FShaderResourceViewRHIRef CurDepthTextureSRV,
		FShaderResourceViewRHIRef PrevDepthTextureSRV
	)
	{
		FRHIComputeShader* ComputeShaderRHI = GetComputeShader();

		if (OutputHeightTexture.IsBound())
		{
			RHICmdList.SetUAVParameter(ComputeShaderRHI, OutputHeightTexture.GetBaseIndex(), OutputHeightTextureUAV);
		}

		if (CurDepthTexture.IsBound())
		{
			RHICmdList.SetShaderResourceViewParameter(ComputeShaderRHI, CurDepthTexture.GetBaseIndex(), CurDepthTextureSRV);
		}

		if (PrevDepthTexture.IsBound())
		{
			RHICmdList.SetShaderResourceViewParameter(ComputeShaderRHI, PrevDepthTexture.GetBaseIndex(), PrevDepthTextureSRV);
		}
	}

	void SetShaderParameters(FRHICommandList& RHICmdList, const FSurfaceHeightComputeShaderParameters& Parameters)
	{
		FRHIComputeShader* ComputeShaderRHI = GetComputeShader();
		SetUniformBufferParameterImmediate(RHICmdList, ComputeShaderRHI, GetUniformBufferParameter<FSurfaceHeightComputeShaderParameters>(), Parameters);
	}

private:

	FShaderResourceParameter CurDepthTexture;
	FShaderResourceParameter PrevDepthTexture;
	FShaderResourceParameter OutputHeightTexture;
};

IMPLEMENT_SHADER_TYPE(, FSurfaceDepthComputeShader, TEXT("/Plugin/Caustic/SurfaceDepthComputeShader.usf"), TEXT("ComputeSurfaceDepth"), SF_Compute);
IMPLEMENT_SHADER_TYPE(, FSurfaceHeightComputeShader, TEXT("/Plugin/Caustic/SurfaceHeightComputeShader.usf"), TEXT("ComputeSurfaceHeight"), SF_Compute);

FSurfaceDepthPassRenderer::FSurfaceDepthPassRenderer() :
	bInitiated(false)
{

}

FSurfaceDepthPassRenderer::~FSurfaceDepthPassRenderer()
{
	SafeReleaseTextureResource(InputDepthTexture);
	SafeReleaseTextureResource(InputDepthTextureSRV);
	SafeReleaseTextureResource(OutputDepthTexture);
	SafeReleaseTextureResource(OutputDepthTextureUAV);
	SafeReleaseTextureResource(OutputDepthTextureSRV);
	SafeReleaseTextureResource(OutputHeightTexture);
	SafeReleaseTextureResource(OutputHeightTextureUAV);
	SafeReleaseTextureResource(OutputHeightTextureSRV);
	SafeReleaseTextureResource(PrevDepthTexture);
	SafeReleaseTextureResource(PrevDepthTextureSRV);
}

void FSurfaceDepthPassRenderer::InitPass(const FSurfaceDepthPassConfig& InConfig)
{
	if (!bInitiated)
	{
		FRHIResourceCreateInfo CreateInfo;
		uint32 TextureWidth = InConfig.TextureWidth;
		uint32 TextureHeight = InConfig.TextureHeight;

		OutputDepthTexture = RHICreateTexture2D(TextureWidth, TextureHeight, PF_FloatRGBA, 1, 1, TexCreate_ShaderResource | TexCreate_UAV, CreateInfo);
		OutputDepthTextureUAV = RHICreateUnorderedAccessView(OutputDepthTexture);
		OutputDepthTextureSRV = RHICreateShaderResourceView(OutputDepthTexture, 0);

		OutputHeightTexture = RHICreateTexture2D(TextureWidth, TextureHeight, PF_FloatRGBA, 1, 1, TexCreate_ShaderResource | TexCreate_UAV, CreateInfo);
		OutputHeightTextureUAV = RHICreateUnorderedAccessView(OutputHeightTexture);
		OutputHeightTextureSRV = RHICreateShaderResourceView(OutputHeightTexture, 0);
		
		InputDepthTexture = RHICreateTexture2D(TextureWidth, TextureHeight, PF_R16F, 1, 1, TexCreate_ShaderResource, CreateInfo);
		InputDepthTextureSRV = RHICreateShaderResourceView(InputDepthTexture, 0);

		PrevDepthTexture =  RHICreateTexture2D(TextureWidth, TextureHeight, PF_FloatRGBA, 1, 1, TexCreate_ShaderResource, CreateInfo);
		PrevDepthTextureSRV = RHICreateShaderResourceView(PrevDepthTexture, 0);

		DepthDebugTextureRHIRef = Caustic::GetRHITextureFromRenderTarget(InConfig.DepthDebugTextureRef);
		HeightDebugTextureRHIRef = Caustic::GetRHITextureFromRenderTarget(InConfig.HeightDebugTextureRef);

		Config = InConfig;

		bInitiated = true;
	}
}

void FSurfaceDepthPassRenderer::Render(const FLiquidParam& LiquidParam, FRHITexture* DepthTextureRef)
{
	if (IsValidPass())
	{
		ENQUEUE_RENDER_COMMAND(SurfaceDepthPass)
		(
			[&LiquidParam, DepthTextureRef, this](FRHICommandListImmediate& RHICmdList)
			{
				check(IsInRenderingThread());

				RenderSurfaceDepthPass(RHICmdList, LiquidParam, DepthTextureRef);
				RenderSurfaceHeightPass(RHICmdList, LiquidParam);
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
	bValid &= !!OutputHeightTexture;
	bValid &= !!OutputHeightTextureUAV;
	bValid &= !!OutputHeightTextureSRV;
	bValid &= !!PrevDepthTexture;
	bValid &= !!PrevDepthTextureSRV;

	return bValid;
}

void FSurfaceDepthPassRenderer::RenderSurfaceDepthPass(FRHICommandListImmediate& RHICmdList, const FLiquidParam& LiquidParam, FRHITexture* DepthTextureRef)
{
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
	UniformParam.ForceFactor = LiquidParam.ForceFactor;
	SurfaceDepthComputeShader->SetShaderParameters(RHICmdList, UniformParam);

	// Dispatch shader
	const int ThreadGroupCountX = StaticCast<int>(Config.TextureWidth / 32);
	const int ThreadGroupCountY = StaticCast<int>(Config.TextureHeight / 32);
	DispatchComputeShader(RHICmdList, *SurfaceDepthComputeShader, ThreadGroupCountX, ThreadGroupCountY, 1);

	// Debug drawing
	if (DepthDebugTextureRHIRef)
	{
		RHICmdList.CopyToResolveTarget(OutputDepthTexture, DepthDebugTextureRHIRef, FResolveParams());
	}
}

void FSurfaceDepthPassRenderer::RenderSurfaceHeightPass(FRHICommandListImmediate& RHICmdList, const FLiquidParam& LiquidParam)
{
	// Bind shader textures
	TShaderMapRef<FSurfaceHeightComputeShader> SurfaceHeightComputeShader(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
	RHICmdList.SetComputeShader(SurfaceHeightComputeShader->GetComputeShader());
	SurfaceHeightComputeShader->BindShaderTextures(RHICmdList, OutputHeightTextureUAV, OutputDepthTextureSRV, PrevDepthTextureSRV);

	// Bind shader uniform
	FSurfaceHeightComputeShaderParameters UniformParam;
	UniformParam.LiquidParam = EncodeLiquidParam(LiquidParam);
	UniformParam.AttenuationCoefficient = LiquidParam.AttenuationCoefficient;
	SurfaceHeightComputeShader->SetShaderParameters(RHICmdList, UniformParam);

	// Dispatch shader
	const int ThreadGroupCountX = StaticCast<int>(Config.TextureWidth / 32);
	const int ThreadGroupCountY = StaticCast<int>(Config.TextureHeight / 32);
	DispatchComputeShader(RHICmdList, *SurfaceHeightComputeShader, ThreadGroupCountX, ThreadGroupCountY, 1);

	// Copy to cache depth texture
	FRHICopyTextureInfo CopyInfo;
	RHICmdList.CopyTexture(OutputDepthTexture, PrevDepthTexture, CopyInfo);
	RHICmdList.CopyTexture(OutputHeightTexture, OutputDepthTexture, CopyInfo);

	// Debug drawing
	if (HeightDebugTextureRHIRef)
	{
		RHICmdList.CopyToResolveTarget(OutputHeightTexture, HeightDebugTextureRHIRef, FResolveParams());
	}
}

FVector4 FSurfaceDepthPassRenderer::EncodeLiquidParam(const FLiquidParam& LiquidParam) const
{
	const float SampleSpacing = 1.0f / LiquidParam.DepthTextureWidth;
	const float FixedDeltaTime = 0.016f;
	float Viscosity = FMath::Abs(LiquidParam.Viscosity);
	float MaxVelocity = SampleSpacing / (2 * FixedDeltaTime) * FMath::Sqrt(Viscosity * FixedDeltaTime + 2);
	float Velocity = FMath::Abs(LiquidParam.Velocity) * MaxVelocity;
	float ViscositySqr = Viscosity * Viscosity;
	float VelocitySqr = Velocity * Velocity;
	float DeltaSizeSqr = SampleSpacing * SampleSpacing;
	float DeltaT = FMath::Sqrt(ViscositySqr + 32 * VelocitySqr / DeltaSizeSqr);
	float DeltaTDensity = 8 * VelocitySqr / DeltaSizeSqr;
	float MaxT1 = (Viscosity + DeltaT) / DeltaTDensity;
	float MaxT2 = (Viscosity - DeltaT) / DeltaTDensity;

	float MaxT = (MaxT2 > 0) ? FMath::Min(MaxT1, MaxT2) : MaxT1;
	MaxT = FMath::Max(FixedDeltaTime, MaxT);

	float Factor = VelocitySqr * FixedDeltaTime * FixedDeltaTime / DeltaSizeSqr;
	float I = Viscosity * FixedDeltaTime - 2;
	float J = Viscosity * FixedDeltaTime + 2;

	float K1 = (4 - 8 * Factor) / J;
	float K2 = I / J;
	float K3 = 2 * Factor / J;

	return FVector4(K1, K2, K3, SampleSpacing);
}

