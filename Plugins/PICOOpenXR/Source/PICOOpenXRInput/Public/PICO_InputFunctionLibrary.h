// Copyright 2023 PICO Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/DataTable.h"
#include "PICO_InputFunctionLibrary.generated.h"

UCLASS()
class PICOOPENXRINPUT_API UInputFunctionLibraryPICO : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
		
public:
	/**
	* Get the current battery level of the controller
	*
	* @param Hand		left or right hand
	* @param Level      the current battery level of the controller
	*
	* @return		    true if the function call was successful, false otherwise
	*/
	UFUNCTION(BlueprintCallable, Category = "PICO|Input")
	static bool GetControllerBatteryLevelPICO(const EControllerHand Hand, float& Level);
};