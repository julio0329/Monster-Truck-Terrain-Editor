#include "RampActor.h"
#include "ProceduralMeshComponent.h"

namespace
{
	/** Small helper that builds a closed mesh and fixes triangle winding per face. */
	struct FRampMeshBuilder
	{
		TArray<FVector> Verts;
		TArray<FVector> Normals;
		TArray<FVector2D> UVs;
		TArray<int32> Tris;

		int32 AddVert(const FVector& P, const FVector& N, const FVector2D& UV)
		{
			Verts.Add(P);
			Normals.Add(N);
			UVs.Add(UV);
			return Verts.Num() - 1;
		}

		/**
		 * Adds a triangle whose front face points toward OutwardHint.
		 * Unreal treats the side where the cross product (B-A) x (C-A) points as the
		 * front face, so the winding is flipped when that points away from the hint.
		 */
		void AddTri(int32 A, int32 B, int32 C, const FVector& OutwardHint)
		{
			const FVector N = FVector::CrossProduct(Verts[B] - Verts[A], Verts[C] - Verts[A]);
			if (N.SizeSquared() < 1e-4f)
			{
				return; // degenerate (zero area), skip so collision cooking stays clean
			}
			if (FVector::DotProduct(N, OutwardHint) >= 0.f)
			{
				Tris.Add(A); Tris.Add(B); Tris.Add(C);
			}
			else
			{
				Tris.Add(A); Tris.Add(C); Tris.Add(B);
			}
		}

		/** Quad given as a loop of four vertices around its perimeter. */
		void AddQuad(int32 A, int32 B, int32 C, int32 D, const FVector& OutwardHint)
		{
			AddTri(A, B, C, OutwardHint);
			AddTri(A, C, D, OutwardHint);
		}
	};
}

ARampActor::ARampActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("RampMesh"));
	SetRootComponent(Mesh);
	Mesh->bUseAsyncCooking = true;
	Mesh->bUseComplexAsSimpleCollision = true;
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));
}

void ARampActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Rebuild();
}

void ARampActor::Rebuild()
{
	const float L = FMath::Max(Params.Length, 10.f);
	const float W = FMath::Max(Params.Width, 10.f);
	const float H = FMath::Max(Params.Height, 1.f);
	const float P = FMath::Clamp(Params.CurvePower, 0.5f, 6.f);
	const int32 N = FMath::Clamp(Params.LengthSegments, 2, 128);
	const float HalfW = W * 0.5f;

	// Surface height and slope (dz/dx) at normalized position T in [0, 1].
	auto SurfaceZ = [&](float T) { return H * FMath::Pow(T, P); };
	auto SurfaceSlope = [&](float T)
	{
		return H * P * FMath::Pow(FMath::Max(T, KINDA_SMALL_NUMBER), P - 1.f) / L;
	};

	FRampMeshBuilder B;

	// Top surface: two vertices (left/right) per station along the length.
	TArray<int32> TopL, TopR;
	for (int32 i = 0; i <= N; ++i)
	{
		const float T = static_cast<float>(i) / N;
		const float X = T * L;
		const float Z = SurfaceZ(T);
		const FVector Normal = FVector(-SurfaceSlope(T), 0.f, 1.f).GetSafeNormal();
		TopL.Add(B.AddVert(FVector(X, -HalfW, Z), Normal, FVector2D(T, 0.f)));
		TopR.Add(B.AddVert(FVector(X, HalfW, Z), Normal, FVector2D(T, 1.f)));
	}
	for (int32 i = 0; i < N; ++i)
	{
		B.AddQuad(TopL[i], TopL[i + 1], TopR[i + 1], TopR[i], FVector::UpVector);
	}

	// Side walls, closing the solid between the ground (Z = 0) and the top surface.
	auto AddSide = [&](float Y, const FVector& Normal)
	{
		TArray<int32> Bottom, Top;
		for (int32 i = 0; i <= N; ++i)
		{
			const float T = static_cast<float>(i) / N;
			const float X = T * L;
			Bottom.Add(B.AddVert(FVector(X, Y, 0.f), Normal, FVector2D(T, 0.f)));
			Top.Add(B.AddVert(FVector(X, Y, SurfaceZ(T)), Normal, FVector2D(T, 1.f)));
		}
		for (int32 i = 0; i < N; ++i)
		{
			B.AddQuad(Bottom[i], Bottom[i + 1], Top[i + 1], Top[i], Normal);
		}
	};
	AddSide(-HalfW, FVector(0.f, -1.f, 0.f));
	AddSide(HalfW, FVector(0.f, 1.f, 0.f));

	// Back wall at the launch lip.
	{
		const FVector Normal(1.f, 0.f, 0.f);
		const int32 A = B.AddVert(FVector(L, -HalfW, 0.f), Normal, FVector2D(0.f, 0.f));
		const int32 Bv = B.AddVert(FVector(L, HalfW, 0.f), Normal, FVector2D(1.f, 0.f));
		const int32 C = B.AddVert(FVector(L, HalfW, H), Normal, FVector2D(1.f, 1.f));
		const int32 D = B.AddVert(FVector(L, -HalfW, H), Normal, FVector2D(0.f, 1.f));
		B.AddQuad(A, Bv, C, D, Normal);
	}

	// Bottom face.
	{
		const FVector Normal(0.f, 0.f, -1.f);
		const int32 A = B.AddVert(FVector(0.f, -HalfW, 0.f), Normal, FVector2D(0.f, 0.f));
		const int32 Bv = B.AddVert(FVector(L, -HalfW, 0.f), Normal, FVector2D(1.f, 0.f));
		const int32 C = B.AddVert(FVector(L, HalfW, 0.f), Normal, FVector2D(1.f, 1.f));
		const int32 D = B.AddVert(FVector(0.f, HalfW, 0.f), Normal, FVector2D(0.f, 1.f));
		B.AddQuad(A, Bv, C, D, Normal);
	}

	Mesh->ClearAllMeshSections();
	Mesh->CreateMeshSection(0, B.Verts, B.Tris, B.Normals, B.UVs,
		TArray<FColor>(), TArray<FProcMeshTangent>(), true);
}
