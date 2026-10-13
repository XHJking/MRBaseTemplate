// Copyright 2023 PICO Inc. All Rights Reserved.

#pragma once

#include "PICO_MovementFunctionLibrary.h"
#include "CoreMinimal.h"
#include "IOpenXRExtensionPlugin.h"

class FFaceTrackingPICO : public IOpenXRExtensionPlugin
{
public:
	FFaceTrackingPICO();
	virtual ~FFaceTrackingPICO() {}

	void Register();
	void Unregister();

	/** IOpenXRExtensionPlugin */
	virtual FString GetDisplayName() override
	{
		return FString(TEXT("FaceTrackingPICO"));
	}
	virtual bool GetRequiredExtensions(TArray<const ANSICHAR*>& OutExtensions) override;
	virtual const void* OnGetSystem(XrInstance InInstance, const void* InNext) override;
	virtual void PostGetSystem(XrInstance InInstance, XrSystemId InSystem) override;
	virtual void PostCreateSession(XrSession InSession) override;
	virtual void UpdateDeviceLocations(XrSession InSession, XrTime DisplayTime, XrSpace TrackingSpace) override;

	bool GetFaceTrackingSupported(bool& Supported, TArray<EFaceTrackingModePICO>& Modes);
	bool StartFaceTracking(EFaceTrackingModePICO Mode);
	bool StopFaceTracking();
	bool SetFaceTrackingCurrentMode(EFaceTrackingModePICO Mode);
	bool GetFaceTrackingCurrentMode(EFaceTrackingModePICO& Mode);
	bool GetFaceTrackingData(int64 DisplayTime, FFaceStatePICO& outState);



private:
	bool bCapabilityUpdated;
	bool bFaceTrackingAvailable;

	XrSession Session;
	XrTime Time;
};
