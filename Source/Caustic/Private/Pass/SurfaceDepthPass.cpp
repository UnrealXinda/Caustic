// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "SurfaceDepthPass.h"
#include "Components/PrimitiveComponent.h"

class FSurfaceDepthVertexShader : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FSurfaceDepthVertexShader);
	SHADER_USE_PARAMETER_STRUCT(FSurfaceDepthVertexShader, FGlobalShader);

public:

	BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(FMatrix, Model)
		SHADER_PARAMETER(FMatrix, View)
		SHADER_PARAMETER(FMatrix, Projection)
		SHADER_PARAMETER(float,   MinDepth)
		SHADER_PARAMETER(float,   MaxDepth)
	END_GLOBAL_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};
IMPLEMENT_SHADER_TYPE(, FSurfaceDepthVertexShader, TEXT("/Plugin/Caustic/SurfaceDepthVertexShader.usf"), TEXT("Main_VS"), SF_Vertex);

class FSurfaceDepthPixelShader : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FSurfaceDepthPixelShader);
	SHADER_USE_PARAMETER_STRUCT(FSurfaceDepthPixelShader, FGlobalShader);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};
IMPLEMENT_SHADER_TYPE(, FSurfaceDepthPixelShader, TEXT("/Plugin/Caustic/SurfaceDepthPixelShader.usf"), TEXT("Main_PS"), SF_Pixel);

void FSurfaceDepthPassRenderer::Render(
	ERHIFeatureLevel::Type        FeatureLevel,
	FRHICommandListImmediate&     RHICmdList,
	FVector2D                     RenderTargetSize,
	const FMatrix&                View,
	const FMatrix&                Projection,
	TArray<UPrimitiveComponent*>& Components
)
{
	check(IsInRenderingThread());

	FRDGBuilder GraphBuilder(RHICmdList);

	FSurfaceDepthVertexShader::FParameters* VertexParams = GraphBuilder.AllocParameters<FSurfaceDepthVertexShader::FParameters>();
	{
		// Init vertex shader params
		VertexParams->View = View;
		VertexParams->Projection = Projection;
	}

	// UPrimitiveComponent -> FPrimitiveSceneProxy -> MeshProcessor(?) -> MeshDrawCommand(?) -> RHICommandList

	GraphBuilder.AddPass(RDG_EVENT_NAME("SurfaceDepthPass"), VertexParams, ERDGPassFlags::Raster,
		[this](FRHICommandList& RHICmdList)
		{
		}
	);
}