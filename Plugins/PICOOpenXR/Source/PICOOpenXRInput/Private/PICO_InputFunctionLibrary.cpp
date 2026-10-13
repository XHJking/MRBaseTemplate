// Copyright 2023 PICO Inc. All Rights Reserved.

#include "PICO_InputFunctionLibrary.h"
#include "PICO_InputModule.h"

bool UInputFunctionLibraryPICO::GetControllerBatteryLevelPICO(const EControllerHand Hand, float& Level)
{
	return FPICOOpenXRInputModule::Get().GetController().GetControllerBatteryLevel(Hand, Level);
}
