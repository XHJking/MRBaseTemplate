// Copyright 2023 PICO Inc. All Rights Reserved.

#include "PICO_MotionTracking.h"
#include "Engine/Engine.h"
#include "IXRTrackingSystem.h"
#include "OpenXRCore.h"
#include "PICOOpenXRRuntimeSettings.h"
#include "IOpenXRHMDModule.h"

FMotionTrackingPICO::FMotionTrackingPICO()
{
}

void FMotionTrackingPICO::Register()
{
	RegisterOpenXRExtensionModularFeature();
}

void FMotionTrackingPICO::Unregister()
{
	UnregisterOpenXRExtensionModularFeature();
}

bool FMotionTrackingPICO::GetRequiredExtensions(TArray<const ANSICHAR*>& OutExtensions)
{
	if (UPICOOpenXRRuntimeSettings::GetBoolConfigByKey("EnableMotionTrackingEXT"))
	{
		OutExtensions.Add("XR_PICO_motion_tracking");
		return true;
	}
	return false;
}

const void* FMotionTrackingPICO::OnGetSystem(XrInstance InInstance, const void* InNext)
{
	bSupportMotionTracking = IOpenXRHMDModule::Get().IsExtensionEnabled(XR_PICO_MOTION_TRACKING_EXTENSION_NAME);
	if (bSupportMotionTracking)
	{
		XR_ENSURE(xrGetInstanceProcAddr(InInstance, "xrRequestMotionTrackerDevicePICO", (PFN_xrVoidFunction*)&xrRequestMotionTrackerDevicePICO));
		XR_ENSURE(xrGetInstanceProcAddr(InInstance, "xrGetMotionTrackerBatteryStatePICO", (PFN_xrVoidFunction*)&xrGetMotionTrackerBatteryStatePICO));
		XR_ENSURE(xrGetInstanceProcAddr(InInstance, "xrLocateMotionTrackerPICO", (PFN_xrVoidFunction*)&xrLocateMotionTrackerPICO));
	}
	return InNext;
}

void FMotionTrackingPICO::PostCreateSession(XrSession InSession)
{
	static FName SystemName(TEXT("OpenXR"));
	if (GEngine->XRSystem.IsValid() && (GEngine->XRSystem->GetSystemName() == SystemName))
	{
		XRTrackingSystem = GEngine->XRSystem.Get();
	}
	Session = InSession;
}

void FMotionTrackingPICO::OnEvent(XrSession InSession, const XrEventDataBaseHeader* InHeader)
{
	const XrEventDataBuffer* EventDataBuffer = reinterpret_cast<const XrEventDataBuffer*>(InHeader);

	if (EventDataBuffer == nullptr)
	{
		return;
	}

	switch (EventDataBuffer->type)
	{
	case XR_TYPE_EVENT_DATA_REQUEST_MOTION_TRACKER_COMPLETE_PICO:
	{
		const XrEventDataRequestMotionTrackerCompletePICO* MotionTrackerComplete = reinterpret_cast<const XrEventDataRequestMotionTrackerCompletePICO*>(EventDataBuffer);
		if (MotionTrackerComplete)
		{
			TArray<int64> IDs = {};
			bool Result = XR_SUCCEEDED(MotionTrackerComplete->result);
			if (Result)
			{
				for (uint32 i = 0; i < MotionTrackerComplete->trackerCount; i++)
				{
					IDs.Add(MotionTrackerComplete->trackerIds[i]);
				}
			}

			UMovementFunctionLibraryPICO::OnRequestMotionTrackerCompletePICO.ExecuteIfBound(IDs, Result);
		}
	}
	break;
	case XR_TYPE_EVENT_DATA_MOTION_TRACKER_CONNECTION_STATE_CHANGED_PICO:
	{
		const XrEventDataMotionTrackerConnectionStateChangedPICO* ConnectionStateChanged = reinterpret_cast<const XrEventDataMotionTrackerConnectionStateChangedPICO*>(EventDataBuffer);
		if (ConnectionStateChanged)
		{
			UMovementFunctionLibraryPICO::OnMotionTrackerConnectionStateChangedPICO.ExecuteIfBound((int64)ConnectionStateChanged->trackerId, ConnectionStateChanged->state == XrMotionTrackerConnectionStatePICO::XR_MOTION_TRACKER_CONNECTION_STATE_CONNECTED_PICO);
		}
	}
	break;
	case XR_TYPE_EVENT_DATA_MOTION_TRACKER_POWER_KEY_EVENT_PICO:
	{
		const XrEventDataMotionTrackerPowerKeyEventPICO* PowerKey = reinterpret_cast<const XrEventDataMotionTrackerPowerKeyEventPICO*>(EventDataBuffer);
		if (PowerKey)
		{
			UMovementFunctionLibraryPICO::OnDataMotionTrackerPowerKeyStateChangedPICO.ExecuteIfBound((int64)PowerKey->trackerId, PowerKey->isLongClick == XR_TRUE);
		}
	}
	break;
	}
}

