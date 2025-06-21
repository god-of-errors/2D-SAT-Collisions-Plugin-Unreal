#pragma once

#include <vector>

#include "CoreMinimal.h"
#include "SATCollisionTypes.h"
#include "GameFramework/Actor.h"
#include "Polygon2D.generated.h"

UCLASS()
class APolygon2D : public AActor
{
	GENERATED_BODY()

public:
	APolygon2D();

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void Tick(float DeltaTime) override;

	UPROPERTY()
	USceneComponent* Root;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Polygon")
	TArray<FVector2D> Points;

	UFUNCTION(BlueprintCallable, Category = "Polygon")
	void AddPoint(float X, float Y);

	UFUNCTION(BlueprintCallable, Category = "Polygon")
	void SetPoint(int Index, float X, float Y);

	UFUNCTION(BlueprintCallable, Category = "Polygon")
	void ClearPolygon();

	UPROPERTY(EditAnywhere)
	FColor ShapeColor = FColor::Green;

	void SetColliding(bool bColliding)
	{
		ShapeColor = bColliding ? FColor::Red : FColor::Green;
	}

	TArray<FVector2D> GetLocalPoints() const
	{
		return Points;
	}

	TArray<FVector2D> GetTransformedPoints(const FTransform& Transform) const;
	TArray<FVector2D> GetNormalsTransformed(const FTransform& Transform) const;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SAT Collision")
	ESATCollisionResponse CollisionResponse = ESATCollisionResponse::Overlap;
	
	void TranslatePoints(const FVector2D& Offset);
	FVector2D GetCentroidWorld() const;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSATOverlapEvent, AActor*, OtherActor);
	
	UPROPERTY(BlueprintAssignable, Category = "SAT Collision")
	FSATOverlapEvent OnSATOverlapBeginDelegate;

	UPROPERTY(BlueprintAssignable, Category = "SAT Collision")
	FSATOverlapEvent OnSATOverlapEndDelegate;
	
	UFUNCTION()
	void HandleSATOverlapBegin(AActor* OtherActor);

	UFUNCTION()
	void HandleSATOverlapEnd(AActor* OtherActor);
	
protected:
	UFUNCTION(BlueprintNativeEvent, Category = "SAT Collision")
	void OnSATOverlapBegin(AActor* OtherActor);
	virtual void OnSATOverlapBegin_Implementation(AActor* OtherActor);

	UFUNCTION(BlueprintNativeEvent, Category = "SAT Collision")
	void OnSATOverlapEnd(AActor* OtherActor);
	virtual void OnSATOverlapEnd_Implementation(AActor* OtherActor);

private:
	void DrawDebugPolygon() const;
};
