// Copyright 2023 PICO Inc. All Rights Reserved.

#include "PICO_Controller.h"
#include "OpenXRCore.h"
#include "PICO_InputModule.h"
#include "IOpenXRHMDModule.h"
#include "Haptics/HapticFeedbackEffect_Base.h"
#include "Framework/Application/SlateApplication.h"
#include "Haptics/HapticFeedbackEffect_SoundWave.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"

#if PLATFORM_ANDROID
#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#endif

#define LOCTEXT_NAMESPACE "FControllerPICO"

// Left
const FKey PICOG3_Left_Menu_Click("PICOG3_Left_Menu_Click");
const FKey PICOG3_Left_Trigger("PICOG3_Left_Trigger_Click");
const FKey PICOG3_Left_Trigger_Axis("PICOG3_Left_Trigger_Axis");
const FKey PICOG3_Left_Thumbstick_2D("PICOG3_Left_Thumbstick_2D");
const FKey PICOG3_Left_Thumbstick_X("PICOG3_Left_Thumbstick_X");
const FKey PICOG3_Left_Thumbstick_Y("PICOG3_Left_Thumbstick_Y");
const FKey PICOG3_Left_Thumbstick("PICOG3_Left_Thumbstick_Click");
//Right
const FKey PICOG3_Right_Menu_Click("PICOG3_Right_Menu_Click");
const FKey PICOG3_Right_Trigger("PICOG3_Right_Trigger_Click");
const FKey PICOG3_Right_Trigger_Axis("PICOG3_Right_Trigger_Axis");
const FKey PICOG3_Right_Thumbstick_2D("PICOG3_Right_Thumbstick_2D");
const FKey PICOG3_Right_Thumbstick_X("PICOG3_Right_Thumbstick_X");
const FKey PICOG3_Right_Thumbstick_Y("PICOG3_Right_Thumbstick_Y");
const FKey PICOG3_Right_Thumbstick("PICOG3_Right_Thumbstick_Click");

// Left
const FKey PICONeo3_Left_X_Click("PICONeo3_Left_X_Click");
const FKey PICONeo3_Left_Y_Click("PICONeo3_Left_Y_Click");
const FKey PICONeo3_Left_X_Touch("PICONeo3_Left_X_Touch");
const FKey PICONeo3_Left_Y_Touch("PICONeo3_Left_Y_Touch");
const FKey PICONeo3_Left_Menu_Click("PICONeo3_Left_Menu_Click");
const FKey PICONeo3_Left_Grip("PICONeo3_Left_Grip_Click");
const FKey PICONeo3_Left_Grip_Axis("PICONeo3_Left_Grip_Axis");
const FKey PICONeo3_Left_Trigger("PICONeo3_Left_Trigger_Click");
const FKey PICONeo3_Left_Trigger_Axis("PICONeo3_Left_Trigger_Axis");
const FKey PICONeo3_Left_Trigger_Touch("PICONeo3_Left_Trigger_Touch");
const FKey PICONeo3_Left_Thumbstick_2D("PICONeo3_Left_Thumbstick_2D");
const FKey PICONeo3_Left_Thumbstick_X("PICONeo3_Left_Thumbstick_X");
const FKey PICONeo3_Left_Thumbstick_Y("PICONeo3_Left_Thumbstick_Y");
const FKey PICONeo3_Left_Thumbstick("PICONeo3_Left_Thumbstick_Click");
const FKey PICONeo3_Left_Thumbstick_Touch("PICONeo3_Left_Thumbstick_Touch");
//Right
const FKey PICONeo3_Right_A_Click("PICONeo3_Right_A_Click");
const FKey PICONeo3_Right_B_Click("PICONeo3_Right_B_Click");
const FKey PICONeo3_Right_A_Touch("PICONeo3_Right_A_Touch");
const FKey PICONeo3_Right_B_Touch("PICONeo3_Right_B_Touch");
const FKey PICONeo3_Right_Menu_Click("PICONeo3_Right_Menu_Click");
const FKey PICONeo3_Right_Grip("PICONeo3_Right_Grip_Click");
const FKey PICONeo3_Right_Grip_Axis("PICONeo3_Right_Grip_Axis");
const FKey PICONeo3_Right_Trigger("PICONeo3_Right_Trigger_Click");
const FKey PICONeo3_Right_Trigger_Axis("PICONeo3_Right_Trigger_Axis");
const FKey PICONeo3_Right_Trigger_Touch("PICONeo3_Right_Trigger_Touch");
const FKey PICONeo3_Right_Thumbstick_2D("PICONeo3_Right_Thumbstick_2D");
const FKey PICONeo3_Right_Thumbstick_X("PICONeo3_Right_Thumbstick_X");
const FKey PICONeo3_Right_Thumbstick_Y("PICONeo3_Right_Thumbstick_Y");
const FKey PICONeo3_Right_Thumbstick("PICONeo3_Right_Thumbstick_Click");
const FKey PICONeo3_Right_Thumbstick_Touch("PICONeo3_Right_Thumbstick_Touch");

