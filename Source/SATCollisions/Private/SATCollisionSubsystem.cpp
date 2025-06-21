#include "SATCollisionSubsystem.h"

#include <vector>

#include "Polygon2D.h"
#include "Circle2D.h"

void USATCollisionSubsystem::RegisterPolygon(APolygon2D* Polygon)
{
	PolygonActors.AddUnique(Polygon);
}

void USATCollisionSubsystem::UnregisterPolygon(APolygon2D* Polygon)
{
	PolygonActors.Remove(Polygon);
}

void USATCollisionSubsystem::RegisterCircle(ACircle2D* Circle)
{
	CircleActors.AddUnique(Circle);
}

void USATCollisionSubsystem::UnregisterCircle(ACircle2D* Circle)
{
	CircleActors.Remove(Circle);
}

void USATCollisionSubsystem::ProjectPolygonOntoAxis(const TArray<FVector2D>& Points, const FVector2D& Axis,
	float& OutMin, float& OutMax)
{
	
	OutMin = FVector2D::DotProduct(Points[0], Axis);
	OutMax = OutMin;

	for (size_t i = 1; i < Points.Num(); ++i)
	{
		float Projection = FVector2D::DotProduct(Points[i], Axis);
		OutMin = FMath::Min(OutMin, Projection);
		OutMax = FMath::Max(OutMax, Projection);
	}
}

