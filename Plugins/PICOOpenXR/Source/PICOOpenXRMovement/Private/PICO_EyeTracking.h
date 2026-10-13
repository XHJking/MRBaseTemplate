// Copyright 2023 PICO Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PICO_MovementFunctionLibrary.h"
#include "IOpenXRExtensionPlugin.h"

class FEyeTrackingPICO : public IOpenXRExtensionPlugin
{
public:
	FEyeTrackingPICO();
	virtual ~FEyeTrackingPICO() {}

	void Register();
	void Unregister();

	/** IOpenXRExtensionPlugin */
	virtual FString GetDisplayName() override
	{
		return FString(TEXT("EyeTrackingPICO"));
	}
	virtual bool GetRequiredExtensions(TArray<const ANSICHAR*>& OutExtensions) override;
	virtual void PostGetSystem(XrInstance InInstance, XrSystemId InSystem) override;
	virtual void PostCreateSession(XrSession InSession) override;

	bool IsEyeTrackerSupported(bool& Supported) { return bIsEyeTrackerSupported; }
	bool IsEyeTrackingRunning() { return bIsEyeTrackingRunning; }
	bool StartEyeTracking();
	bool StopEyeTracking();
	bool GetEyeTrackingData(FEyeDataPICO& LeftEye, FEyeDataPICO& RightEye, bool QueryGazeData, FEyeTrackerGazeData& OutGazeData, float WorldToMeters = 100.0f);

private:
	XrSession Session;

	PFN_xrCreateEyeTrackerPICO xrCreateEyeTrackerPICO = nullptr;
	PFN_xrDestroyEyeTrackerPICO xrDestroyEyeTrackerPICO = nullptr;
	PFN_xrGetEyeDataPICO xrGetEyeDataPICO = nullptr;

	bool bIsEyeTrackerSupported = false;
	bool bIsEyeTrackingRunning = false;

	XrEyeTrackerPICO EyeTracker = XR_NULL_HANDLE;
};
