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
	UMaterialInstanceDynamic* Cpp_DynamicStaminaBar;
	
	UPROPERTY(BlueprintReadWrite)
	UMaterialInstanceDynamic* Cpp_DynamicStaminaDamageBar;

	UPROPERTY(BlueprintReadWrite)
	UMaterialInstanceDynamic* Cpp_DynamicFortifyFill;

	
	
	UPROPERTY(BlueprintReadWrite)
	float Cpp_TargetDodgeStamina;
	
	UPROPERTY(BlueprintReadOnly)
	float Cpp_DamageIndicatorLerpTime = 4.f;

	UPROPERTY(BlueprintReadOnly)
	float Cpp_FortifyLerpTime = 6.f;

	UFUNCTION(BlueprintCallable)
	void Cpp_UpdateDodgeStaminaDamageBar();

	UFUNCTION(BlueprintCallable)
	void Cpp_UpdateDodgeStamina(const float& Stamina, const float& MaxStamina);

	UFUNCTION(BlueprintCallable)
    void Cpp_UpdateDodgeStaminaDecrementScalar(const float& DecrementScalar);
	
};