void USATCollisionSubsystem::Tick(float DeltaTime)
{
	TSet<AActor*> CollidingActors;
	TSet<TPair<AActor*, AActor*>> CurrentOverlaps;
	
	// polygon-vs-polygon collisions
	for (int32 i = 0; i < PolygonActors.Num(); ++i)
	{
		APolygon2D* A = PolygonActors[i].Get();
		if (!IsValid(A)) continue;
		if (A->Points.Num() < 3 || A->CollisionResponse == ESATCollisionResponse::Ignore) continue;

		for (int32 j = i + 1; j < PolygonActors.Num(); ++j)
		{
			APolygon2D* B = PolygonActors[j].Get();
			if (!IsValid(B)) continue;
			if (B->Points.Num() < 3 || B->CollisionResponse == ESATCollisionResponse::Ignore) continue;

			FVector2D Axis;
			float Overlap;
			if (CheckSATCollision(*A, A->GetActorTransform(),
								  *B, B->GetActorTransform(),
								  Axis, Overlap))
			{
				A->SetColliding(true);
				B->SetColliding(true);
				CollidingActors.Add(A);
				CollidingActors.Add(B);

				TPair<AActor*, AActor*> Pair = MakeSortedPair(A, B);
				CurrentOverlaps.Add(Pair);

				if (!PreviousOverlaps.Contains(Pair))
				{
					A->OnSATOverlapBeginDelegate.Broadcast(B);
					B->OnSATOverlapBeginDelegate.Broadcast(A);
				}
				
				if (A->CollisionResponse == ESATCollisionResponse::Block
					|| B->CollisionResponse == ESATCollisionResponse::Block)
				{
					ApplyPolygonMTV(*A, A->GetActorTransform(), *B, B->GetActorTransform());
				}
			}
		}
	}

	// circle-vs-circle collisions
	for (int32 i = 0; i < CircleActors.Num(); ++i)
	{
		ACircle2D* A = CircleActors[i].Get();
		if (!IsValid(A)) continue;
		if (A->Radius <= 0) continue;

		ESATCollisionResponse ResponseA = A->CollisionResponse;
		if (ResponseA == ESATCollisionResponse::Ignore) continue;
		
		for (int32 j = i + 1; j < CircleActors.Num(); ++j)
		{
			ACircle2D* B = CircleActors[j].Get();
			if (!IsValid(B)) continue;
			if (B->Radius <= 0) continue;

			ESATCollisionResponse ResponseB = B->CollisionResponse;
			if (ResponseB == ESATCollisionResponse::Ignore) continue;

			FVector2D Axis;
			float Overlap;
			if (CheckCircleCollision(*A, A->GetActorTransform(),
										*B, B->GetActorTransform(),
										Axis, Overlap)) // Assumes you have this
			{
				A->SetColliding(true);
				B->SetColliding(true);
				CollidingActors.Add(A);
				CollidingActors.Add(B);

				TPair<AActor*, AActor*> Pair = MakeSortedPair(A, B);
				CurrentOverlaps.Add(Pair);

				if (!PreviousOverlaps.Contains(Pair))
				{
					A->OnSATOverlapBeginDelegate.Broadcast(B);
					B->OnSATOverlapBeginDelegate.Broadcast(A);
				}
				
				if (ResponseA == ESATCollisionResponse::Block || ResponseB == ESATCollisionResponse::Block)
				{
					ApplyCircleCircleMTV(*A, *B, Axis, Overlap);
				}
			}
		}
	}
	
	// polygon-vs-circle collisions
	for (TWeakObjectPtr<APolygon2D> PolyActor : PolygonActors)
	{
		APolygon2D* Polygon = PolyActor.Get();
		if (!IsValid(Polygon)) continue;
		if (Polygon->Points.Num() < 3) continue;

		ESATCollisionResponse PolygonResponse = Polygon->CollisionResponse;
		if (PolygonResponse == ESATCollisionResponse::Ignore) continue;

		for (TWeakObjectPtr<ACircle2D> CircleActor : CircleActors)
		{
			ACircle2D* Circle = CircleActor.Get();
			if (!IsValid(Circle)) continue;
			if (Circle->Radius <= 0) continue;

			ESATCollisionResponse CircleResponse = Circle->CollisionResponse;
			if (CircleResponse == ESATCollisionResponse::Ignore) continue;

			FVector2D Axis;
			float Overlap;
			if (CheckCirclePolygonCollision(*Circle, Circle->GetTransform(),
											*Polygon, Polygon->GetActorTransform(),
											Axis, Overlap)) // Assumes this too
			{
				Polygon->SetColliding(true);
				Circle->SetColliding(true);
				CollidingActors.Add(Polygon);
				CollidingActors.Add(Circle);

				TPair<AActor*, AActor*> Pair = MakeSortedPair(Polygon, Circle);
				CurrentOverlaps.Add(Pair);

				if (!PreviousOverlaps.Contains(Pair))
				{
					Polygon->OnSATOverlapBeginDelegate.Broadcast(Circle);
					Circle->OnSATOverlapBeginDelegate.Broadcast(Polygon);
				}
				
				if (CircleResponse == ESATCollisionResponse::Block || PolygonResponse == ESATCollisionResponse::Block)
				{
					ApplyCirclePolygonMTV(*Circle, *Polygon, Axis, Overlap);
				}
			}
		}
	}

	// End overlaps
	for (const TPair<AActor*, AActor*>& Pair : PreviousOverlaps)
	{
		if (!CurrentOverlaps.Contains(Pair))
		{
			AActor* A = Pair.Key;
			AActor* B = Pair.Value;

			if (IsValid(A))
			{
				if (auto Poly = Cast<APolygon2D>(A))
				{
					Poly->OnSATOverlapBeginDelegate.Broadcast(B);
				}
				else if (auto Circle = Cast<ACircle2D>(A))
				{
					Circle->OnSATOverlapBeginDelegate.Broadcast(B);
				}
			}

			if (IsValid(B))
			{
				if (auto Poly = Cast<APolygon2D>(B))
				{
					Poly->OnSATOverlapEndDelegate.Broadcast(A);
				}
				else if (auto Circle = Cast<ACircle2D>(B))
				{
					Circle->OnSATOverlapEndDelegate.Broadcast(A);
				}
			}
		}
	}
	
	PreviousOverlaps = CurrentOverlaps;
	
	for (TWeakObjectPtr<APolygon2D> PolyActor : PolygonActors)
	{
		APolygon2D* Polygon = PolyActor.Get();
		if (IsValid(Polygon) && !CollidingActors.Contains(Polygon))
		{
			Polygon->SetColliding(false);
		}
	}

	for (TWeakObjectPtr<ACircle2D> CircleActor : CircleActors)
	{
		ACircle2D* Circle = CircleActor.Get();
		if (IsValid(Circle) && !CollidingActors.Contains(Circle))
		{
			Circle->SetColliding(false);
		}
	}
}

