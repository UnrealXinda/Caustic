// Fill out your copyright notice in the Description page of Project Settings.


#include "CausticBody.h"
#include "Components/BoxComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ProceduralMeshComponent.h"
#include "Kismet/KismetRenderingLibrary.h"

// Sets default values
ACausticBody::ACausticBody() :
	SurfaceDepthPassRenderer(new FSurfaceDepthPassRenderer())
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	BoxCollisionComp = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollisionComponent"));
	SurfaceMeshComp = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("SurfaceMeshComponent"));
	BodyMeshComp = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("BodyMeshComponent"));
	DepthCaptureComp = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("DepthCaptureComponent"));

	RootComponent = BoxCollisionComp;
	SurfaceMeshComp->SetupAttachment(RootComponent);
	BodyMeshComp->SetupAttachment(RootComponent);
	DepthCaptureComp->SetupAttachment(RootComponent);

	CellSize = 15.0f;
	BodyWidth = 512.0f;
	BodyHeight = 512.0f;
	BodyDepth = 512.0f;

	BoxCollisionComp->SetBoxExtent(FVector(BodyWidth, BodyHeight, BodyDepth));
	BoxCollisionComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	BoxCollisionComp->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);

	DepthCaptureComp->RelativeRotation = FRotator(-90.0f, 0.0f, 270.0f);
	DepthCaptureComp->ProjectionType = ECameraProjectionMode::Orthographic;
	DepthCaptureComp->OrthoWidth = BodyWidth;
	DepthCaptureComp->PostProcessBlendWeight = 0.0f;
	DepthCaptureComp->bOverride_CustomNearClippingPlane = true;
	DepthCaptureComp->CustomNearClippingPlane = 0.0f;
	DepthCaptureComp->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;
	DepthCaptureComp->CaptureSource = ESceneCaptureSource::SCS_SceneDepth;
	DepthCaptureComp->MaxViewDistanceOverride = BodyDepth;

	LiquidParam.DepthTextureWidth = 128;
	LiquidParam.DepthTextureHeight = 128;
	LiquidParam.Viscosity = 0.15f;
	LiquidParam.Velocity = 0.5426512f;
	LiquidParam.ForceFactor = 1.49f;

	GenerateSurfaceMesh();
	GenerateBodyMesh();
}

// Called when the game starts or when spawned
void ACausticBody::BeginPlay()
{
	Super::BeginPlay();

	uint32 TextureWidth = LiquidParam.DepthTextureWidth;
	uint32 TextureHeight = LiquidParam.DepthTextureHeight;

	DepthRenderTarget = UKismetRenderingLibrary::CreateRenderTarget2D(GetWorld(), TextureWidth, TextureHeight, RTF_R16f);
	DepthCaptureComp->TextureTarget = DepthRenderTarget;

	FSurfaceDepthPassConfig Config;
	Config.MinDepth = 0.0f;
	Config.MaxDepth = BodyDepth;
	Config.TextureWidth = TextureWidth;
	Config.TextureHeight = TextureHeight;
	Config.DepthDebugTextureRef = SurfaceDepthPassDebugTexture;
	Config.HeightDebugTextureRef = SurfaceHeightPassDebugTexture;

	SurfaceDepthPassRenderer->InitPass(Config);
}

void ACausticBody::GenerateSurfaceMesh()
{
	const int32 SizeX = FMath::RoundToInt(BodyWidth / CellSize);
	const int32 SizeY = FMath::RoundToInt(BodyHeight / CellSize);
	const float CellWidth = BodyWidth / SizeX;
	const float CellHeight = BodyHeight / SizeY;
	const float CellU = 1.0f / SizeX;
	const float CellV = 1.0f / SizeY;
	const int VertexCount = (SizeX + 1) * (SizeY + 1);
	const int TriangleCount = SizeX * SizeY * 6;

	SurfaceMeshComp->ClearMeshSection(0);

	TArray<FVector> Vertices;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<int32> Triangles;

	Vertices.Reserve(VertexCount);
	Normals.Reserve(VertexCount);
	UVs.Reserve(VertexCount);
	Triangles.Reserve(TriangleCount);

	for (int32 Y = 0; Y <= SizeY; ++Y)
	{
		for (int32 X = 0; X <= SizeX; ++X)
		{
			float LocX = -BodyWidth * 0.5f + Y * CellWidth;
			float LocY = -BodyHeight * 0.5f + X * CellHeight;
			float LocZ = 0.0f;

			float U = Y * CellU;
			float V = X * CellV;

			Vertices.Add(FVector(LocX, LocY, LocZ));
			Normals.Add(FVector::UpVector);
			UVs.Add(FVector2D(U, V));
		}
	}

	for (int32 Y = 0; Y < SizeY; ++Y)
	{
		for (int32 X = 0; X < SizeX; ++X)
		{
			int A = Y * (SizeX + 1) + X;
			int B = A + SizeX + 1;
			int C = A + SizeX + 2;
			int D = A + 1;

			Triangles.Add(A);
			Triangles.Add(C);
			Triangles.Add(B);

			Triangles.Add(A);
			Triangles.Add(D);
			Triangles.Add(C);
		}
	}

	TArray<FLinearColor> EmptyVertexColor;
	TArray<FProcMeshTangent> EmptyTangent;

	SurfaceMeshComp->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, EmptyVertexColor, EmptyTangent, false);
}

