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

	int32 VertexCount;

	void Init(uint32 Width, uint32 Height, uint32 CellSize)
	{
		ReleaseRHI();

		const int32 SizeX = FMath::RoundToInt(Width / CellSize);
		const int32 SizeY = FMath::RoundToInt(Height / CellSize);
		const float CellU = 1.0f / SizeX;
		const float CellV = 1.0f / SizeY;
		VertexCount = (SizeX + 1) * (SizeY + 1);

		TResourceArray<FCausticSimpleVertex, VERTEXBUFFER_ALIGNMENT> Vertices;
		Vertices.SetNumUninitialized(VertexCount);

		int Index = 0;
		for (int32 Y = 0; Y <= SizeY; ++Y)
		{
			for (int32 X = 0; X <= SizeX; ++X)
			{
				float U = X * CellU;
				float V = Y * CellV;

				float LocX = FMath::Lerp(-1.f, 1.f, U);
				float LocY = FMath::Lerp(-1.f, 1.f, V);
				float LocZ = 0.0f;

				Vertices[Index++] = { FVector4(LocX, LocY, LocZ, 1), FVector2D(U, V) };
			}
		}

		FRHIResourceCreateInfo CreateInfo(&Vertices);
		VertexBufferRHI = RHICreateVertexBuffer(Vertices.GetResourceDataSize(), BUF_Dynamic, CreateInfo);
	}

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
		VertexBufferRHI = RHICreateVertexBuffer(Vertices.GetResourceDataSize(), BUF_Dynamic, CreateInfo);
	}
};

class FSurfaceCausticSimpleIndexBuffer : public FIndexBuffer
{
public:

	int32 IndexCount;

	void Init(uint32 Width, uint32 Height, uint32 CellSize)
	{
		ReleaseRHI();

		const uint16 SizeX = FMath::RoundToInt(Width / CellSize);
		const uint16 SizeY = FMath::RoundToInt(Height / CellSize);
		const float CellWidth = Width / SizeX;
		const float CellHeight = Height / SizeY;
		const float CellU = 1.0f / SizeX;
		const float CellV = 1.0f / SizeY;
		IndexCount = SizeX * SizeY * 6;

		TResourceArray<uint16, INDEXBUFFER_ALIGNMENT> Indices;
		Indices.SetNumUninitialized(IndexCount);

		int Index = 0;
		for (uint16 Y = 0; Y < SizeY; ++Y)
		{
			for (uint16 X = 0; X < SizeX; ++X)
			{
				uint16 A = Y * (SizeX + 1) + X;
				uint16 B = A + SizeX + 1;
				uint16 C = A + SizeX + 2;
				uint16 D = A + 1;

				Indices[Index++] = A;
				Indices[Index++] = B;
				Indices[Index++] = C;

				Indices[Index++] = A;
				Indices[Index++] = C;
				Indices[Index++] = D;
			}
		}

		FRHIResourceCreateInfo CreateInfo(&Indices);
		IndexBufferRHI = RHICreateIndexBuffer(sizeof(uint16), Indices.GetResourceDataSize(), BUF_Dynamic, CreateInfo);
	}

	void InitRHI() override
	{
		const uint16 Indices[] = { 0, 1, 2, 2, 1, 3, 0, 4, 5 };
		TResourceArray<uint16, INDEXBUFFER_ALIGNMENT> IndexBuffer;
		uint32 NumIndices = UE_ARRAY_COUNT(Indices);
		IndexBuffer.SetNumUninitialized(NumIndices);
		FMemory::Memcpy(IndexBuffer.GetData(), Indices, NumIndices * sizeof(uint16));

		// Create index buffer. Fill buffer with initial data upon creation
		FRHIResourceCreateInfo CreateInfo(&IndexBuffer);
		IndexBufferRHI = RHICreateIndexBuffer(sizeof(uint16), IndexBuffer.GetResourceDataSize(), BUF_Dynamic, CreateInfo);
	}
};

TGlobalResource<FSurfaceCausticSimpleVertexBuffer> GSurfaceCausticVertexBuffer;
TGlobalResource<FSurfaceCausticSimpleIndexBuffer> GSurfaceCausticIndexBuffer;

BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FSurfaceCausticVertexShaderParameters, )
	SHADER_PARAMETER(float, Refraction)
