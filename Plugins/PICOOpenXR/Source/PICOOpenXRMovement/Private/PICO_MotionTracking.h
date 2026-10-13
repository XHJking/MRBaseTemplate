// Copyright 2023 PICO Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PICO_MovementFunctionLibrary.h"
#include "IOpenXRExtensionPlugin.h"

class FMotionTrackingPICO : public IOpenXRExtensionPlugin
{
public:
	FMotionTrackingPICO();
	virtual ~FMotionTrackingPICO() {}

	void Register();
	void Unregister();

	/** IOpenXRExtensionPlugin */
	virtual FString GetDisplayName() override
	{
		return FString(TEXT("MotionTrackingPICO"));
	}
	virtual bool GetRequiredExtensions(TArray<const ANSICHAR*>& OutExtensions) override;
	virtual const void* OnGetSystem(XrInstance InInstance, const void* InNext) override;
	virtual void PostCreateSession(XrSession InSession) override;
	virtual void OnEvent(XrSession InSession, const XrEventDataBaseHeader* InHeader) override;
	virtual void UpdateDeviceLocations(XrSession InSession, XrTime DisplayTime, XrSpace TrackingSpace) override;

	bool RequestMotionTrackerDevice(int DeviceCount);
	bool GetMotionTrackerBatteryState(int64 ID, float& BatteryLevel, EChargingStatePICO& State);
	bool LocateMotionTracker(int64 ID, FRotator& OutRotation, FVector& OutPosition, FVector& OutLinearVelocity, FVector& OutAngularVelocity, FVector& OutLinearAcceleration, FVector& OutAngularAcceleration, bool GetVelAndAcc = false);

private:
	class IXRTrackingSystem* XRTrackingSystem = nullptr;
	XrSession Session = XR_NULL_HANDLE;
	XrTime PredictedTime;
	XrSpace BaseSpace;
	bool bSupportMotionTracking = false;

	PFN_xrRequestMotionTrackerDevicePICO xrRequestMotionTrackerDevicePICO = nullptr;
	PFN_xrGetMotionTrackerBatteryStatePICO xrGetMotionTrackerBatteryStatePICO = nullptr;
	PFN_xrLocateMotionTrackerPICO xrLocateMotionTrackerPICO = nullptr;
};
