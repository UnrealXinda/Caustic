// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "SurfaceCausticPass.h"
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

struct FCausticSimpleVertex
{
	FVector4  Position;
	FVector2D UV;
};

class FSurfaceCausticVertexDeclaration : public FRenderResource
{
public:

	FRHIVertexDeclaration* VertexDeclarationRHI;

	virtual void InitRHI() override
	{
		FVertexDeclarationElementList Elements;
		uint32 Stride = sizeof(FCausticSimpleVertex);
		Elements.Add(FVertexElement(0, STRUCT_OFFSET(FCausticSimpleVertex, Position), VET_Float4, 0, Stride));
		Elements.Add(FVertexElement(0, STRUCT_OFFSET(FCausticSimpleVertex, UV), VET_Float2, 1, Stride));

		VertexDeclarationRHI = RHICreateVertexDeclaration(Elements);
	}

	virtual void ReleaseRHI()
	{
		if (VertexDeclarationRHI)
		{
			VertexDeclarationRHI->Release();
		}
	}
};

class FSurfaceCausticSimpleVertexBuffer : public FVertexBuffer
{
public:

	void InitRHI() override
	{
		TResourceArray<FCausticSimpleVertex, VERTEXBUFFER_ALIGNMENT> Vertices;
		Vertices.SetNumUninitialized(6);

		Vertices[0] = { FVector4(1, 1, 0, 1), FVector2D(1, 1) };
		Vertices[1] = { FVector4(-1, 1, 0, 1), FVector2D(0, 1) };
		Vertices[2] = { FVector4(1, -1, 0, 1), FVector2D(1, 0) };
		Vertices[3] = { FVector4(-1, -1, 0, 1), FVector2D(0, 0) };

		// The final two vertices are used for the triangle optimization (a single triangle spans the entire viewport)
		Vertices[4] = { FVector4(-1, 1, 0, 1), FVector2D(-1, 1) };
		Vertices[5] = { FVector4(1, -1, 0, 1), FVector2D(1, -1) };

		FRHIResourceCreateInfo CreateInfo(&Vertices);
		VertexBufferRHI = RHICreateVertexBuffer(Vertices.GetResourceDataSize(), BUF_Static, CreateInfo);
	}
};

class FSurfaceCausticSimpleIndexBuffer : public FIndexBuffer
{
public:

	void InitRHI() override
	{
		const uint16 Indices[] = { 0, 1, 2, 2, 1, 3, 0, 4, 5 };
		TResourceArray<uint16, INDEXBUFFER_ALIGNMENT> IndexBuffer;
		uint32 NumIndices = UE_ARRAY_COUNT(Indices);
		IndexBuffer.AddUninitialized(NumIndices);
		FMemory::Memcpy(IndexBuffer.GetData(), Indices, NumIndices * sizeof(uint16));

		// Create index buffer. Fill buffer with initial data upon creation
		FRHIResourceCreateInfo CreateInfo(&IndexBuffer);
		IndexBufferRHI = RHICreateIndexBuffer(sizeof(uint16), IndexBuffer.GetResourceDataSize(), BUF_Static, CreateInfo);
	}
};

TGlobalResource<FSurfaceCausticSimpleVertexBuffer> GSurfaceCausticVertexBuffer;
TGlobalResource<FSurfaceCausticSimpleIndexBuffer> GSurfaceCausticIndexBuffer;

class FSurfaceCausticVertexShader : public FGlobalShader
{

	DECLARE_SHADER_TYPE(FSurfaceCausticVertexShader, Global);

public:

	FSurfaceCausticVertexShader() {}
	FSurfaceCausticVertexShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FGlobalShader(Initializer) {}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};

BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FSurfaceCausticPixelShaderParameters, )
	SHADER_PARAMETER(float, Refraction)
END_GLOBAL_SHADER_PARAMETER_STRUCT()
IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FSurfaceCausticPixelShaderParameters, "SurfaceCausticUniform");

class FSurfaceCausticPixelShader : public FGlobalShader
{

	DECLARE_SHADER_TYPE(FSurfaceCausticPixelShader, Global);

public:

