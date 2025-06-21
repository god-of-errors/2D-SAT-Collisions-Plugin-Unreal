#pragma once


#include <vector>

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "SATCollisionSubsystem.generated.h"

class APolygon2D;
class ACircle2D;

UCLASS()
class USATCollisionSubsystem : public UWorldSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(USATCollisionSubsystem, STATGROUP_Tickables); }
	virtual bool IsTickable() const override { return true; }
	virtual bool IsTickableWhenPaused() const override { return true; }
	virtual bool IsTickableInEditor() const override { return true; }

	// Automatically initialize
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override { return true; }
	
	void RegisterPolygon(APolygon2D* Polygon);
	void UnregisterPolygon(APolygon2D* Polygon);
	void RegisterCircle(ACircle2D* Circle);
	void UnregisterCircle(ACircle2D* Circle);

	static void ProjectPolygonOntoAxis(const TArray<FVector2D>& Points, const FVector2D& Axis, float& OutMin, float& OutMax);

private:
	TArray<TWeakObjectPtr<APolygon2D>> PolygonActors;
	TArray<TWeakObjectPtr<ACircle2D>> CircleActors;

	bool CheckSATCollision(const APolygon2D& A, const FTransform& TransformA,
						   const APolygon2D& B, const FTransform& TransformB,
						   FVector2D& OutAxis, float& OutOverlap);

	FVector2D GetPolygonMTV(const APolygon2D& A, const FTransform& TransformA,
										 const APolygon2D& B, const FTransform& TransformB);

	void ApplyPolygonMTV(APolygon2D& A, const FTransform& TransformA,
	                     APolygon2D& B, const FTransform& TransformB);


	bool CheckCircleCollision(const ACircle2D& A, const FTransform& TransformA,
	                          const ACircle2D& B, const FTransform& TransformB,
	                          FVector2D& OutAxis, float& OutOverlap);

	void ApplyCircleCircleMTV(ACircle2D& A, ACircle2D& B,
								const FVector2D& Axis, float Overlap);

	bool CheckCirclePolygonCollision(const ACircle2D& Circle, const FTransform& TransformCircle,
	                                 const APolygon2D& Polygon, const FTransform& TransformPolygon,
	                                 FVector2D& OutAxis, float& OutOverlap);

	void ApplyCirclePolygonMTV(ACircle2D& Circle, APolygon2D& Polygon,
							   const FVector2D& Axis, float Overlap);

	TSet<TPair<AActor*, AActor*>> PreviousOverlaps;

	static TPair<AActor*, AActor*> MakeSortedPair(AActor* A, AActor* B)
	{
		return A < B ? TPair<AActor*, AActor*>(A, B) : TPair<AActor*, AActor*>(B, A);
	}
};