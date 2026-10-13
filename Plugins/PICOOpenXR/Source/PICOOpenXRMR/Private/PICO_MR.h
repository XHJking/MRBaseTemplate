// Copyright 2023 PICO Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IOpenXRExtensionPlugin.h"
#include "PICO_MRTypes.h"
DECLARE_DELEGATE_OneParam(FPICOPollFutureDelegate, const XrFutureEXT&);

class FSpatialSensingExtensionPICO : public IOpenXRExtensionPlugin
{
public:
	FSpatialSensingExtensionPICO();
	virtual ~FSpatialSensingExtensionPICO() {}

	void Register();
	void Unregister();

	static FSpatialSensingExtensionPICO* GetInstance()
	{
		static FSpatialSensingExtensionPICO Instance;
		return &Instance;
	}

	static EResultPICO CastToPICOResult(XrResult Result);

	/** IOpenXRExtensionPlugin implementation */
	virtual FString GetDisplayName() override
	{
		return FString(TEXT("SpatialSensingPICO"));
	}
	
	virtual void OnEvent(XrSession InSession, const XrEventDataBaseHeader* InHeader) override;
	virtual bool GetOptionalExtensions(TArray<const ANSICHAR*>& OutExtensions) override;
	virtual void PostGetSystem(XrInstance InInstance, XrSystemId InSystem) override;
	virtual void PostCreateSession(XrSession InSession) override;
	virtual void UpdateDeviceLocations(XrSession InSession, XrTime DisplayTime, XrSpace TrackingSpace) override;
	
	bool StartSenseDataProviderComplete(const XrFutureEXT& FutureHandle, FSenseDataProviderStartCompletionPICO& Completion,EResultPICO& OutResult);
	bool QuerySenseDataComplete(const XrSenseDataProviderPICO& ProviderHandle,const XrFutureEXT& FutureHandle, FSenseDataQueryCompletionPICO& Completion,EResultPICO& OutResult);
	bool GetQueriedSenseData(const XrSenseDataProviderPICO& ProviderHandle,const XrSenseDataSnapshotPICO& QueryResultHandle, FQueriedSenseDataPICO& QueriedSenseData,EResultPICO& OutResult);
	bool DestroySenseDataQueryResult(const XrSenseDataSnapshotPICO& QueryResultHandle,EResultPICO& OutResult);
	bool GetSpatialEntityLocation(const XrSenseDataSnapshotPICO& SnapshotHandle,const XrSpatialEntityIdPICO& EntityHandle, FTransform& Transform,EResultPICO& OutResult);
	bool GetSpatialEntitySemantic(const XrSenseDataSnapshotPICO& SnapshotHandle,const XrSpatialEntityIdPICO& EntityHandle, TArray<ESemanticLabelPICO>& Semantics,EResultPICO& OutResult);
	bool GetSpatialEntityBoundary3D(const XrSenseDataSnapshotPICO& SnapshotHandle,const XrSpatialEntityIdPICO& EntityHandle, FBoundingBox3DPICO& Box,EResultPICO& OutResult);
	bool GetSpatialEntityBoundary2D(const XrSenseDataSnapshotPICO& SnapshotHandle,const XrSpatialEntityIdPICO& EntityHandle, FBoundingBox2DPICO& Box,EResultPICO& OutResult);
	bool GetSpatialEntityPolygon(const XrSenseDataSnapshotPICO& SnapshotHandle,const XrSpatialEntityIdPICO& EntityHandle, TArray<FVector>& Vertices,EResultPICO& OutResult);
	bool GetSpatialEntityTriangleMesh(const XrSenseDataSnapshotPICO& SnapshotHandle,const XrSpatialEntityIdPICO& EntityHandle, TArray<FVector>& Vertices, TArray<uint16>& Triangles,EResultPICO& OutResult);
	
	bool EnumerateSpatialEntityComponentTypes(const XrSenseDataSnapshotPICO& SnapshotHandle,const XrSpatialEntityIdPICO& EntityHandle, TArray<ESpatialEntityComponentTypePICO>& componentTypes,EResultPICO& OutResult);
	bool AddPollFutureRequirement(const XrFutureEXT& FutureHandle, const FPICOPollFutureDelegate& Delegate);

	bool IsSupportsSpatialSensing() const {return bSupportsSpatialSensing; }
	bool IsSupportsSpatialSensingEXT() const {return bSupportsSpatialSensingEXT; }

