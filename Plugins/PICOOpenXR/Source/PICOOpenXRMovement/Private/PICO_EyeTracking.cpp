// Copyright 2023 PICO Inc. All Rights Reserved.

#include "PICO_EyeTracking.h"
#include "PICO_MovementModule.h"
#include "OpenXRCore.h"
#include "Engine/Engine.h"
#include "IXRTrackingSystem.h"
#include "PICOOpenXRRuntimeSettings.h"
#include "IOpenXRHMDModule.h"
#include "EyeTrackerFunctionLibrary.h"

#if PLATFORM_ANDROID
#include "Android/AndroidApplication.h"
#endif

FEyeTrackingPICO::FEyeTrackingPICO()
{
}

void FEyeTrackingPICO::Register()
{
	RegisterOpenXRExtensionModularFeature();
}

void FEyeTrackingPICO::Unregister()
{
	UnregisterOpenXRExtensionModularFeature();
}

bool FEyeTrackingPICO::GetRequiredExtensions(TArray<const ANSICHAR*>& OutExtensions)
{
	if (UPICOOpenXRRuntimeSettings::GetBoolConfigByKey("bEyeTrackingEnabled"))
	{
		OutExtensions.Add("XR_PICO_eye_tracker");
		return true;
	}
	return false;
}

void FEyeTrackingPICO::PostGetSystem(XrInstance InInstance, XrSystemId InSystem)
{
	XR_ENSURE(xrGetInstanceProcAddr(InInstance, "xrCreateEyeTrackerPICO", (PFN_xrVoidFunction*)&xrCreateEyeTrackerPICO));
	XR_ENSURE(xrGetInstanceProcAddr(InInstance, "xrDestroyEyeTrackerPICO", (PFN_xrVoidFunction*)&xrDestroyEyeTrackerPICO));
	XR_ENSURE(xrGetInstanceProcAddr(InInstance, "xrGetEyeDataPICO", (PFN_xrVoidFunction*)&xrGetEyeDataPICO));

	if (IOpenXRHMDModule::Get().IsExtensionEnabled(XR_EXT_EYE_GAZE_INTERACTION_EXTENSION_NAME))
	{
		XrSystemEyeGazeInteractionPropertiesEXT EyeGazeInteractionProperties = { XR_TYPE_SYSTEM_EYE_GAZE_INTERACTION_PROPERTIES_EXT };
		XrSystemProperties systemProperties = { XR_TYPE_SYSTEM_PROPERTIES, &EyeGazeInteractionProperties };
		XR_ENSURE(xrGetSystemProperties(InInstance, InSystem, &systemProperties));
		bIsEyeTrackerSupported = EyeGazeInteractionProperties.supportsEyeGazeInteraction == XR_TRUE;
	}
}

void FEyeTrackingPICO::PostCreateSession(XrSession InSession)
{
	Session = InSession;
}

bool FEyeTrackingPICO::StartEyeTracking()
{
	if (Session != XR_NULL_HANDLE)
	{
		UE_LOG(PICOOpenXRMovement, Log, TEXT("Start EyeTracking!"));
		XrEyeTrackerCreateInfoPICO EyeTrackerCreateInfoPICO = { XR_TYPE_EYE_TRACKER_CREATE_INFO_PICO };
		XrResult Result = xrCreateEyeTrackerPICO(Session, &EyeTrackerCreateInfoPICO, &EyeTracker);
		if (XR_SUCCEEDED(Result) && EyeTracker != XR_NULL_HANDLE)
		{
			bIsEyeTrackingRunning = true;
			return true;
		}
	}
	return false;
}

bool FEyeTrackingPICO::StopEyeTracking()
{
	if (EyeTracker != XR_NULL_HANDLE)
	{
		UE_LOG(PICOOpenXRMovement, Log, TEXT("Stop EyeTracking!"));
		XrResult Result = xrDestroyEyeTrackerPICO(EyeTracker);
		if (XR_SUCCEEDED(Result))
		{
			bIsEyeTrackingRunning = false;
			return true;
		}
		return false;
	}
	return true;
}

bool FEyeTrackingPICO::GetEyeTrackingData(FEyeDataPICO& LeftEye, FEyeDataPICO& RightEye, bool QueryGazeData, FEyeTrackerGazeData& OutGazeData, float WorldToMeters)
{
	if (bIsEyeTrackingRunning)
	{
		XrEyeTrackerDataPICO EyeTrackingData = { XR_TYPE_EYE_TRACKER_DATA_PICO };
		XrEyeTrackerDataInfoPICO EyeTrackerDataInfo = { XR_TYPE_EYE_TRACKER_DATA_INFO_PICO };
		EyeTrackerDataInfo.eyeTrackingFlags = XR_EYE_TRACKER_LEFT_BIT_PICO | XR_EYE_TRACKER_RIGHT_BIT_PICO;
		XrResult Result = xrGetEyeDataPICO(EyeTracker, &EyeTrackerDataInfo, &EyeTrackingData);
		if (XR_SUCCEEDED(Result))
		{
			LeftEye.Valid = EyeTrackingData.trackingState & XR_EYE_TRACKER_TRACKING_STATE_LEFT_EYE_BIT_PICO;
			if (LeftEye.Valid)
			{
				LeftEye.Openness = EyeTrackingData.leftEyeData.openness;
				LeftEye.PupilDiameter = EyeTrackingData.leftEyeData.pupilDilation / 1000.0f * WorldToMeters;
				LeftEye.MiddleCanthusUV = { EyeTrackingData.leftEyeData.middleCanthusUv.x, EyeTrackingData.leftEyeData.middleCanthusUv.y };
			}

			RightEye.Valid = EyeTrackingData.trackingState & XR_EYE_TRACKER_TRACKING_STATE_RIGHT_EYE_BIT_PICO;
			if (RightEye.Valid)
			{
				RightEye.Openness = EyeTrackingData.rightEyeData.openness;
				RightEye.PupilDiameter = EyeTrackingData.rightEyeData.pupilDilation / 1000.0f * WorldToMeters;
				RightEye.MiddleCanthusUV = { EyeTrackingData.rightEyeData.middleCanthusUv.x, EyeTrackingData.leftEyeData.middleCanthusUv.y };
			}

			return QueryGazeData ? UEyeTrackerFunctionLibrary::GetGazeData(OutGazeData) : true;
		}
	}
	return false;
}