END_GLOBAL_SHADER_PARAMETER_STRUCT()
IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FSurfaceCausticVertexShaderParameters, "SurfaceCausticUniform");

class FSurfaceCausticVertexShader : public FGlobalShader
{

	DECLARE_SHADER_TYPE(FSurfaceCausticVertexShader, Global);

public:

	FSurfaceCausticVertexShader() {}
	FSurfaceCausticVertexShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FGlobalShader(Initializer)
	{
		InputNormalTexture.Bind(Initializer.ParameterMap, TEXT("InputNormalTexture"));
		CausticPassSampler.Bind(Initializer.ParameterMap, TEXT("CausticPassSampler"));
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << InputNormalTexture << CausticPassSampler;
		return bShaderHasOutdatedParameters;
	}

	void BindShaderTextures(FRHICommandList& RHICmdList, FShaderResourceViewRHIRef InputTextureSRV)
	{
		FRHIVertexShader* VertexShaderRHI = GetVertexShader();
		FRHISamplerState* SamplerStateLinear = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();

		SetSRVParameter(RHICmdList, VertexShaderRHI, InputNormalTexture, InputTextureSRV);
		SetSamplerParameter(RHICmdList, VertexShaderRHI, CausticPassSampler, SamplerStateLinear);
	}

	void UnbindShaderTextures(FRHICommandList& RHICmdList)
	{
		FRHIVertexShader* VertexShaderRHI = GetVertexShader();
		SetSRVParameter(RHICmdList, VertexShaderRHI, InputNormalTexture, FShaderResourceViewRHIRef());
	}

	void SetShaderParameters(FRHICommandList& RHICmdList, const FSurfaceCausticVertexShaderParameters& Parameters)
	{
		FRHIVertexShader* VertexShaderRHI = GetVertexShader();
		SetUniformBufferParameterImmediate(RHICmdList, VertexShaderRHI, GetUniformBufferParameter<FSurfaceCausticVertexShaderParameters>(), Parameters);
	}

private:

	FShaderResourceParameter InputNormalTexture;
	FShaderResourceParameter CausticPassSampler;
};

class FSurfaceCausticPixelShader : public FGlobalShader
{

	DECLARE_SHADER_TYPE(FSurfaceCausticPixelShader, Global);

public:

	FSurfaceCausticPixelShader() {}
	FSurfaceCausticPixelShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FGlobalShader(Initializer)
	{
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5);
	}
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

		GSurfaceCausticVertexBuffer.Init(Config.TextureWidth, Config.TextureHeight, Config.CellSize);
		GSurfaceCausticIndexBuffer.Init(Config.TextureWidth, Config.TextureHeight, Config.CellSize);
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
				FRHIRenderPassInfo PassInfo(RenderTargetResourceRef, ERenderTargetActions::DontLoad_Store, nullptr);

				RHICmdList.BeginRenderPass(PassInfo, TEXT("SurfaceCausticPass"));
				{
					const uint32 TextureWidth = Config.TextureWidth;
					const uint32 TextureHeight = Config.TextureHeight;
					const uint32 RenderTextureWidth = RenderTargetResourceRef->GetSizeX();
					const uint32 RenderTextureHeight = RenderTargetResourceRef->GetSizeY();

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
					VertexShader->BindShaderTextures(RHICmdList, NormalTextureSRV);

					// Bind shader uniform
					FSurfaceCausticVertexShaderParameters UniformParam;
					UniformParam.Refraction = LiquidParam.Refraction;
					VertexShader->SetShaderParameters(RHICmdList, UniformParam);

					// Dispatch pass
					RHICmdList.SetStreamSource(0, GSurfaceCausticVertexBuffer.VertexBufferRHI, 0);
					RHICmdList.DrawIndexedPrimitive(
						GSurfaceCausticIndexBuffer.IndexBufferRHI,
						0, // BaseVertexIndex
						0, // MinIndex
						GSurfaceCausticVertexBuffer.VertexCount, // NumVertices
						0, // StartIndex
						GSurfaceCausticIndexBuffer.IndexCount / 3, // NumPrimitives
						1  // NumInstances
					);

					// Unbind shader textures
					VertexShader->UnbindShaderTextures(RHICmdList);
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