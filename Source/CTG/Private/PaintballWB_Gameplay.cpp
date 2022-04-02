// None


#include "CTG/Public/PaintballWB_Gameplay.h"

// Fits PawnOwner->DodgeStamina to 01 and sets DynamicStaminaBar to that value
void UPaintballWB_Gameplay::Cpp_UpdateDodgeStamina(const float& Stamina, const float& MaxStamina)
{
	if (!Cpp_PawnOwner || !Cpp_DynamicStaminaBar)
	{
		return;
	}
	
	const float PawnOwnerDodgeStamina = Stamina;
	float DodgeStaminaTo01 = 0.f;
	
	// avoid division by zero
	if (PawnOwnerDodgeStamina != 0.f)
	{
		// fit to 01
		DodgeStaminaTo01 = PawnOwnerDodgeStamina / MaxStamina;
	}

	// save as member so that the (red) DynamicStaminaDamageBar can catch up via lerp
	Cpp_TargetDodgeStamina = DodgeStaminaTo01;

	// DynamicStaminaBar is set instantly 
	Cpp_DynamicStaminaBar->SetScalarParameterValue(FName("Decimal"), Cpp_TargetDodgeStamina);
}

// Lerps (red) DynamicStaminaDamageBar Decimal to match TargetDodgeStamina with drag 
void UPaintballWB_Gameplay::Cpp_UpdateDodgeStaminaDamageBar()
{
	if (!Cpp_DynamicStaminaDamageBar)
	{
		return;
	}

	float CurrentDecimalOut = 0.f;
	Cpp_DynamicStaminaDamageBar->GetScalarParameterValue(FName("Decimal"), CurrentDecimalOut);

	Cpp_DynamicStaminaDamageBar->SetScalarParameterValue(FName("Decimal"), FMath::Lerp(
        CurrentDecimalOut,
        Cpp_TargetDodgeStamina,
        Cpp_DamageIndicatorLerpTime * GetWorld()->GetDeltaSeconds()
    ));
}

//Lerps DynamicFortifyFill Decimal to match PawnOwner->DodgeStaminaDecrementScalar with drag
void UPaintballWB_Gameplay::Cpp_UpdateDodgeStaminaDecrementScalar(const float& DecrementScalar)
{
	if (!Cpp_PawnOwner || !Cpp_DynamicFortifyFill)
	{
		return;
	}

	const float PawnOwnerDodgeStaminaDecrementScalar = DecrementScalar;

	float CurrentDecimalOut = 0.f;
	Cpp_DynamicFortifyFill->GetScalarParameterValue(FName("Decimal"), CurrentDecimalOut);
	
	Cpp_DynamicFortifyFill->SetScalarParameterValue(FName("Decimal"), FMath::Lerp(
		CurrentDecimalOut,
		PawnOwnerDodgeStaminaDecrementScalar * -1.7f, // * -1.7f adjusts for material inaccuracy  
		Cpp_FortifyLerpTime * GetWorld()->GetDeltaSeconds()
	));
}
