// None


// ReSharper disable CppMemberFunctionMayBeConst
#include "Zombies/SpawnerRegulator.h"
#include "PaintballCharacter.h"
#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"

ASpawnerRegulator::ASpawnerRegulator()
{
	PrimaryActorTick.bCanEverTick = true;

	OuterCollider = CreateDefaultSubobject<UBoxComponent>(TEXT("OuterCollider"));
	SetRootComponent(OuterCollider);
	
	InnerCollider = CreateDefaultSubobject<UBoxComponent>(TEXT("InnerCollider"));
    InnerCollider->SetupAttachment(OuterCollider);
	InnerCollider->SetRelativeLocation(FVector(250.0f, 0.0f, 0.0f));

	ArrowComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("InnerDirectionArrow"));
	ArrowComponent->SetupAttachment(InnerCollider);
	
	InnerCollider->OnComponentBeginOverlap.AddDynamic(this, &ASpawnerRegulator::InnerCollisionReceived);
	OuterCollider->OnComponentBeginOverlap.AddDynamic(this, &ASpawnerRegulator::OuterCollisionReceived);

	SetActorScale3D(FVector(1, 3, 3));
}

void ASpawnerRegulator::BeginPlay()
{
	Super::BeginPlay();
	
}

void ASpawnerRegulator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ASpawnerRegulator::OuterCollisionReceived(UPrimitiveComponent* OnComponentBeginOverlap, AActor* OtherActor,
	UPrimitiveComponent* OverlappedComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	const APaintballCharacter* PaintballCharacter = Cast<APaintballCharacter>(OtherActor);
	if (PaintballCharacter)
	{
		// Check whether a player collision was detected
		if (PaintballCharacter->Tags.Contains(FName("Player")))
		{
			bOuterTriggered = true;
			if (bInnerTriggered)
			{
				GEngine->AddOnScreenDebugMessage(-1, 2, FColor::Green, TEXT("Regulator broadcast deactivate"));
				OnDeactivated.Broadcast();
			}
		}
	}
}

void ASpawnerRegulator::InnerCollisionReceived(UPrimitiveComponent* OnComponentBeginOverlap, AActor* OtherActor,
	UPrimitiveComponent* OverlappedComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	const APaintballCharacter* PaintballCharacter = Cast<APaintballCharacter>(OtherActor);
	if (PaintballCharacter)
	{
		// Check whether a player collision was detected
		if (PaintballCharacter->Tags.Contains(FName("Player")))
		{
			bInnerTriggered = true;
			if (bOuterTriggered)
			{
				GEngine->AddOnScreenDebugMessage(-1, 2, FColor::Green, TEXT("Regulator broadcast activate"));
				OnActivated.Broadcast();
			}
		}
	}
}
