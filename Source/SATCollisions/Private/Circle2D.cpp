#include "Circle2D.h"

#include "DrawDebugHelpers.h"
#include "SATCollisionSubsystem.h"

ACircle2D::ACircle2D()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
}

void ACircle2D::BeginPlay()
{
	Super::BeginPlay();

	if (auto* SAT = GetWorld()->GetSubsystem<USATCollisionSubsystem>())
	{
		SAT->RegisterCircle(this);
	}
	
	OnSATOverlapBeginDelegate.AddDynamic(this, &ACircle2D::HandleSATOverlapBegin);
	OnSATOverlapEndDelegate.AddDynamic(this, &ACircle2D::HandleSATOverlapEnd);
}

void ACircle2D::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (auto* SAT = GetWorld()->GetSubsystem<USATCollisionSubsystem>())
	{
		SAT->UnregisterCircle(this);
	}
}

void ACircle2D::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	
	DrawDebugCircle2D();
}

void ACircle2D::SetCircle(float InX, float InY, float InRadius)
{
	X = InX;
	Y = InY;
	Radius = InRadius;
}

void ACircle2D::HandleSATOverlapBegin(AActor* OtherActor)
{
	OnSATOverlapBegin(OtherActor);  // Calls virtual function
	UE_LOG(LogTemp, Warning, TEXT("%s began overlapping with %s"), *GetName(), *OtherActor->GetName());
}

void ACircle2D::HandleSATOverlapEnd(AActor* OtherActor)
{
	OnSATOverlapEnd(OtherActor);  // Calls virtual function
	UE_LOG(LogTemp, Warning, TEXT("%s ended overlapping with %s"), *GetName(), *OtherActor->GetName());
}

void ACircle2D::OnSATOverlapBegin_Implementation(AActor* OtherActor)
{
}

void ACircle2D::OnSATOverlapEnd_Implementation(AActor* OtherActor)
{
}

void ACircle2D::DrawDebugCircle2D() const
{
	if (Segments<3 || Radius <= 0 || !GetWorld()) return;
	
	FVector Center = GetActorLocation() + FVector(0, 0, ZOffset);
    
	float AngleStep = 2 * PI / Segments;
	FVector LastPoint = Center + FVector(Radius, 0, 0);

	for (int32 i = 1; i <= Segments; ++i)
	{
		float Angle = i * AngleStep;

		FVector ThisPoint = Center + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0);
        
		DrawDebugLine(GetWorld(), LastPoint, ThisPoint, ShapeColor, false, 1.0f, 0, 1.0f);
		LastPoint = ThisPoint;
	}
}
