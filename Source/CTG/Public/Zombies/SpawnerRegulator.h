// None

#pragma once

#include "CoreMinimal.h"
#include "SpawnerRegulator.generated.h"

class UBoxComponent;
class UArrowComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FActivated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDeactivated);

UCLASS()
class CTG_API ASpawnerRegulator final : public AActor
{
	GENERATED_BODY()

public:
	ASpawnerRegulator();

	virtual void Tick(float DeltaTime) override;

	// Delegate called when a player *enters* through the Regulator
	UPROPERTY(BlueprintReadWrite)
	FActivated OnActivated;

	// Delegate called when a player *exits* through the Regulator
	UPROPERTY(BlueprintReadWrite)
	FDeactivated OnDeactivated;
	
protected:
	virtual void BeginPlay() override;

	// The collider which should be placed farther from the spawner
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UBoxComponent* OuterCollider;

	// The collider which should be placed closer to the spawner
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UBoxComponent* InnerCollider;

	// Arrow is attached to the InnerCollider
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UArrowComponent* ArrowComponent;

	UFUNCTION()
	void OuterCollisionReceived(UPrimitiveComponent* OnComponentBeginOverlap, AActor* OtherActor,
		UPrimitiveComponent* OverlappedComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
	UFUNCTION()
	void InnerCollisionReceived(UPrimitiveComponent* OnComponentBeginOverlap, AActor* OtherActor,
		UPrimitiveComponent* OverlappedComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	bool bInnerTriggered = false;
	bool bOuterTriggered = false;
};