// Left
const FKey PICO4_Left_X_Click("PICO4_Left_X_Click");
const FKey PICO4_Left_Y_Click("PICO4_Left_Y_Click");
const FKey PICO4_Left_X_Touch("PICO4_Left_X_Touch");
const FKey PICO4_Left_Y_Touch("PICO4_Left_Y_Touch");
const FKey PICO4_Left_Menu_Click("PICO4_Left_Menu_Click");
const FKey PICO4_Left_Grip("PICO4_Left_Grip_Click");
const FKey PICO4_Left_Grip_Axis("PICO4_Left_Grip_Axis");
const FKey PICO4_Left_Trigger("PICO4_Left_Trigger_Click");
const FKey PICO4_Left_Trigger_Axis("PICO4_Left_Trigger_Axis");
const FKey PICO4_Left_Trigger_Touch("PICO4_Left_Trigger_Touch");
const FKey PICO4_Left_Thumbstick_2D("PICO4_Left_Thumbstick_2D");
const FKey PICO4_Left_Thumbstick_X("PICO4_Left_Thumbstick_X");
const FKey PICO4_Left_Thumbstick_Y("PICO4_Left_Thumbstick_Y");
const FKey PICO4_Left_Thumbstick("PICO4_Left_Thumbstick_Click");
const FKey PICO4_Left_Thumbstick_Touch("PICO4_Left_Thumbstick_Touch");
//Right
const FKey PICO4_Right_A_Click("PICO4_Right_A_Click");
const FKey PICO4_Right_B_Click("PICO4_Right_B_Click");
const FKey PICO4_Right_A_Touch("PICO4_Right_A_Touch");
const FKey PICO4_Right_B_Touch("PICO4_Right_B_Touch");
const FKey PICO4_Right_Grip("PICO4_Right_Grip_Click");
const FKey PICO4_Right_Grip_Axis("PICO4_Right_Grip_Axis");
const FKey PICO4_Right_Trigger("PICO4_Right_Trigger_Click");
const FKey PICO4_Right_Trigger_Axis("PICO4_Right_Trigger_Axis");
const FKey PICO4_Right_Trigger_Touch("PICO4_Right_Trigger_Touch");
const FKey PICO4_Right_Thumbstick_2D("PICO4_Right_Thumbstick_2D");
const FKey PICO4_Right_Thumbstick_X("PICO4_Right_Thumbstick_X");
const FKey PICO4_Right_Thumbstick_Y("PICO4_Right_Thumbstick_Y");
const FKey PICO4_Right_Thumbstick("PICO4_Right_Thumbstick_Click");
const FKey PICO4_Right_Thumbstick_Touch("PICO4_Right_Thumbstick_Touch");

// Left
const FKey PICO4U_Left_X_Click("PICO4U_Left_X_Click");
const FKey PICO4U_Left_Y_Click("PICO4U_Left_Y_Click");
const FKey PICO4U_Left_X_Touch("PICO4U_Left_X_Touch");
const FKey PICO4U_Left_Y_Touch("PICO4U_Left_Y_Touch");
const FKey PICO4U_Left_Menu_Click("PICO4U_Left_Menu_Click");
const FKey PICO4U_Left_Grip("PICO4U_Left_Grip_Click");
const FKey PICO4U_Left_Grip_Axis("PICO4U_Left_Grip_Axis");
const FKey PICO4U_Left_Trigger("PICO4U_Left_Trigger_Click");
const FKey PICO4U_Left_Trigger_Axis("PICO4U_Left_Trigger_Axis");
const FKey PICO4U_Left_Trigger_Touch("PICO4U_Left_Trigger_Touch");
const FKey PICO4U_Left_Thumbstick_2D("PICO4U_Left_Thumbstick_2D");
const FKey PICO4U_Left_Thumbstick_X("PICO4U_Left_Thumbstick_X");
const FKey PICO4U_Left_Thumbstick_Y("PICO4U_Left_Thumbstick_Y");
const FKey PICO4U_Left_Thumbstick("PICO4U_Left_Thumbstick_Click");
const FKey PICO4U_Left_Thumbstick_Touch("PICO4U_Left_Thumbstick_Touch");
const FKey PICO4U_Left_ThumbRest_Touch("PICO4U_Left_ThumbRest_Touch");
//Right
const FKey PICO4U_Right_A_Click("PICO4U_Right_A_Click");
const FKey PICO4U_Right_B_Click("PICO4U_Right_B_Click");
const FKey PICO4U_Right_A_Touch("PICO4U_Right_A_Touch");
const FKey PICO4U_Right_B_Touch("PICO4U_Right_B_Touch");
const FKey PICO4U_Right_Grip("PICO4U_Right_Grip_Click");
const FKey PICO4U_Right_Grip_Axis("PICO4U_Right_Grip_Axis");
const FKey PICO4U_Right_Trigger("PICO4U_Right_Trigger_Click");
const FKey PICO4U_Right_Trigger_Axis("PICO4U_Right_Trigger_Axis");
const FKey PICO4U_Right_Trigger_Touch("PICO4U_Right_Trigger_Touch");
const FKey PICO4U_Right_Thumbstick_2D("PICO4U_Right_Thumbstick_2D");
const FKey PICO4U_Right_Thumbstick_X("PICO4U_Right_Thumbstick_X");
const FKey PICO4U_Right_Thumbstick_Y("PICO4U_Right_Thumbstick_Y");
const FKey PICO4U_Right_Thumbstick("PICO4U_Right_Thumbstick_Click");
const FKey PICO4U_Right_Thumbstick_Touch("PICO4U_Right_Thumbstick_Touch");
const FKey PICO4U_Right_ThumbRest_Touch("PICO4U_Right_ThumbRest_Touch");

FControllerPICO::FControllerPICO()
	:SuggestedBindings(false)
{
}

