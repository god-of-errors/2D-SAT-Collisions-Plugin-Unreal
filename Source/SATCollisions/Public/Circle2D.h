#pragma once

#include "CoreMinimal.h"
#include "SATCollisionTypes.h"
#include "GameFramework/Actor.h"
#include "Circle2D.generated.h"

UCLASS()
class ACircle2D : public AActor
{
	GENERATED_BODY()

public:
	ACircle2D();

protected:
	virtual void BeginPlay() override;

	void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void Tick(float DeltaTime) override;

	UPROPERTY()
	USceneComponent* Root;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Circle")
	float X = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Circle")
	float Y = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Circle")
	float Radius = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Circle Debug")
	int32 Segments = 32;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Circle Debug")
	FColor ShapeColor = FColor::Green;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Circle Debug")
	float ZOffset = 10.f;

	UFUNCTION(BlueprintCallable, Category = "Circle")
	void SetCircle(float InX, float InY, float InRadius);

	void SetColliding(bool bColliding)
	{
		ShapeColor = bColliding ? FColor::Red : FColor::Green;
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SAT Collision")
	ESATCollisionResponse CollisionResponse = ESATCollisionResponse::Overlap;

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

	void DrawDebugCircle2D() const;
};