bool USATCollisionSubsystem::CheckSATCollision(const APolygon2D& A, const FTransform& TransformA, const APolygon2D& B,
	const FTransform& TransformB, FVector2D& OutAxis, float& OutOverlap)
{
	float MinA, MaxA, MinB, MaxB;
	float SmallestOverlap = TNumericLimits<float>::Max();
	bool bIsColliding = false;

	const TArray<FVector2D>& PointsA = A.GetTransformedPoints(TransformA);
	const TArray<FVector2D>& PointsB = B.GetTransformedPoints(TransformB);

	std::vector<FVector2D> Normals;

	// Get normals for A
	for (size_t i = 0; i < PointsA.Num(); ++i)
	{
		FVector2D P1 = PointsA[i];
		FVector2D P2 = PointsA[(i + 1) % PointsA.Num()];
		FVector2D Edge = P2 - P1;
		FVector2D Normal(-Edge.Y, Edge.X);
		Normals.push_back(Normal.GetSafeNormal());
	}

	// Get normals for B
	for (size_t i = 0; i < PointsB.Num(); ++i)
	{
		FVector2D P1 = PointsB[i];
		FVector2D P2 = PointsB[(i + 1) % PointsB.Num()];
		FVector2D Edge = P2 - P1;
		FVector2D Normal(-Edge.Y, Edge.X);
		Normals.push_back(Normal.GetSafeNormal());
	}

	// Loop through all normals (separating axes)
	for (const FVector2D& Axis : Normals)
	{
		// Project both polygons onto the axis
		ProjectPolygonOntoAxis(PointsA, Axis, MinA, MaxA);
		ProjectPolygonOntoAxis(PointsB, Axis, MinB, MaxB);

		// If projections don't overlap, no collision
		if (MaxA < MinB || MaxB < MinA)
		{
			return false;
		}

		// Check for smallest overlap
		float Overlap = FMath::Min(MaxA, MaxB) - FMath::Max(MinA, MinB);
		if (Overlap < SmallestOverlap)
		{
			SmallestOverlap = Overlap;
			OutAxis = Axis;
			OutOverlap = SmallestOverlap;
			bIsColliding = true;
		}
	}
	
	return bIsColliding;
}

FVector2D USATCollisionSubsystem::GetPolygonMTV(const APolygon2D& A, const FTransform& TransformA, const APolygon2D& B,
	const FTransform& TransformB)
{
	FVector2D Axis;
	float Overlap = 0;

	if (!CheckSATCollision(A, TransformA, B, TransformB, Axis, Overlap))
	{
		return FVector2D::ZeroVector;
	}

	Axis.Normalize();
	FVector2D CentroidA = A.GetCentroidWorld();
	FVector2D CentroidB = B.GetCentroidWorld();

	FVector2D Direction = CentroidA - CentroidB;

	if (FVector2D::DotProduct(Axis, Direction) < 0)
	{
		Axis *= -1;
	}

	return Axis * Overlap;
}

void USATCollisionSubsystem::ApplyPolygonMTV(APolygon2D& A, const FTransform& TransformA, APolygon2D& B,
	const FTransform& TransformB)
{
	FVector2D MTV = GetPolygonMTV(A, TransformA, B, TransformB);
	if (MTV.IsNearlyZero()) return;

	FVector2D HalfMTV = 0.5f * MTV;

	A.TranslatePoints(HalfMTV);   // Implement this in APolygon2D to offset Points
	B.TranslatePoints(-HalfMTV);  // Same as above
}

bool USATCollisionSubsystem::CheckCircleCollision(const ACircle2D& A, const FTransform& TransformA, const ACircle2D& B,
                                                  const FTransform& TransformB, FVector2D& OutAxis, float& OutOverlap)
{
	// Get world positions of both circles
	const FVector WorldPosA = TransformA.TransformPosition(FVector(A.X, A.Y, 0.f));
	const FVector WorldPosB = TransformB.TransformPosition(FVector(B.X, B.Y, 0.f));
	const FVector2D CenterA(WorldPosA.X, WorldPosA.Y);
	const FVector2D CenterB(WorldPosB.X, WorldPosB.Y);

	const float RadiusA = A.Radius;
	const float RadiusB = B.Radius;

	const FVector2D Delta = CenterB - CenterA;
	const float DistanceSq = Delta.SizeSquared();
	const float RadiusSum = RadiusA + RadiusB;

	if (DistanceSq > RadiusSum * RadiusSum)
	{
		return false; // Not colliding
	}

	const float Distance = FMath::Sqrt(DistanceSq);

	// Avoid division by zero if circles are exactly overlapping
	if (Distance > KINDA_SMALL_NUMBER)
	{
		OutAxis = Delta / Distance;
		OutOverlap = RadiusSum - Distance;
	}
	else
	{
		// Circles are perfectly overlapping; choose an arbitrary MTV
		OutAxis = FVector2D(1, 0);
		OutOverlap = RadiusSum;
	}

	return true;
}

