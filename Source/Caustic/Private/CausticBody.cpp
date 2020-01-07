// Fill out your copyright notice in the Description page of Project Settings.


#include "CausticBody.h"
#include "ProceduralMeshComponent.h"

// Sets default values
ACausticBody::ACausticBody()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SurfaceComp = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("SurfaceComponent"));
	BodyComp = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("BodyComponent"));

	SurfaceComp->SetupAttachment(RootComponent);
	BodyComp->SetupAttachment(RootComponent);

	CellSize = 15.0f;
	BodyWidth = 500.0f;
	BodyHeight = 500.0f;
	BodyDepth = 500.0f;
}

// Called when the game starts or when spawned
void ACausticBody::BeginPlay()
{
	Super::BeginPlay();
	
}

void ACausticBody::GenerateSurfaceMesh()
{
	int32 SizeX = FMath::RoundToInt(BodyWidth / CellSize);
	int32 SizeY = FMath::RoundToInt(BodyHeight / CellSize);
	float CellWidth = BodyWidth / SizeX;
	float CellHeight = BodyHeight / SizeY;
	float CellU = 1.0f / SizeX;
	float CellV = 1.0f / SizeY;

	SurfaceComp->ClearAllMeshSections();

	TArray<FVector> Vertices;
	TArray<FVector> Normals;
	TArray<FVector2D> UV;
	TArray<int32> Triangles;

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
			UV.Add(FVector2D(U, V));
		}
	}

	for (int32 Y = 0; Y < SizeY; ++Y)
	{
		for (int32 X = 0; X < SizeX; ++X)
		{
			int A = Y * (SizeX + 1) + X;
			int B = A + SizeX + 1;
			int C = B + 1;
			int D = A + 1;

			Triangles.Add(D);
			Triangles.Add(A);
			Triangles.Add(B);

			Triangles.Add(D);
			Triangles.Add(B);
			Triangles.Add(C);
		}
	}

	TArray<FLinearColor> EmptyVertexColor;
	TArray<FProcMeshTangent> EmptyTangent;

	SurfaceComp->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UV, EmptyVertexColor, EmptyTangent, false);
}

void ACausticBody::GenerateBodyMesh()
{

}

// Called every frame
void ACausticBody::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