	// XR_PICO_spatial_sensing
	PFN_xrCreateSenseDataProviderPICO			xrCreateSenseDataProviderPICO		 = nullptr;
	PFN_xrStartSenseDataProviderAsyncPICO		xrStartSenseDataProviderAsyncPICO	 = nullptr;
	PFN_xrStartSenseDataProviderCompletePICO	xrStartSenseDataProviderCompletePICO = nullptr;
	PFN_xrGetSenseDataProviderStatePICO			xrGetSenseDataProviderStatePICO		 = nullptr;
	PFN_xrQuerySenseDataAsyncPICO			    xrQuerySenseDataAsyncPICO			 = nullptr;
	PFN_xrQuerySenseDataCompletePICO			xrQuerySenseDataCompletePICO		 = nullptr;
	PFN_xrStopSenseDataProviderPICO				xrStopSenseDataProviderPICO			 = nullptr;
	PFN_xrDestroySenseDataProviderPICO			xrDestroySenseDataProviderPICO		 = nullptr;
	PFN_xrDestroySenseDataSnapshotPICO          xrDestroySenseDataSnapshotPICO		 = nullptr;
	PFN_xrGetQueriedSenseDataPICO			    xrGetQueriedSenseDataPICO			 = nullptr;
	PFN_xrPollFutureEXT							xrPollFutureEXT						 = nullptr;
	PFN_xrGetSpatialEntityUuidPICO				xrGetSpatialEntityUuidPICO			 = nullptr;
	PFN_xrGetSpatialEntityComponentDataPICO     xrGetSpatialEntityComponentDataPICO	 = nullptr;
	PFN_xrEnumerateSpatialEntityComponentTypesPICO xrEnumerateSpatialEntityComponentTypesPICO = nullptr;

	PFN_xrRetrieveSpatialEntityAnchorPICO		xrRetrieveSpatialEntityAnchorPICO	= nullptr;
	PFN_xrDestroyAnchorPICO						xrDestroyAnchorPICO					= nullptr;
	PFN_xrGetAnchorUuidPICO						xrGetAnchorUuidPICO					= nullptr;
	PFN_xrLocateAnchorPICO						xrLocateAnchorPICO					= nullptr;


protected:
	EProviderTypePICO GetProviderTypeByHandle(const XrSenseDataProviderPICO& Handle);
	uint64 GetUUID();

private:
	void ApplicationResumeDelegate();
	void PollEvent(const XrEventDataBuffer* EventData);
	void PXR_PollFuture();
	
	TQueue<FFutureMessagePICO> FutureQueueForProviders;
	TMap<uint64_t, FPICOPollFutureDelegate> FutureToDelegateMap;
	
	class IXRTrackingSystem* XRTrackingSystem = nullptr;
	XrTime CurrentDisplayTime;
	XrSession Session = XR_NULL_HANDLE;
	XrInstance Instance = XR_NULL_HANDLE;
	XrSpace TrackingSpace = XR_NULL_HANDLE;
	
	bool bSupportsSpatialSensingEXT = false;
	bool bSupportsSpatialSensing = false;

	uint64 GlobalUUIDCount;
	FRWLock DestroyLock;
};

class FMixedRealityPICO :public IOpenXRExtensionPlugin
{
public:
	FMixedRealityPICO();
	virtual ~FMixedRealityPICO() {}

	void Register();
	void Unregister();
	
	virtual void OnDestroySession(XrSession InSession) override;
	virtual void PostCreateSession(XrSession InSession) override;
	virtual void UpdateDeviceLocations(XrSession InSession, XrTime DisplayTime, XrSpace InTrackingSpace) override;
	
	virtual bool CreateProvider(const FSenseDataProviderCreateInfoBasePICO& CreateInfo,EResultPICO& OutResult) =0;
	bool StartProvider(const FPICOPollFutureDelegate& StartSenseDataProviderDelegate,EResultPICO& OutResult);
	bool StopProvider(EResultPICO& OutResult);
	bool DestroyProvider(EResultPICO& OutResult);
	EMRStatePICO GetSenseDataProviderState() const;

	bool IsHandleValid() const { return ProviderHandle!=XR_NULL_HANDLE; }
	bool IsInitialized() const { return GetSenseDataProviderState() == EMRStatePICO::Initialized; }
	bool IsRunning() const { return GetSenseDataProviderState() == EMRStatePICO::Running; }
	bool IsEqualProvider(const XrSenseDataProviderPICO& Handle) const { return ProviderHandle==Handle;}
protected:
	EProviderTypePICO GetProviderType();
	
	EProviderTypePICO Type=EProviderTypePICO::Pico_Provider_Unknown;
	XrSenseDataProviderPICO ProviderHandle=XR_NULL_HANDLE;
	
	class IXRTrackingSystem* XRTrackingSystem = nullptr;
	XrTime CurrentDisplayTime;
	XrSession Session = XR_NULL_HANDLE;
	XrInstance Instance = XR_NULL_HANDLE;
	XrSpace TrackingSpace = XR_NULL_HANDLE;
};