void FMotionTrackingPICO::UpdateDeviceLocations(XrSession InSession, XrTime DisplayTime, XrSpace TrackingSpace)
{
	PredictedTime = DisplayTime;
	BaseSpace = TrackingSpace;
}

bool FMotionTrackingPICO::RequestMotionTrackerDevice(int DeviceCount)
{
	if (Session != XR_NULL_HANDLE && bSupportMotionTracking)
	{
		if (XR_SUCCEEDED(xrRequestMotionTrackerDevicePICO(Session, DeviceCount)))
		{
			return true;
		}
	}
	return false;
}

bool FMotionTrackingPICO::GetMotionTrackerBatteryState(int64 ID, float& BatteryLevel, EChargingStatePICO& State)
{
	if (Session != XR_NULL_HANDLE && bSupportMotionTracking)
	{
		XrMotionTrackerBatteryStatePICO BatteryState = { XR_TYPE_MOTION_TRACKER_BATTERY_STATE_PICO };
		if (XR_SUCCEEDED(xrGetMotionTrackerBatteryStatePICO(Session, (XrMotionTrackerIdPICO)ID, &BatteryState)))
		{
			BatteryLevel = BatteryState.batteryLevel;
			State = (EChargingStatePICO)BatteryState.chargingState;
			return true;
		}
	}
	return false;
}

bool FMotionTrackingPICO::LocateMotionTracker(int64 ID, FRotator& OutRotation, FVector& OutPosition, FVector& OutLinearVelocity, FVector& OutAngularVelocity, FVector& OutLinearAcceleration, FVector& OutAngularAcceleration, bool GetVelAndAcc)
{
	if (Session != XR_NULL_HANDLE && bSupportMotionTracking)
	{
		XrMotionTrackerLocationInfoPICO LocationInfo = { XR_TYPE_MOTION_TRACKER_LOCATION_INFO_PICO };
		LocationInfo.baseSpace = BaseSpace;
		LocationInfo.time = PredictedTime;
		XrMotionTrackerSpaceLocationPICO Location = { XR_TYPE_MOTION_TRACKER_SPACE_LOCATION_PICO };
		XrMotionTrackerSpaceVelocityPICO Velocity = { XR_TYPE_MOTION_TRACKER_SPACE_VELOCITY_PICO };
		if (GetVelAndAcc)
		{
			Location.next = &Velocity;
		}
		if (XR_SUCCEEDED(xrLocateMotionTrackerPICO(Session, (XrMotionTrackerIdPICO)ID, &LocationInfo, &Location)))
		{
			const XrSpaceLocationFlags ValidFlags = XR_SPACE_LOCATION_ORIENTATION_VALID_BIT | XR_SPACE_LOCATION_POSITION_VALID_BIT;

			if ((Location.locationFlags & ValidFlags) != ValidFlags)
			{
				return false;
			}

			OutRotation = ToFQuat(Location.pose.orientation).Rotator();
			float WorldToMeter = XRTrackingSystem ? XRTrackingSystem->GetWorldToMetersScale() : 100.0f;
			OutPosition = ToFVector(Location.pose.position, WorldToMeter);

			if (GetVelAndAcc)
			{
				OutLinearVelocity = ToFVector(Velocity.linearVelocity, WorldToMeter);
				OutLinearAcceleration = ToFVector(Velocity.linearAcceleration, WorldToMeter);
				OutAngularVelocity = ToFVector(Velocity.angularVelocity, WorldToMeter);
				OutAngularAcceleration = ToFVector(Velocity.angularAcceleration, WorldToMeter);
			}

			return true;
		}
	}
	return false;
}