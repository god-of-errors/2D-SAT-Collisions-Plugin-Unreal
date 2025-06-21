#include "Polygon2D.h"

#include "Circle2D.h"
#include "DrawDebugHelpers.h"
#include "SATCollisionSubsystem.h"

APolygon2D::APolygon2D()
{
	PrimaryActorTick.bCanEverTick = true;
	
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
}

void APolygon2D::BeginPlay()
{
	Super::BeginPlay();

	if (auto* SAT = GetWorld()->GetSubsystem<USATCollisionSubsystem>())
		SAT->RegisterPolygon(this);
	
	OnSATOverlapBeginDelegate.AddDynamic(this, &APolygon2D::HandleSATOverlapBegin);
	OnSATOverlapEndDelegate.AddDynamic(this, &APolygon2D::HandleSATOverlapEnd);
}

void APolygon2D::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	if (auto* SAT = GetWorld()->GetSubsystem<USATCollisionSubsystem>())
		SAT->UnregisterPolygon(this);
}

void APolygon2D::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	DrawDebugPolygon();
}

void APolygon2D::AddPoint(float X, float Y)
{
	Points.Add(FVector2D(X, Y));
}

void APolygon2D::SetPoint(int Index, float X, float Y)
{
	if (Points.IsValidIndex(Index))
	{
		Points[Index] = FVector2D(X, Y);
	}
}

void APolygon2D::ClearPolygon()
{
	Points.Empty();
}

TArray<FVector2D> APolygon2D::GetTransformedPoints(const FTransform& Transform) const
{
	TArray<FVector2D> Result;
	for (const FVector2D& LocalPoint : GetLocalPoints())
	{
		FVector World = Transform.TransformPosition(FVector(LocalPoint.X, LocalPoint.Y, 0.f));
		Result.Add(FVector2D(World.X, World.Y));
	}
	return Result;
}

TArray<FVector2D> APolygon2D::GetNormalsTransformed(const FTransform& Transform) const
{
	TArray<FVector2D> Normals;
	const TArray<FVector2D> TransformdPoints = GetTransformedPoints(Transform);

	for (int32 i = 0; i < TransformdPoints.Num(); ++i)
	{
		const FVector2D P1 = TransformdPoints[i];
		const FVector2D P2 = TransformdPoints[(i + 1) % TransformdPoints.Num()];
		const FVector2D Edge = P2 - P1;
		Normals.Add(FVector2D(-Edge.Y, Edge.X).GetSafeNormal());
	}
	return Normals;
}

void APolygon2D::TranslatePoints(const FVector2D& Offset)
{
	FVector WorldOffset(Offset, 0.0f);
	AddActorWorldOffset(WorldOffset);
}

FVector2D APolygon2D::GetCentroidWorld() const
{
	if (Points.Num() == 0)
	{
		return FVector2D::ZeroVector;
	}

	FVector2D Sum(0.0f, 0.0f);

	const FTransform& WorldTransform = GetActorTransform();

	for (const FVector2D& Point : Points)
	{
		FVector WorldPoint3D = WorldTransform.TransformPosition(FVector(Point, 0.0f));
		Sum.X += WorldPoint3D.X;
		Sum.Y += WorldPoint3D.Y;
	}

	return Sum / Points.Num();
}

void APolygon2D::HandleSATOverlapBegin(AActor* OtherActor)
{
	OnSATOverlapBegin(OtherActor);  // Calls virtual function
	UE_LOG(LogTemp, Warning, TEXT("%s began overlapping with %s"), *GetName(), *OtherActor->GetName());
}

void APolygon2D::HandleSATOverlapEnd(AActor* OtherActor)
{
	OnSATOverlapEnd(OtherActor);  // Calls virtual function
	UE_LOG(LogTemp, Warning, TEXT("%s ended overlapping with %s"), *GetName(), *OtherActor->GetName());
}

void APolygon2D::OnSATOverlapBegin_Implementation(AActor* OtherActor)
{
}

void APolygon2D::OnSATOverlapEnd_Implementation(AActor* OtherActor)
{
}

void APolygon2D::DrawDebugPolygon() const
{
	if (Points.Num() < 2) return;

	for (int i = 0; i < Points.Num(); ++i)
	{
		const FVector2D& Current = Points[i];
		const FVector2D& Next = Points[(i + 1) % Points.Num()];

		FVector Start = GetActorTransform().TransformPosition(FVector(Current.X, Current.Y, 0.f));
		FVector End   = GetActorTransform().TransformPosition(FVector(Next.X, Next.Y, 0.f));

		DrawDebugLine(GetWorld(), Start, End, ShapeColor, false, 1.f, 0, 2.f);
	}
}

