// Copyright 2023 PICO Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PICO_MovementFunctionLibrary.h"
#include "IOpenXRExtensionPlugin.h"

class FBodyTrackingPICO : public IOpenXRExtensionPlugin
{
public:
	FBodyTrackingPICO();
	virtual ~FBodyTrackingPICO() {}

	void Register();
	void Unregister();

	/** IOpenXRExtensionPlugin */
	virtual FString GetDisplayName() override
	{
		return FString(TEXT("BodyTrackingPICO"));
	}
	virtual bool GetRequiredExtensions(TArray<const ANSICHAR*>& OutExtensions) override;
	virtual bool GetOptionalExtensions(TArray<const ANSICHAR*>& OutExtensions) override;
	virtual const void* OnGetSystem(XrInstance InInstance, const void* InNext) override;
	virtual void PostGetSystem(XrInstance InInstance, XrSystemId InSystem) override;
	virtual void PostCreateSession(XrSession InSession) override;
	virtual void UpdateDeviceLocations(XrSession InSession, XrTime DisplayTime, XrSpace TrackingSpace) override;
	
	bool TryGetBodyState(FBodyStatePICO& outBodyState, float WorldToMeters = 100.0f, bool QueryAcc = false, bool QueryVel = false, bool QueryPostureFlag = false);
	bool IsBodyTrackingEnabled();
	bool IsBodyTrackingSupported();
	bool StartBodyTracking(EBodyTrackingModePICO Mode);
	bool StopBodyTracking();

	bool StartBodyTrackingCalibApp();
	bool GetBodyTrackingState(EBodyTrackingStatusPICO& Status, EBodyTrackingErrorCodePICO& Error);

private:

	PFN_xrCreateBodyTrackerBD xrCreateBodyTrackerBD = nullptr;
	PFN_xrDestroyBodyTrackerBD xrDestroyBodyTrackerBD = nullptr;
	PFN_xrLocateBodyJointsBD xrLocateBodyJointsBD = nullptr;
	bool bCurrentDeviceSupportBodyTracking = false;
	XrInstance Instance;
	XrSession Session;
	XrBodyTrackerBD BodyTracker;
	bool bBodyTrackerIsRunning;
	bool bSupportBodyTracking2 = false;

	XrTime PredictedTime;
	XrSpace BaseSpace;
	TArray<XrBodyJointLocationBD> Locations;
	TArray<XrBodyTrackingPosturePICO> PostureFlags;
	TArray<XrBodyJointVelocityPICO> Velocities;
	TArray<XrBodyJointAccelerationPICO> Accelerations;
	PFN_xrStartBodyTrackingCalibrationAppPICO xrStartBodyTrackingCalibrationAppPICO = nullptr;
	PFN_xrGetBodyTrackingStatePICO xrGetBodyTrackingStatePICO = nullptr;
};
