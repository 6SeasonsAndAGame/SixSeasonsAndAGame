// None

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TravelGameModeBase.generated.h"

/**
 * 
 */
UCLASS()
class CTG_API ATravelGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
	

 UFUNCTION(BlueprintCallable, Category = "Networking", meta = (DisplayName = "Server Travel"))
     void MyServerTravel(FString mapName, FString gameMode, bool bAbsolute, FString additionalOptions);
 
};