void FControllerPICO::Register()
{
	RegisterOpenXRExtensionModularFeature();
	
	EKeys::AddMenuCategoryDisplayInfo("PICOG3", LOCTEXT("PICOG3SubCategory", "PICO G3 Controller"), TEXT("GraphEditor.PadEvent_16x"));

	// Left
	EKeys::AddKey(FKeyDetails(PICOG3_Left_Menu_Click, LOCTEXT("PICOG3_Left_Menu_Click", "PICO G3 (L) Menu"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICOG3"));
	EKeys::AddKey(FKeyDetails(PICOG3_Left_Trigger, LOCTEXT("PICOG3_Left_Trigger_Click", "PICO G3 (L) Trigger"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICOG3"));
	EKeys::AddKey(FKeyDetails(PICOG3_Left_Trigger_Axis, LOCTEXT("PICOG3_Left_Trigger_Axis", "PICO G3 (L) Trigger Axis"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "PICOG3"));
	EKeys::AddKey(FKeyDetails(PICOG3_Left_Thumbstick_X, LOCTEXT("PICOG3_Left_Thumbstick_X", "PICO G3 (L) Thumbstick X"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "PICOG3"));
	EKeys::AddKey(FKeyDetails(PICOG3_Left_Thumbstick_Y, LOCTEXT("PICOG3_Left_Thumbstick_Y", "PICO G3 (L) Thumbstick Y"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "PICOG3"));
	EKeys::AddPairedKey(FKeyDetails(PICOG3_Left_Thumbstick_2D, LOCTEXT("PICOG3_Left_Thumbstick_2D", "PICO G3 (L) Thumbstick 2D-Axis"), FKeyDetails::GamepadKey | FKeyDetails::Axis2D | FKeyDetails::NotBlueprintBindableKey, "PICOG3"), PICOG3_Left_Thumbstick_X, PICOG3_Left_Thumbstick_Y);
	EKeys::AddKey(FKeyDetails(PICOG3_Left_Thumbstick, LOCTEXT("PICOG3_Left_Thumbstick_Click", "PICO G3 (L) Thumbstick"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICOG3"));

	// Right
	EKeys::AddKey(FKeyDetails(PICOG3_Right_Menu_Click, LOCTEXT("PICOG3_Right_Menu_Click", "PICO G3 (R) Menu"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICOG3"));
	EKeys::AddKey(FKeyDetails(PICOG3_Right_Trigger, LOCTEXT("PICOG3_Right_Trigger_Click", "PICO G3 (R) Trigger"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICOG3"));
	EKeys::AddKey(FKeyDetails(PICOG3_Right_Trigger_Axis, LOCTEXT("PICOG3_Right_Trigger_Axis", "PICO G3 (R) Trigger Axis"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "PICOG3"));
	EKeys::AddKey(FKeyDetails(PICOG3_Right_Thumbstick_X, LOCTEXT("PICOG3_Right_Thumbstick_X", "PICO G3 (R) Thumbstick X"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "PICOG3"));
	EKeys::AddKey(FKeyDetails(PICOG3_Right_Thumbstick_Y, LOCTEXT("PICOG3_Right_Thumbstick_Y", "PICO G3 (R) Thumbstick Y"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "PICOG3"));
	EKeys::AddPairedKey(FKeyDetails(PICOG3_Right_Thumbstick_2D, LOCTEXT("PICOG3_Right_Thumbstick_2D", "PICO G3 (R) Thumbstick 2D-Axis"), FKeyDetails::GamepadKey | FKeyDetails::Axis2D | FKeyDetails::NotBlueprintBindableKey, "PICOG3"), PICOG3_Right_Thumbstick_X, PICOG3_Right_Thumbstick_Y);
	EKeys::AddKey(FKeyDetails(PICOG3_Right_Thumbstick, LOCTEXT("PICOG3_Right_Thumbstick_Click", "PICO G3 (R) Thumbstick"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICOG3"));

	EKeys::AddMenuCategoryDisplayInfo("PICONeo3", LOCTEXT("PICONeo3SubCategory", "PICO Neo3 Controller"), TEXT("GraphEditor.PadEvent_16x"));

	// Left
	EKeys::AddKey(FKeyDetails(PICONeo3_Left_X_Click, LOCTEXT("PICONeo3_Left_X_Click", "PICO Neo3 (L) X Press"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICONeo3"));
	EKeys::AddKey(FKeyDetails(PICONeo3_Left_Y_Click, LOCTEXT("PICONeo3_Left_Y_Click", "PICO Neo3 (L) Y Press"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICONeo3"));
	EKeys::AddKey(FKeyDetails(PICONeo3_Left_X_Touch, LOCTEXT("PICONeo3_Left_X_Touch", "PICO Neo3 (L) X Touch"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICONeo3"));
	EKeys::AddKey(FKeyDetails(PICONeo3_Left_Y_Touch, LOCTEXT("PICONeo3_Left_Y_Touch", "PICO Neo3 (L) Y Touch"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICONeo3"));
	EKeys::AddKey(FKeyDetails(PICONeo3_Left_Menu_Click, LOCTEXT("PICONeo3_Left_Menu_Click", "PICO Neo3 (L) Menu"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICONeo3"));
	EKeys::AddKey(FKeyDetails(PICONeo3_Left_Grip, LOCTEXT("PICONeo3_Left_Grip_Click", "PICO Neo3 (L) Grip"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICONeo3"));
	EKeys::AddKey(FKeyDetails(PICONeo3_Left_Grip_Axis, LOCTEXT("PICONeo3_Left_Grip_Axis", "PICO Neo3 (L) Grip Axis"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "PICONeo3"));
	EKeys::AddKey(FKeyDetails(PICONeo3_Left_Trigger, LOCTEXT("PICONeo3_Left_Trigger_Click", "PICO Neo3 (L) Trigger"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICONeo3"));
	EKeys::AddKey(FKeyDetails(PICONeo3_Left_Trigger_Axis, LOCTEXT("PICONeo3_Left_Trigger_Axis", "PICO Neo3 (L) Trigger Axis"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "PICONeo3"));
	EKeys::AddKey(FKeyDetails(PICONeo3_Left_Trigger_Touch, LOCTEXT("PICONeo3_Left_Trigger_Touch", "PICO Neo3 (L) Trigger Touch"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICONeo3"));
	EKeys::AddKey(FKeyDetails(PICONeo3_Left_Thumbstick_X, LOCTEXT("PICONeo3_Left_Thumbstick_X", "PICO Neo3 (L) Thumbstick X"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "PICONeo3"));
	EKeys::AddKey(FKeyDetails(PICONeo3_Left_Thumbstick_Y, LOCTEXT("PICONeo3_Left_Thumbstick_Y", "PICO Neo3 (L) Thumbstick Y"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "PICONeo3"));
	EKeys::AddPairedKey(FKeyDetails(PICONeo3_Left_Thumbstick_2D, LOCTEXT("PICONeo3_Left_Thumbstick_2D", "PICO Neo3 (L) Thumbstick 2D-Axis"), FKeyDetails::GamepadKey | FKeyDetails::Axis2D | FKeyDetails::NotBlueprintBindableKey, "PICONeo3"), PICONeo3_Left_Thumbstick_X, PICONeo3_Left_Thumbstick_Y);
	EKeys::AddKey(FKeyDetails(PICONeo3_Left_Thumbstick, LOCTEXT("PICONeo3_Left_Thumbstick_Click", "PICO Neo3 (L) Thumbstick"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICONeo3"));
	EKeys::AddKey(FKeyDetails(PICONeo3_Left_Thumbstick_Touch, LOCTEXT("PICONeo3_Left_Thumbstick_Touch", "PICO Neo3 (L) Thumbstick Touch"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICONeo3"));

	// Right
	EKeys::AddKey(FKeyDetails(PICONeo3_Right_A_Click, LOCTEXT("PICONeo3_Right_A_Click", "PICO Neo3 (R) A Press"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICONeo3"));
	EKeys::AddKey(FKeyDetails(PICONeo3_Right_B_Click, LOCTEXT("PICONeo3_Right_B_Click", "PICO Neo3 (R) B Press"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICONeo3"));
	EKeys::AddKey(FKeyDetails(PICONeo3_Right_A_Touch, LOCTEXT("PICONeo3_Right_A_Touch", "PICO Neo3 (R) A Touch"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICONeo3"));
	EKeys::AddKey(FKeyDetails(PICONeo3_Right_B_Touch, LOCTEXT("PICONeo3_Right_B_Touch", "PICO Neo3 (R) B Touch"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICONeo3"));
	EKeys::AddKey(FKeyDetails(PICONeo3_Right_Menu_Click, LOCTEXT("PICONeo3_Right_Menu_Click", "PICO Neo3 (R) Menu"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICONeo3"));
	EKeys::AddKey(FKeyDetails(PICONeo3_Right_Grip, LOCTEXT("PICONeo3_Right_Grip_Click", "PICO Neo3 (R) Grip"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICONeo3"));
	EKeys::AddKey(FKeyDetails(PICONeo3_Right_Grip_Axis, LOCTEXT("PICONeo3_Right_Grip_Axis", "PICO Neo3 (R) Grip Axis"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "PICONeo3"));
	EKeys::AddKey(FKeyDetails(PICONeo3_Right_Trigger, LOCTEXT("PICONeo3_Right_Trigger_Click", "PICO Neo3 (R) Trigger"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICONeo3"));
	EKeys::AddKey(FKeyDetails(PICONeo3_Right_Trigger_Axis, LOCTEXT("PICONeo3_Right_Trigger_Axis", "PICO Neo3 (R) Trigger Axis"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "PICONeo3"));
	EKeys::AddKey(FKeyDetails(PICONeo3_Right_Trigger_Touch, LOCTEXT("PICONeo3_Right_Trigger_Touch", "PICO Neo3 (R) Trigger Touch"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICONeo3"));
	EKeys::AddKey(FKeyDetails(PICONeo3_Right_Thumbstick_X, LOCTEXT("PICONeo3_Right_Thumbstick_X", "PICO Neo3 (R) Thumbstick X"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "PICONeo3"));
	EKeys::AddKey(FKeyDetails(PICONeo3_Right_Thumbstick_Y, LOCTEXT("PICONeo3_Right_Thumbstick_Y", "PICO Neo3 (R) Thumbstick Y"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "PICONeo3"));
	EKeys::AddPairedKey(FKeyDetails(PICONeo3_Right_Thumbstick_2D, LOCTEXT("PICONeo3_Right_Thumbstick_2D", "PICO Neo3 (R) Thumbstick 2D-Axis"), FKeyDetails::GamepadKey | FKeyDetails::Axis2D | FKeyDetails::NotBlueprintBindableKey, "PICONeo3"), PICONeo3_Right_Thumbstick_X, PICONeo3_Right_Thumbstick_Y);
	EKeys::AddKey(FKeyDetails(PICONeo3_Right_Thumbstick, LOCTEXT("PICONeo3_Right_Thumbstick_Click", "PICO Neo3 (R) Thumbstick"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICONeo3"));
	EKeys::AddKey(FKeyDetails(PICONeo3_Right_Thumbstick_Touch, LOCTEXT("PICONeo3_Right_Thumbstick_Touch", "PICO Neo3 (R) Thumbstick Touch"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICONeo3"));

	EKeys::AddMenuCategoryDisplayInfo("PICO4", LOCTEXT("PICO4SubCategory", "PICO 4 Controller"), TEXT("GraphEditor.PadEvent_16x"));

	// Left
	EKeys::AddKey(FKeyDetails(PICO4_Left_X_Click, LOCTEXT("PICO4_Left_X_Click", "PICO 4 (L) X Press"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICO4"));
	EKeys::AddKey(FKeyDetails(PICO4_Left_Y_Click, LOCTEXT("PICO4_Left_Y_Click", "PICO 4 (L) Y Press"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICO4"));
	EKeys::AddKey(FKeyDetails(PICO4_Left_X_Touch, LOCTEXT("PICO4_Left_X_Touch", "PICO 4 (L) X Touch"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICO4"));
	EKeys::AddKey(FKeyDetails(PICO4_Left_Y_Touch, LOCTEXT("PICO4_Left_Y_Touch", "PICO 4 (L) Y Touch"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICO4"));
	EKeys::AddKey(FKeyDetails(PICO4_Left_Menu_Click, LOCTEXT("PICO4_Left_Menu_Click", "PICO 4 (L) Menu"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICO4"));
	EKeys::AddKey(FKeyDetails(PICO4_Left_Grip, LOCTEXT("PICO4_Left_Grip_Click", "PICO 4 (L) Grip"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICO4"));
	EKeys::AddKey(FKeyDetails(PICO4_Left_Grip_Axis, LOCTEXT("PICO4_Left_Grip_Axis", "PICO 4 (L) Grip Axis"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "PICO4"));
	EKeys::AddKey(FKeyDetails(PICO4_Left_Trigger, LOCTEXT("PICO4_Left_Trigger_Click", "PICO 4 (L) Trigger"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICO4"));
	EKeys::AddKey(FKeyDetails(PICO4_Left_Trigger_Axis, LOCTEXT("PICO4_Left_Trigger_Axis", "PICO 4 (L) Trigger Axis"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "PICO4"));
	EKeys::AddKey(FKeyDetails(PICO4_Left_Trigger_Touch, LOCTEXT("PICO4_Left_Trigger_Touch", "PICO 4 (L) Trigger Touch"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICO4"));
	EKeys::AddKey(FKeyDetails(PICO4_Left_Thumbstick_X, LOCTEXT("PICO4_Left_Thumbstick_X", "PICO 4 (L) Thumbstick X"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "PICO4"));
	EKeys::AddKey(FKeyDetails(PICO4_Left_Thumbstick_Y, LOCTEXT("PICO4_Left_Thumbstick_Y", "PICO 4 (L) Thumbstick Y"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "PICO4"));
	EKeys::AddPairedKey(FKeyDetails(PICO4_Left_Thumbstick_2D, LOCTEXT("PICO4_Left_Thumbstick_2D", "PICO 4 (L) Thumbstick 2D-Axis"), FKeyDetails::GamepadKey | FKeyDetails::Axis2D | FKeyDetails::NotBlueprintBindableKey, "PICO4"), PICO4_Left_Thumbstick_X, PICO4_Left_Thumbstick_Y);
	EKeys::AddKey(FKeyDetails(PICO4_Left_Thumbstick, LOCTEXT("PICO4_Left_Thumbstick_Click", "PICO 4 (L) Thumbstick"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICO4"));
	EKeys::AddKey(FKeyDetails(PICO4_Left_Thumbstick_Touch, LOCTEXT("PICO4_Left_Thumbstick_Touch", "PICO 4 (L) Thumbstick Touch"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICO4"));

	// Right
	EKeys::AddKey(FKeyDetails(PICO4_Right_A_Click, LOCTEXT("PICO4_Right_A_Click", "PICO 4 (R) A Press"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICO4"));
	EKeys::AddKey(FKeyDetails(PICO4_Right_B_Click, LOCTEXT("PICO4_Right_B_Click", "PICO 4 (R) B Press"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICO4"));
	EKeys::AddKey(FKeyDetails(PICO4_Right_A_Touch, LOCTEXT("PICO4_Right_A_Touch", "PICO 4 (R) A Touch"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICO4"));
	EKeys::AddKey(FKeyDetails(PICO4_Right_B_Touch, LOCTEXT("PICO4_Right_B_Touch", "PICO 4 (R) B Touch"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICO4"));
	EKeys::AddKey(FKeyDetails(PICO4_Right_Grip, LOCTEXT("PICO4_Right_Grip_Click", "PICO 4 (R) Grip"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICO4"));
	EKeys::AddKey(FKeyDetails(PICO4_Right_Grip_Axis, LOCTEXT("PICO4_Right_Grip_Axis", "PICO 4 (R) Grip Axis"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "PICO4"));
	EKeys::AddKey(FKeyDetails(PICO4_Right_Trigger, LOCTEXT("PICO4_Right_Trigger_Click", "PICO 4 (R) Trigger"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICO4"));
	EKeys::AddKey(FKeyDetails(PICO4_Right_Trigger_Axis, LOCTEXT("PICO4_Right_Trigger_Axis", "PICO 4 (R) Trigger Axis"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "PICO4"));
	EKeys::AddKey(FKeyDetails(PICO4_Right_Trigger_Touch, LOCTEXT("PICO4_Right_Trigger_Touch", "PICO 4 (R) Trigger Touch"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICO4"));
	EKeys::AddKey(FKeyDetails(PICO4_Right_Thumbstick_X, LOCTEXT("PICO4_Right_Thumbstick_X", "PICO 4 (R) Thumbstick X"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "PICO4"));
	EKeys::AddKey(FKeyDetails(PICO4_Right_Thumbstick_Y, LOCTEXT("PICO4_Right_Thumbstick_Y", "PICO 4 (R) Thumbstick Y"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "PICO4"));
	EKeys::AddPairedKey(FKeyDetails(PICO4_Right_Thumbstick_2D, LOCTEXT("PICO4_Right_Thumbstick_2D", "PICO 4 (R) Thumbstick 2D-Axis"), FKeyDetails::GamepadKey | FKeyDetails::Axis2D | FKeyDetails::NotBlueprintBindableKey, "PICO4"), PICO4_Right_Thumbstick_X, PICO4_Right_Thumbstick_Y);
	EKeys::AddKey(FKeyDetails(PICO4_Right_Thumbstick, LOCTEXT("PICO4_Right_Thumbstick_Click", "PICO 4 (R) Thumbstick"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICO4"));
	EKeys::AddKey(FKeyDetails(PICO4_Right_Thumbstick_Touch, LOCTEXT("PICO4_Right_Thumbstick_Touch", "PICO 4 (R) Thumbstick Touch"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICO4"));

	EKeys::AddMenuCategoryDisplayInfo("PICO4U", LOCTEXT("PICO4USubCategory", "PICO 4U Controller"), TEXT("GraphEditor.PadEvent_16x"));

	// Left
	EKeys::AddKey(FKeyDetails(PICO4U_Left_X_Click, LOCTEXT("PICO4U_Left_X_Click", "PICO 4U (L) X Press"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICO4U"));
	EKeys::AddKey(FKeyDetails(PICO4U_Left_Y_Click, LOCTEXT("PICO4U_Left_Y_Click", "PICO 4U (L) Y Press"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICO4U"));
	EKeys::AddKey(FKeyDetails(PICO4U_Left_X_Touch, LOCTEXT("PICO4U_Left_X_Touch", "PICO 4U (L) X Touch"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICO4U"));
	EKeys::AddKey(FKeyDetails(PICO4U_Left_Y_Touch, LOCTEXT("PICO4U_Left_Y_Touch", "PICO 4U (L) Y Touch"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICO4U"));
	EKeys::AddKey(FKeyDetails(PICO4U_Left_Menu_Click, LOCTEXT("PICO4U_Left_Menu_Click", "PICO 4U (L) Menu"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICO4U"));
	EKeys::AddKey(FKeyDetails(PICO4U_Left_Grip, LOCTEXT("PICO4U_Left_Grip_Click", "PICO 4U (L) Grip"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICO4U"));
	EKeys::AddKey(FKeyDetails(PICO4U_Left_Grip_Axis, LOCTEXT("PICO4U_Left_Grip_Axis", "PICO 4U (L) Grip Axis"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "PICO4U"));
	EKeys::AddKey(FKeyDetails(PICO4U_Left_Trigger, LOCTEXT("PICO4U_Left_Trigger_Click", "PICO 4U (L) Trigger"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICO4U"));
	EKeys::AddKey(FKeyDetails(PICO4U_Left_Trigger_Axis, LOCTEXT("PICO4U_Left_Trigger_Axis", "PICO 4U (L) Trigger Axis"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "PICO4U"));
	EKeys::AddKey(FKeyDetails(PICO4U_Left_Trigger_Touch, LOCTEXT("PICO4U_Left_Trigger_Touch", "PICO 4U (L) Trigger Touch"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICO4U"));
	EKeys::AddKey(FKeyDetails(PICO4U_Left_Thumbstick_X, LOCTEXT("PICO4U_Left_Thumbstick_X", "PICO 4U (L) Thumbstick X"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "PICO4U"));
	EKeys::AddKey(FKeyDetails(PICO4U_Left_Thumbstick_Y, LOCTEXT("PICO4U_Left_Thumbstick_Y", "PICO 4U (L) Thumbstick Y"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "PICO4U"));
	EKeys::AddPairedKey(FKeyDetails(PICO4U_Left_Thumbstick_2D, LOCTEXT("PICO4U_Left_Thumbstick_2D", "PICO 4U (L) Thumbstick 2D-Axis"), FKeyDetails::GamepadKey | FKeyDetails::Axis2D | FKeyDetails::NotBlueprintBindableKey, "PICO4U"), PICO4U_Left_Thumbstick_X, PICO4U_Left_Thumbstick_Y);
	EKeys::AddKey(FKeyDetails(PICO4U_Left_Thumbstick, LOCTEXT("PICO4U_Left_Thumbstick_Click", "PICO 4U (L) Thumbstick"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICO4U"));
	EKeys::AddKey(FKeyDetails(PICO4U_Left_Thumbstick_Touch, LOCTEXT("PICO4U_Left_Thumbstick_Touch", "PICO 4U (L) Thumbstick Touch"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICO4U"));
	EKeys::AddKey(FKeyDetails(PICO4U_Left_ThumbRest_Touch, LOCTEXT("PICO4U_Left_ThumbRest_Touch", "PICO 4U (L) ThumbRest Touch"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICO4U"));

	// Right
	EKeys::AddKey(FKeyDetails(PICO4U_Right_A_Click, LOCTEXT("PICO4U_Right_A_Click", "PICO 4U (R) A Press"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICO4U"));
	EKeys::AddKey(FKeyDetails(PICO4U_Right_B_Click, LOCTEXT("PICO4U_Right_B_Click", "PICO 4U (R) B Press"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICO4U"));
	EKeys::AddKey(FKeyDetails(PICO4U_Right_A_Touch, LOCTEXT("PICO4U_Right_A_Touch", "PICO 4U (R) A Touch"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICO4U"));
	EKeys::AddKey(FKeyDetails(PICO4U_Right_B_Touch, LOCTEXT("PICO4U_Right_B_Touch", "PICO 4U (R) B Touch"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICO4U"));
	EKeys::AddKey(FKeyDetails(PICO4U_Right_Grip, LOCTEXT("PICO4U_Right_Grip_Click", "PICO 4U (R) Grip"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICO4U"));
	EKeys::AddKey(FKeyDetails(PICO4U_Right_Grip_Axis, LOCTEXT("PICO4U_Right_Grip_Axis", "PICO 4U (R) Grip Axis"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "PICO4U"));
	EKeys::AddKey(FKeyDetails(PICO4U_Right_Trigger, LOCTEXT("PICO4U_Right_Trigger_Click", "PICO 4U (R) Trigger"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICO4U"));
	EKeys::AddKey(FKeyDetails(PICO4U_Right_Trigger_Axis, LOCTEXT("PICO4U_Right_Trigger_Axis", "PICO 4U (R) Trigger Axis"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "PICO4U"));
	EKeys::AddKey(FKeyDetails(PICO4U_Right_Trigger_Touch, LOCTEXT("PICO4U_Right_Trigger_Touch", "PICO 4U (R) Trigger Touch"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICO4U"));
	EKeys::AddKey(FKeyDetails(PICO4U_Right_Thumbstick_X, LOCTEXT("PICO4U_Right_Thumbstick_X", "PICO 4U (R) Thumbstick X"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "PICO4U"));
	EKeys::AddKey(FKeyDetails(PICO4U_Right_Thumbstick_Y, LOCTEXT("PICO4U_Right_Thumbstick_Y", "PICO 4U (R) Thumbstick Y"), FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "PICO4U"));
	EKeys::AddPairedKey(FKeyDetails(PICO4U_Right_Thumbstick_2D, LOCTEXT("PICO4U_Right_Thumbstick_2D", "PICO 4U (R) Thumbstick 2D-Axis"), FKeyDetails::GamepadKey | FKeyDetails::Axis2D | FKeyDetails::NotBlueprintBindableKey, "PICO4U"), PICO4U_Right_Thumbstick_X, PICO4U_Right_Thumbstick_Y);
	EKeys::AddKey(FKeyDetails(PICO4U_Right_Thumbstick, LOCTEXT("PICO4U_Right_Thumbstick_Click", "PICO 4U (R) Thumbstick"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICO4U"));
	EKeys::AddKey(FKeyDetails(PICO4U_Right_Thumbstick_Touch, LOCTEXT("PICO4U_Right_Thumbstick_Touch", "PICO 4U (R) Thumbstick Touch"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICO4U"));
	EKeys::AddKey(FKeyDetails(PICO4U_Right_ThumbRest_Touch, LOCTEXT("PICO4U_Right_ThumbRest_Touch", "PICO 4U (R) ThumbRest Touch"), FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "PICO4U"));
}

void FControllerPICO::Unregister()
{
	UnregisterOpenXRExtensionModularFeature();
}

bool FControllerPICO::GetRequiredExtensions(TArray<const ANSICHAR*>& OutExtensions)
{
	OutExtensions.Add("XR_BD_controller_interaction");
	return true;
}

void FControllerPICO::PostCreateInstance(XrInstance InInstance)
{
	Instance = InInstance;

	XrPath PathLeft;
	XrResult Result;
	Result = xrStringToPath(Instance, "/user/hand/left", &PathLeft);
	if (XR_SUCCEEDED(Result))
	{
		SubactionPaths.Add(PathLeft);
	}

	XrPath PathRight;
	Result = xrStringToPath(Instance, "/user/hand/right", &PathRight);
	if (XR_SUCCEEDED(Result))
	{
		SubactionPaths.Add(PathRight);
	}

	ActionStateFloats.SetNum(SubactionPaths.Num());
}

const void* FControllerPICO::OnCreateSession(XrInstance InInstance, XrSystemId InSystem, const void* InNext)
{
	if (ControllerActionSet)
	{
		XR_ENSURE(xrDestroyActionSet(ControllerActionSet));
	}

	XrActionSetCreateInfo ActionSetInfo = { XR_TYPE_ACTION_SET_CREATE_INFO };
	FCStringAnsi::Strncpy(ActionSetInfo.actionSetName, "openxrbatteryactionset", XR_MAX_ACTION_SET_NAME_SIZE);
	FCStringAnsi::Strncpy(ActionSetInfo.localizedActionSetName, "OpenXR Battery Action Set", XR_MAX_LOCALIZED_ACTION_SET_NAME_SIZE);
	XR_ENSURE(xrCreateActionSet(Instance, &ActionSetInfo, &ControllerActionSet));

	XrActionCreateInfo ActionInfo = { XR_TYPE_ACTION_CREATE_INFO };
	ActionInfo.countSubactionPaths = SubactionPaths.Num();
	ActionInfo.subactionPaths = SubactionPaths.GetData();
	FCStringAnsi::Strncpy(ActionInfo.actionName, "openxrbatteryaction", XR_MAX_ACTION_NAME_SIZE);
	FCStringAnsi::Strncpy(ActionInfo.localizedActionName, "OpenXR Battery Action", XR_MAX_LOCALIZED_ACTION_NAME_SIZE);
	ActionInfo.actionType = XR_ACTION_TYPE_FLOAT_INPUT;
	XR_ENSURE(xrCreateAction(ControllerActionSet, &ActionInfo, &ControllerBatteryAction));

	return InNext;
}

void FControllerPICO::PostCreateSession(XrSession InSession)
{
	Session = InSession;
}

bool FControllerPICO::GetInteractionProfiles(XrInstance InInstance, TArray<FString>&OutKeyPrefixes, TArray<XrPath>&OutPaths, TArray<bool>&OutHasHaptics)
{
	XrPath InteractionProfile;
	XrResult Result = xrStringToPath(InInstance, "/interaction_profiles/bytedance/pico_g3_controller", &InteractionProfile);
	if (XR_SUCCEEDED(Result))
	{
		OutKeyPrefixes.Add("PICOG3");
		OutPaths.Add(InteractionProfile);
		OutHasHaptics.Add(false);
		AddProfile(InteractionProfile);
	}

	Result = xrStringToPath(InInstance, "/interaction_profiles/bytedance/pico_neo3_controller", &InteractionProfile);
	if (XR_SUCCEEDED(Result))
	{
		OutKeyPrefixes.Add("PICONeo3");
		OutPaths.Add(InteractionProfile);
		OutHasHaptics.Add(true);
		AddProfile(InteractionProfile);
	}

	Result = xrStringToPath(InInstance, "/interaction_profiles/bytedance/pico4_controller", &InteractionProfile);
	if (XR_SUCCEEDED(Result))
	{
		OutKeyPrefixes.Add("PICO4");
		OutPaths.Add(InteractionProfile);
		OutHasHaptics.Add(true);
		AddProfile(InteractionProfile);
	}

	Result = xrStringToPath(InInstance, "/interaction_profiles/bytedance/pico4s_controller", &InteractionProfile);
	if (XR_SUCCEEDED(Result))
	{
		OutKeyPrefixes.Add("PICO4U");
		OutPaths.Add(InteractionProfile);
		OutHasHaptics.Add(true);
		AddProfile(InteractionProfile);
	}

	return true;
}

bool FControllerPICO::GetSuggestedBindings(XrPath InInteractionProfile, TArray<XrActionSuggestedBinding>& OutBindings)
{
	if (Instance != XR_NULL_HANDLE && ControllerBatteryAction != XR_NULL_HANDLE && Profiles.Find(InInteractionProfile) != INDEX_NONE)
	{
		XrPath BatteryLeft;
		XrResult Result;
		Result = xrStringToPath(Instance, "/user/hand/left/input/battery/value", &BatteryLeft);
		if (XR_SUCCEEDED(Result))
		{
			OutBindings.Add(XrActionSuggestedBinding{ ControllerBatteryAction ,BatteryLeft });
		}

		XrPath BatteryRight;
		Result = xrStringToPath(Instance, "/user/hand/right/input/battery/value", &BatteryRight);
		if (XR_SUCCEEDED(Result))
		{
			OutBindings.Add(XrActionSuggestedBinding{ ControllerBatteryAction ,BatteryRight });
		}

		SuggestedBindings = true;
		return true;
	}

	return false;
}

void FControllerPICO::AttachActionSets(TSet<XrActionSet>& OutActionSets)
{
	if (SuggestedBindings)
	{
		check(ControllerActionSet != XR_NULL_HANDLE);
		OutActionSets.Add(ControllerActionSet);
	}
}

void FControllerPICO::GetActiveActionSetsForSync(TArray<XrActiveActionSet>& OutActiveSets)
{
	if (SuggestedBindings)
	{
		check(ControllerActionSet != XR_NULL_HANDLE);
		OutActiveSets.Add(XrActiveActionSet{ ControllerActionSet, SubactionPaths[0] });
		OutActiveSets.Add(XrActiveActionSet{ ControllerActionSet, SubactionPaths[1] });
	}
}

void FControllerPICO::PostSyncActions(XrSession InSession)
{
	IInputInterface* InputInterface = nullptr;

	if (FSlateApplication::IsInitialized())
	{
		InputInterface = FSlateApplication::Get().GetInputInterface();
	}

	int32 ControllerID = 0;

	if (SuggestedBindings)
	{
		check(ControllerBatteryAction != XR_NULL_HANDLE);
		for (int i = 0; i < SubactionPaths.Num(); i++)
		{
			XrActionStateGetInfo GetActionStateInfo{ XR_TYPE_ACTION_STATE_GET_INFO };
			GetActionStateInfo.action = ControllerBatteryAction;
			GetActionStateInfo.subactionPath = SubactionPaths[i];
			ActionStateFloats[i] = { XR_TYPE_ACTION_STATE_FLOAT };
			XR_ENSURE(xrGetActionStateFloat(InSession, &GetActionStateInfo, &ActionStateFloats[i]));
		}
	}
}

bool FControllerPICO::GetControllerBatteryLevel(const EControllerHand Hand, float& Level)
{
	if (Hand == EControllerHand::Left)
	{
		Level = ActionStateFloats[0].currentState;
		return ActionStateFloats[0].isActive == XR_TRUE;
	}
	else if (Hand == EControllerHand::Right)
	{
		Level = ActionStateFloats[1].currentState;
		return ActionStateFloats[1].isActive == XR_TRUE;
	}
	return false;
}

void FControllerPICO::AddProfile(XrPath Profile)
{
	Profiles.Add(Profile);
}

#undef LOCTEXT_NAMESPACE