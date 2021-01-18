// None

#pragma once

#include "CoreMinimal.h"

#include "PaintballCharacter.h"
#include "Blueprint/UserWidget.h"
#include "PaintballWB_Gameplay.generated.h"

/**
 * 
 */
UCLASS()
class CTG_API UPaintballWB_Gameplay : public UUserWidget
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite)
	class APaintballCharacter* Cpp_PawnOwner;
	
	UPROPERTY(BlueprintReadWrite)
	UMaterialInstanceDynamic* Cpp_DynamicDamageIndicator;

	UPROPERTY(BlueprintReadWrite)
	float Cpp_TargetDodgeStamina;
	
	UPROPERTY(BlueprintReadOnly)
	float Cpp_DamageIndicatorLerpTime = 4.f;

	UFUNCTION(BlueprintCallable)
	void Cpp_UpdateDodgeStaminaDamageIndicator();

	UFUNCTION(BlueprintImplementableEvent)
	void Cpp_UpdateDodgeStamina();
	
};
