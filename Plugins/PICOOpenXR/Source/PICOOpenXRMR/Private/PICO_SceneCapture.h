// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "PICO_MR.h"

class FSceneCaptureExtensionPICO :public FMixedRealityPICO
{
public:
	FSceneCaptureExtensionPICO();
	virtual ~FSceneCaptureExtensionPICO() {}
	
	/** IOpenXRExtensionPlugin implementation */
	virtual FString GetDisplayName() override
	{
		return FString(TEXT("SceneCapturePICO"));
	}

	static FSceneCaptureExtensionPICO* GetInstance()
	{
		static FSceneCaptureExtensionPICO Instance;
		return &Instance;
	}
	
	virtual bool CreateProvider(const FSenseDataProviderCreateInfoBasePICO& CreateInfo,EResultPICO& OutResult) override;

	virtual bool GetOptionalExtensions(TArray<const ANSICHAR*>& OutExtensions) override;
	virtual void PostGetSystem(XrInstance InInstance, XrSystemId InSystem) override;
	virtual void PostCreateSession(XrSession InSession) override;
	
	bool RequestSpatialSceneInfos(const FSceneLoadInfoPICO& LoadInfo, const FPICOPollFutureDelegate& Delegate,EResultPICO& OutResult);
	bool GetSpatialSceneInfos(const XrFutureEXT& FutureHandle,TArray<FMRSceneInfoPICO>& SceneLoadInfos,EResultPICO& OutResult);
	bool StartSceneCaptureAsync(const FPICOPollFutureDelegate& Delegate,EResultPICO& OutResult);
	bool StartSceneCaptureComplete(const XrFutureEXT& FutureHandle,FSceneCaptureStartCompletionPICO& cPICOSceneCaptureStartCompletion,EResultPICO& OutResult);

	void ClearComponentBuffer();

	bool GetSpatialSceneBoundingBox3D(const FSpatialUUIDPICO& UUID, FBoundingBox3DPICO& Box);
	bool GetSpatialSceneBoundingBox2D(const FSpatialUUIDPICO& UUID, FBoundingBox2DPICO& Box2D);
	bool GetSpatialSceneBoundingPolygon(const FSpatialUUIDPICO& UUID, TArray<FVector>& Polygon);
	
private:
	// XR_PICO_scene_capture
	PFN_xrStartSceneCaptureAsyncPICO			xrStartSceneCaptureAsyncPICO		= nullptr;
	PFN_xrStartSceneCaptureCompletePICO			xrStartSceneCaptureCompletePICO		= nullptr;
	
	TMap<FSpatialUUIDPICO, TSharedRef<FBoundingBox3DPICO>> EntityToBoundingBox3DMap;
	TMap<FSpatialUUIDPICO, TSharedRef<FBoundingBox2DPICO>> EntityToBoundingBox2DMap;
	TMap<FSpatialUUIDPICO,TSharedRef<TArray<FVector>>> EntityToPolygonMap;

	bool bSupportsSceneCaptureEXT = false;
	bool bSupportsSceneCapture = false;

};