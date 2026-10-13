// Copyright 2023 PICO Inc. All Rights Reserved.

#pragma once

#include "PICO_IMovementModule.h"
#include "PICO_FaceTracking.h"
#include "PICO_BodyTracking.h"
#include "ILiveLinkSource.h"
#include "PICO_LiveLinkSource.h"
#include "PICO_MotionTracking.h"
#include "PICO_EyeTracking.h"
#include "PICO_ExpandDevice.h"

class FPICOOpenXRMovementModule : public IPICOOpenXRMovementModule
{
public:
	static inline FPICOOpenXRMovementModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FPICOOpenXRMovementModule>("PICOOpenXRMovement");
	}

	FFaceTrackingPICO& GetFaceTrackingPICOExtension() { return FaceTrackingPICOExtension; }
	FBodyTrackingPICO& GetBodyTrackingPICOExtension() { return BodyTrackingPICOExtension; }
	FMotionTrackingPICO& GetMotionTrackingPICOExtension() { return MotionTrackingPICOExtension; }
	FEyeTrackingPICO& GetEyeTrackingPICOExtension() { return EyeTrackingPICOExtension; }
	FExpandDevicePICO& GetFExpandDevicePICOExtension() { return ExpandDeviceExtension; }

	virtual void StartupModule() override;

	virtual void ShutdownModule() override;

	/* Live link */
	virtual TSharedPtr<ILiveLinkSource> GetLiveLinkSource();
	virtual bool IsLiveLinkSourceValid() const;
	virtual void AddLiveLinkSource();
	virtual void RemoveLiveLinkSource();

private:
	TSharedPtr<PICOLiveLink::LiveLinkSource> MovementSource{ nullptr };

public:
	FFaceTrackingPICO FaceTrackingPICOExtension;
	FBodyTrackingPICO BodyTrackingPICOExtension;
	FMotionTrackingPICO MotionTrackingPICOExtension;
	FEyeTrackingPICO EyeTrackingPICOExtension;
	FExpandDevicePICO ExpandDeviceExtension;
};