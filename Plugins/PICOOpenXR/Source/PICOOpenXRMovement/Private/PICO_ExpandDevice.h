// Copyright 2023 PICO Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PICO_MovementFunctionLibrary.h"
#include "IOpenXRExtensionPlugin.h"

class FExpandDevicePICO : public IOpenXRExtensionPlugin
{
public:
	FExpandDevicePICO();
	virtual ~FExpandDevicePICO() {}

	void Register();
	void Unregister();

	/** IOpenXRExtensionPlugin */
	virtual FString GetDisplayName() override
	{
		return FString(TEXT("ExpandDevicePICO"));
	}
	virtual bool GetRequiredExtensions(TArray<const ANSICHAR*>& OutExtensions) override;
	virtual const void* OnGetSystem(XrInstance InInstance, const void* InNext) override;
	virtual void PostCreateSession(XrSession InSession) override;

	bool EnumerateExpandDevice(TArray<int64>& IDs);
	bool SetExpandDeviceMotorVibrate(int64 ID, int64 Duration, int32 Frequency, float Amp);
	bool GetExpandDeviceBatteryState(int64 ID, float& BatteryLevel, EChargingStatePICO& ChargingState);
	bool SetExpandDeviceCustomDataCapability(bool Enable);
	bool SetExpandDeviceCustomData(const TArray<FExpandDeviceDataPICO>& Datas);
	bool GetExpandDeviceCustomData(TArray<FExpandDeviceDataPICO>& Datas);
private:
	XrSession Session = XR_NULL_HANDLE;
	bool bSupportExpandDeviceEXT = false;
    bool bCustomDataCapability = false;

	PFN_xrEnumerateExpandDevicePICO xrEnumerateExpandDevicePICO = nullptr;
	PFN_xrSetExpandDeviceMotorVibratePICO xrSetExpandDeviceMotorVibratePICO = nullptr;
	PFN_xrGetExpandDeviceBatteryStatePICO xrGetExpandDeviceBatteryStatePICO = nullptr;
	PFN_xrSetExpandDeviceCustomDataCapabilityPICO xrSetExpandDeviceCustomDataCapabilityPICO = nullptr;
	PFN_xrSetExpandDeviceCustomDataPICO xrSetExpandDeviceCustomDataPICO = nullptr;
	PFN_xrGetExpandDeviceCustomDataPICO xrGetExpandDeviceCustomDataPICO = nullptr;
};