void USATCollisionSubsystem::ApplyCircleCircleMTV(ACircle2D& A, ACircle2D& B, const FVector2D& Axis, float Overlap)
{
	if (FMath::IsNearlyZero(Overlap)) return;

	FVector2D NormalizedAxis = Axis.GetSafeNormal();
	if (NormalizedAxis.IsNearlyZero()) return;

	FVector2D MTV = NormalizedAxis * Overlap;

	// Split movement evenly between both circles
	const FVector2D HalfMTV = 0.5f * MTV;

	A.AddActorWorldOffset(FVector(-HalfMTV, 0.0f));
	B.AddActorWorldOffset(FVector(HalfMTV, 0.0f));
}

void USATCollisionSubsystem::ApplyCirclePolygonMTV(ACircle2D& Circle, APolygon2D& Polygon, const FVector2D& Axis,
                                                   float Overlap)
{
	if (FMath::IsNearlyZero(Overlap)) return;

	FVector2D NormalizedAxis = Axis.GetSafeNormal();
	if (NormalizedAxis.IsNearlyZero()) return;

	FVector2D MTV = NormalizedAxis * Overlap;

	// Move circle in MTV direction (plus nudge)
	Circle.AddActorWorldOffset(FVector(MTV + (NormalizedAxis * 0.01f), 0.0f));

	// Push polygon in opposite direction
	FVector2D OpposingMTV = -MTV;
	Polygon.AddActorWorldOffset(FVector(OpposingMTV, 0.0f));
}

bool USATCollisionSubsystem::CheckCirclePolygonCollision(const ACircle2D& Circle, const FTransform& TransformCircle,
                                                         const APolygon2D& Polygon, const FTransform& TransformPolygon, FVector2D& OutAxis, float& OutOverlap)
{
	const FVector WorldCenter = TransformCircle.TransformPosition(FVector(Circle.X, Circle.Y, 0.f));
	const FVector2D CircleCenter(WorldCenter.X, WorldCenter.Y);
	const float Radius = Circle.Radius;

	const TArray<FVector2D> PolygonPoints = Polygon.GetTransformedPoints(TransformPolygon);
	const TArray<FVector2D> Normals = Polygon.GetNormalsTransformed(TransformPolygon);

	TArray<FVector2D> Axes = Normals;

	// Find closest polygon vertex to the circle center
	FVector2D ClosestVertex;
	float MinDistSq = TNumericLimits<float>::Max();
	for (const FVector2D& Point : PolygonPoints)
	{
		const float DistSq = FVector2D::DistSquared(CircleCenter, Point);
		if (DistSq < MinDistSq)
		{
			MinDistSq = DistSq;
			ClosestVertex = Point;
		}
	}

	// Add axis from circle center to closest vertex
	FVector2D CircleAxis = ClosestVertex - CircleCenter;
	if (!CircleAxis.IsNearlyZero())
	{
		Axes.Add(CircleAxis.GetSafeNormal());
	}

	bool bIsColliding = false;
	float SmallestOverlap = TNumericLimits<float>::Max();

	for (const FVector2D& Axis : Axes)
	{
		float MinA, MaxA, MinB, MaxB;

		// Project polygon onto axis
		ProjectPolygonOntoAxis(PolygonPoints, Axis, MinA, MaxA);

		// Project circle onto axis (circle projects to a range centered at dot product)
		const float CircleCenterProj = FVector2D::DotProduct(CircleCenter, Axis);
		MinB = CircleCenterProj - Radius;
		MaxB = CircleCenterProj + Radius;

		// Check for separation
		if (MaxA < MinB || MaxB < MinA)
		{
			return false; // Separating axis found
		}

		// Calculate overlap
		const float Overlap = FMath::Min(MaxA, MaxB) - FMath::Max(MinA, MinB);
		if (Overlap < SmallestOverlap)
		{
			SmallestOverlap = Overlap;
			OutAxis = Axis;
			OutOverlap = Overlap;
			bIsColliding = true;
		}
	}

	// Ensure Minimum Translation Vector points outward from circle
	if (bIsColliding)
	{
		const FVector2D Dir = CircleCenter - ClosestVertex;
		if (FVector2D::DotProduct(OutAxis, Dir) < 0)
		{
			OutAxis *= -1;
		}
	}

	return bIsColliding;
}