void ACausticBody::GenerateBodyMesh()
{
	const int32 SizeX = FMath::RoundToInt(BodyWidth / CellSize);
	const int32 SizeY = FMath::RoundToInt(BodyHeight / CellSize);
	const float CellWidth = BodyWidth / SizeX;
	const float CellHeight = BodyHeight / SizeY;
	const float CellU = 1.0f / SizeX;
	const float CellV = 1.0f / SizeY;
	const float HalfWidth = BodyWidth * 0.5f;
	const float HalfHeight = BodyHeight * 0.5f;
	const FLinearColor WhiteOneAlpha(1.0f, 1.0f, 1.0f, 1.0f);
	const FLinearColor WhiteZeroAlpha(1.0f, 1.0f, 1.0f, 0.0f);

	const int32 VertexCount = 4 * (SizeX + SizeY + 2);
	const int32 TriangleCount = 12 * (SizeX + SizeY);

	BodyMeshComp->ClearMeshSection(0);

	TArray<FVector> Vertices;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FLinearColor> VertexColors;
	TArray<int32> Triangles;

	Vertices.Reserve(VertexCount);
	Normals.Reserve(VertexCount);
	UVs.Reserve(VertexCount);
	VertexColors.Reserve(VertexCount);
	Triangles.Reserve(TriangleCount);

	for (int32 Y = 0; Y <= SizeY; ++Y)
	{
		float OffsetY = Y * CellHeight;
		float OffsetV = Y * CellV;

		Vertices.Add(FVector(-HalfWidth, OffsetY - HalfHeight, -BodyDepth));
		Vertices.Add(FVector(-HalfWidth, OffsetY - HalfHeight, 0.0f));
		Vertices.Add(FVector(HalfWidth, OffsetY - HalfHeight, -BodyDepth));
		Vertices.Add(FVector(HalfWidth, OffsetY - HalfHeight, 0.0f));

		Normals.Add(FVector::LeftVector);
		Normals.Add(FVector::LeftVector);
		Normals.Add(FVector::RightVector);
		Normals.Add(FVector::RightVector);

		UVs.Add(FVector2D(0.0f, OffsetV));
		UVs.Add(FVector2D(0.0f, OffsetV));
		UVs.Add(FVector2D(1.0f, OffsetV));
		UVs.Add(FVector2D(1.0f, OffsetV));

		VertexColors.Add(WhiteOneAlpha);
		VertexColors.Add(WhiteZeroAlpha);
		VertexColors.Add(WhiteOneAlpha);
		VertexColors.Add(WhiteZeroAlpha);
	}

	for (int32 X = 0; X <= SizeX; ++X)
	{
		float OffsetX = X * CellWidth;
		float OffsetU = X * CellU;

		Vertices.Add(FVector(OffsetX - HalfWidth, -HalfHeight, -BodyDepth));
		Vertices.Add(FVector(OffsetX - HalfWidth, -HalfHeight, 0.0f));
		Vertices.Add(FVector(OffsetX - HalfWidth, HalfHeight, -BodyDepth));
		Vertices.Add(FVector(OffsetX - HalfWidth, HalfHeight, 0.0f));

		Normals.Add(FVector::BackwardVector);
		Normals.Add(FVector::BackwardVector);
		Normals.Add(FVector::ForwardVector);
		Normals.Add(FVector::ForwardVector);

		UVs.Add(FVector2D(OffsetU, 0.0f));
		UVs.Add(FVector2D(OffsetU, 0.0f));
		UVs.Add(FVector2D(OffsetU, 1.0f));
		UVs.Add(FVector2D(OffsetU, 1.0f));

		VertexColors.Add(WhiteOneAlpha);
		VertexColors.Add(WhiteZeroAlpha);
		VertexColors.Add(WhiteOneAlpha);
		VertexColors.Add(WhiteZeroAlpha);
	}

	for (int Y = 0; Y < SizeY; ++Y)
	{
		int A = Y * 4;
		int B = A + 5;
		int C = A + 1;
		int D = A + 4;
		int E = A + 6;
		int F = A + 3;
		int G = A + 7;
		int H = A + 2;

		Triangles.Add(A);
		Triangles.Add(B);
		Triangles.Add(C);

		Triangles.Add(A);
		Triangles.Add(D);
		Triangles.Add(B);

		Triangles.Add(E);
		Triangles.Add(F);
		Triangles.Add(G);

		Triangles.Add(E);
		Triangles.Add(H);
		Triangles.Add(F);
	}

	for (int X = 0; X < SizeX; ++X)
	{
		int A = (SizeY + 1) * 4 + X * 4;
		int B = A + 1;
		int C = A + 5;
		int D = A + 4;
		int E = A + 6;
		int F = A + 7;
		int G = A + 3;
		int H = A + 2;

		Triangles.Add(A);
		Triangles.Add(B);
		Triangles.Add(C);

		Triangles.Add(A);
		Triangles.Add(C);
		Triangles.Add(D);

		Triangles.Add(E);
		Triangles.Add(F);
		Triangles.Add(G);

		Triangles.Add(E);
		Triangles.Add(G);
		Triangles.Add(H);
	}

	TArray<FProcMeshTangent> EmptyTangent;

	BodyMeshComp->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, VertexColors, EmptyTangent, false);
}

// Called every frame
void ACausticBody::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FRHITexture* DepthTextureRef = DepthRenderTarget->TextureReference.TextureReferenceRHI->GetTextureReference()->GetReferencedTexture();
	SurfaceDepthPassRenderer->Render(LiquidParam, DepthTextureRef);
}

