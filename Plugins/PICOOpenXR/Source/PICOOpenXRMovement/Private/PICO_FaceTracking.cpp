// Copyright 2023 PICO Inc. All Rights Reserved.

#include "PICO_FaceTracking.h"
#include "PICOOpenXRRuntimeSettings.h"
#include "OpenXRCore.h"

#if PLATFORM_ANDROID
#include <dlfcn.h> 
#endif //PLATFORM_ANDROID

FFaceTrackingPICO::FFaceTrackingPICO()
	: bCapabilityUpdated(false)
	, bFaceTrackingAvailable(false)
	, Session(XR_NULL_HANDLE)
	, Time(0)
{
}

void FFaceTrackingPICO::Register()
{
	RegisterOpenXRExtensionModularFeature();
}

void FFaceTrackingPICO::Unregister()
{
	UnregisterOpenXRExtensionModularFeature();
}

bool FFaceTrackingPICO::GetRequiredExtensions(TArray<const ANSICHAR*>& OutExtensions)
{
	return false;
}

const void* FFaceTrackingPICO::OnGetSystem(XrInstance InInstance, const void* InNext)
{
	return InNext;
}

void FFaceTrackingPICO::PostGetSystem(XrInstance InInstance, XrSystemId InSystem)
{
}

void FFaceTrackingPICO::PostCreateSession(XrSession InSession)
{
	Session = InSession;
}

void FFaceTrackingPICO::UpdateDeviceLocations(XrSession InSession, XrTime DisplayTime, XrSpace TrackingSpace)
{
	Time = DisplayTime;
}

bool FFaceTrackingPICO::GetFaceTrackingSupported(bool& Supported, TArray<EFaceTrackingModePICO>& Modes)
{
	Supported = bFaceTrackingAvailable;
	return bCapabilityUpdated;
}

bool FFaceTrackingPICO::StartFaceTracking(EFaceTrackingModePICO Mode)
{
	return false;
}

bool FFaceTrackingPICO::StopFaceTracking()
{
	return false;
}

bool FFaceTrackingPICO::SetFaceTrackingCurrentMode(EFaceTrackingModePICO Mode)
{
	return false;
}

bool FFaceTrackingPICO::GetFaceTrackingCurrentMode(EFaceTrackingModePICO& Mode)
{
	return false;
}

bool FFaceTrackingPICO::GetFaceTrackingData(int64 DisplayTime, FFaceStatePICO& outState)
{
	return false;
}