	FSurfaceCausticPixelShader() {}
	FSurfaceCausticPixelShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FGlobalShader(Initializer)
	{
		InputNormalTexture.Bind(Initializer.ParameterMap, TEXT("InputNormalTexture"));
		NormalMapSampler.Bind(Initializer.ParameterMap, TEXT("NormalMapSampler"));
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	void BindShaderTextures(FRHICommandList& RHICmdList, FShaderResourceViewRHIRef InputTextureSRV)
	{
		FRHIPixelShader* PixelShaderRHI = GetPixelShader();

		if (InputNormalTexture.IsBound())
		{
			RHICmdList.SetShaderResourceViewParameter(PixelShaderRHI, InputNormalTexture.GetBaseIndex(), InputTextureSRV);
		}
		
		if (NormalMapSampler.IsBound())
		{
			FRHISamplerState* SamplerStateLinear = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
			RHICmdList.SetShaderSampler(PixelShaderRHI, NormalMapSampler.GetBaseIndex(), SamplerStateLinear);
		}
	}

	void UnbindShaderTextures(FRHICommandList& RHICmdList)
	{
		FRHIPixelShader* PixelShaderRHI = GetPixelShader();

		if (InputNormalTexture.IsBound())
		{
			RHICmdList.SetShaderResourceViewParameter(PixelShaderRHI, InputNormalTexture.GetBaseIndex(), FShaderResourceViewRHIRef());
		}
	}

	void SetShaderParameters(FRHICommandList& RHICmdList, const FSurfaceCausticPixelShaderParameters& Parameters)
	{
		FRHIComputeShader* ComputeShaderRHI = GetComputeShader();
		SetUniformBufferParameterImmediate(RHICmdList, ComputeShaderRHI, GetUniformBufferParameter<FSurfaceCausticPixelShaderParameters>(), Parameters);
	}

private:

	FShaderResourceParameter InputNormalTexture;
	FShaderResourceParameter NormalMapSampler;
};

IMPLEMENT_SHADER_TYPE(, FSurfaceCausticVertexShader, TEXT("/Plugin/Caustic/SurfaceCausticShader.usf"), TEXT("MainVS"), SF_Vertex)
IMPLEMENT_SHADER_TYPE(, FSurfaceCausticPixelShader, TEXT("/Plugin/Caustic/SurfaceCausticShader.usf"), TEXT("MainPS"), SF_Pixel)

FSurfaceCausticPassRenderer::FSurfaceCausticPassRenderer() :
	bInitiated(false)
{

}

FSurfaceCausticPassRenderer::~FSurfaceCausticPassRenderer()
{
}

void FSurfaceCausticPassRenderer::InitPass(const FSurfaceCausticPassConfig& InConfig)
{
	if (!bInitiated)
	{
		Config = InConfig;
		bInitiated = true;
	}
}

void FSurfaceCausticPassRenderer::Render(const FLiquidParam& LiquidParam, FShaderResourceViewRHIRef NormalTextureSRV, UTextureRenderTarget2D* RenderTarget)
{
	if (IsValidPass())
	{
		ENQUEUE_RENDER_COMMAND(SurfaceCausticPassCommand)
		(
			[&LiquidParam, NormalTextureSRV, RenderTarget, this](FRHICommandListImmediate& RHICmdList)
			{
				check(IsInRenderingThread());

				FTextureRenderTargetResource* RenderTargetResource = RenderTarget->GetRenderTargetResource();
				FTexture2DRHIRef RenderTargetResourceRef = RenderTargetResource->GetRenderTargetTexture();
				FRHIRenderPassInfo PassInfo(RenderTargetResourceRef, ERenderTargetActions::DontLoad_Store,nullptr);

				RHICmdList.BeginRenderPass(PassInfo, TEXT("SurfaceCausticPass"));
				{
					const uint32 TextureWidth = Config.TextureWidth;
					const uint32 TextureHeight = Config.TextureHeight;
					const uint32 RenderTextureWidth = RenderTargetResourceRef->GetSizeX();
					const uint32 RenderTextureHeight = RenderTargetResourceRef->GetSizeY();
					FIntPoint DisplacementMapResolution(RenderTextureWidth, RenderTextureHeight);

					// Update viewport
					RHICmdList.SetViewport(
						0.f, 0.f, 0.f,
						TextureWidth, TextureHeight, 1.f
					);

					// Get shaders
					TShaderMap<FGlobalShaderType>* GlobalShaderMap = GetGlobalShaderMap(ERHIFeatureLevel::SM5);
					TShaderMapRef<FSurfaceCausticVertexShader> VertexShader(GlobalShaderMap);
					TShaderMapRef<FSurfaceCausticPixelShader> PixelShader(GlobalShaderMap);

					FSurfaceCausticVertexDeclaration VertexDesc;
					VertexDesc.InitRHI();

					// Set the graphic pipeline state
					FGraphicsPipelineStateInitializer GraphicsPSOInit;
					RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
					GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
					GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
					GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
					GraphicsPSOInit.PrimitiveType = PT_TriangleList;
					GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = VertexDesc.VertexDeclarationRHI;
					GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
					GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);
					SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

					// Update viewport
					RHICmdList.SetViewport(
						0.f, 0.f, 0.f,
						RenderTextureWidth, RenderTextureHeight, 1.f
					);

					// Bind shader textures
					PixelShader->BindShaderTextures(RHICmdList, NormalTextureSRV);

					// Bind shader uniform
					FSurfaceCausticPixelShaderParameters UniformParam;
					UniformParam.Refraction = LiquidParam.Refraction;
					PixelShader->SetShaderParameters(RHICmdList, UniformParam);

					// Dispatch pass
					RHICmdList.SetStreamSource(0, GSurfaceCausticVertexBuffer.VertexBufferRHI, 0);
					RHICmdList.DrawIndexedPrimitive(
						GSurfaceCausticIndexBuffer.IndexBufferRHI,
						0, // BaseVertexIndex
						0, // MinIndex
						4, // NumBertices
						0, // StartIndex
						2, // NumPrimitives
						1  // NumInstances
					);

					// Unbind shader textures
					PixelShader->UnbindShaderTextures(RHICmdList);
				}

				RHICmdList.EndRenderPass();
			}
		);
	}
}

bool FSurfaceCausticPassRenderer::IsValidPass() const
{
	return true;
}

