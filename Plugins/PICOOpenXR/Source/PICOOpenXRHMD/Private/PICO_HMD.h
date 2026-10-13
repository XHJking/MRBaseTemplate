// Copyright 2023 PICO Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IOpenXRExtensionPlugin.h"
#include "OpenXRHMD.h"
#include "PICO_HMDFunctionLibrary.h"

class IXRTrackingSystem;

class FHMDPICO : public IOpenXRExtensionPlugin
{
public:
	FHMDPICO();
	virtual ~FHMDPICO() {}

	void Register();
	void Unregister();

	/** IOpenXRExtensionPlugin implementation */
	virtual FString GetDisplayName() override
	{
		return FString(TEXT("HMDPICO"));
	}

	virtual bool GetCustomLoader(PFN_xrGetInstanceProcAddr* OutGetProcAddr) override;
	virtual bool GetOptionalExtensions(TArray<const ANSICHAR*>& OutExtensions) override;
	virtual void PostCreateInstance(XrInstance InInstance) override;
	virtual void PostGetSystem(XrInstance InInstance, XrSystemId InSystem) override;
	virtual const void* OnCreateSession(XrInstance InInstance, XrSystemId InSystem, const void* InNext) override;
	virtual void PostCreateSession(XrSession InSession) override;
	virtual void OnDestroySession(XrSession InSession) override;
	virtual bool UseCustomReferenceSpaceType(XrReferenceSpaceType& OutReferenceSpaceType);
	virtual bool GetSpectatorScreenController(FHeadMountedDisplayBase* InHMDBase, TUniquePtr<FDefaultSpectatorScreenController>& OutSpectatorScreenController) override;
	virtual void OnEvent(XrSession InSession, const XrEventDataBaseHeader* InHeader) override;
	virtual const void* OnEndProjectionLayer_RHIThread(XrSession InSession, int32 InLayerIndex, const void* InNext, XrCompositionLayerFlags& OutFlags) override;
	virtual void UpdateDeviceLocations(XrSession InSession, XrTime DisplayTime, XrSpace TrackingSpace) override;
	virtual void OnBeginRendering_GameThread(XrSession InSession, FSceneViewFamily& InViewFamily, TArrayView<const uint32> VisibleLayers) override;
	virtual void OnBeginRendering_RenderThread(XrSession InSession, FRDGBuilder& GraphBuilder) override;
	virtual void UpdateCompositionLayers_RHIThread(XrSession InSession, TArray<XrCompositionLayerBaseHeader*>& Headers) override;

	bool GetSupportedDisplayRefreshRates(TArray<float>& Rates);
	bool GetCurrentDisplayRefreshRate(float& Rate, bool DoubleCheck);
	bool SetDisplayRefreshRate(float Rate);

	void EnableContentProtect(bool Enable);

	bool SetPerformanceLevel(int domain, int level);
	bool GetDevicePoseForTime(const EControllerHand Hand, bool UseDefaultTime, FTimespan Timespan, bool& OutTimeWasUsed, FRotator& Orientation, FVector& Position, bool& bProvidedLinearVelocity, FVector& LinearVelocity, bool& bProvidedAngularVelocity, FVector& AngularVelocityRadPerSec, bool& bProvidedLinearAcceleration, FVector& LinearAcceleration, float InWorldToMetersScale);
	EHMDWornState::Type GetHMDWornState(bool& ResultValid);
	bool GetFieldOfView(float& OutHFOVInDegrees, float& OutVFOVInDegrees);
	bool GetInterpupillaryDistance(float& IPD);
	void SetBaseRotationAndBaseOffset(FRotator Rotation, FVector BaseOffset, EOrientPositionSelector::Type Options);
	void GetBaseRotationAndBaseOffset(FRotator& OutRotation, FVector& OutBaseOffset);
    FTimespan GetDisplayTime();

	bool IsStationaryBoundaryMode(bool& bIsStationary);
	bool GetVirtualBoundaryStatus(bool& bIsReady, bool& bIsEnable, bool& bIsVisible);
	bool SetVirtualBoundaryEnable(bool bEnable);
	bool SetVirtualBoundaryVisible(bool bVisible);
	bool SetVirtualBoundarySeeThroughVisible(bool bVisible);
	bool BoundaryintersectPointOrNode(bool bPoint, EControllerHand Node, FVector Point, EBoundaryTypePICO BoundaryType, bool& Valid, bool& IsTriggering, float& ClosestDistance, FVector& ClosestPoint, FVector& ClosestPointNormal, float InWorldToMetersScale = 100);
	bool GetBoundaryGeometry(EBoundaryTypePICO BoundaryType, bool& Valid, TArray<FVector>& Points, float InWorldToMetersScale);

	void CopyTexture_RenderThread(FRHICommandListImmediate& RHICmdList, FRHITexture* DstTexture, FRHITexture* SrcTexture, FIntRect DstRect = FIntRect(), FIntRect SrcRect = FIntRect(), bool bAlphaPremultiply = false, bool bNoAlpha = false, bool bClearGreen = false, bool bInvertX = false, bool bInvertY = false, bool bInvertAlpha = false) const;
private:
	void*								 LoaderHandle;
	bool								 bPICORuntime;
	FRWLock								 SessionHandleMutex;
	XrInstance							 Instance;
	XrSystemId							 System;
	XrSession							 Session;
	XrPath								 CommonInteractionProfile;

	bool								 bSupportLocalFloorLevelEXT;

	bool								 bSupportDisplayRefreshRate;
	float								 CurrentDisplayRefreshRate;
	TArray<float>						 SupportedDisplayRefreshRates;
	PFN_xrEnumerateDisplayRefreshRatesFB xrEnumerateDisplayRefreshRatesFB = nullptr;
	PFN_xrGetDisplayRefreshRateFB		 xrGetDisplayRefreshRateFB = nullptr;
	PFN_xrRequestDisplayRefreshRateFB	 xrRequestDisplayRefreshRateFB = nullptr;

	bool								 bContentProtectEnabled;
	XrCompositionLayerSecureContentFB	 ContentProtect;
	bool								 bSupportPerformanceSettingsEXT;
	PFN_xrPerfSettingsSetPerformanceLevelEXT xrPerfSettingsSetPerformanceLevelEXT = nullptr;

	XrTime                               CurrentDisplayTime;
	class FOpenXRHMD*				     OpenXRHMD = nullptr;
	bool                                 IsSupportsUserPresence;
	EHMDWornState::Type     			 WornState;
	
	bool								 bSupportLayerDepth = false;

	bool								 bSupportedVirtualBoundary = false;
	PFN_xrGetVirtualBoundaryModePICO     xrGetVirtualBoundaryModePICO = nullptr;
	PFN_xrGetVirtualBoundaryStatusPICO   xrGetVirtualBoundaryStatusPICO = nullptr;
	PFN_xrSetVirtualBoundaryEnablePICO	 xrSetVirtualBoundaryEnablePICO = nullptr;
	PFN_xrSetVirtualBoundaryVisiblePICO	 xrSetVirtualBoundaryVisiblePICO = nullptr;
	PFN_xrSetVirtualBoundarySeeThroughVisiblePICO xrSetVirtualBoundarySeeThroughVisiblePICO = nullptr;
	PFN_xrGetVirtualBoundaryTriggerPICO	 xrGetVirtualBoundaryTriggerPICO = nullptr;
	PFN_xrGetVirtualBoundaryGeometryPICO xrGetVirtualBoundaryGeometryPICO = nullptr;

	XrSpace								 CurrentBaseSpace = XR_NULL_HANDLE;

private:

	FIntRect GetViewportSize(const FOpenXRLayer::FPerEyeTextureData& EyeData, const IStereoLayers::FLayerDesc& Desc);
	FVector2D GetQuadSize(const FOpenXRLayer::FPerEyeTextureData& EyeData, const IStereoLayers::FLayerDesc& Desc);

	void OnWorldTickStart(UWorld* World, ELevelTick TickType, float DeltaTime);
	FDelegateHandle OnWorldTickStartDelegateHandle;

	/*************************** MRC Begin ***************************/
public:
	bool bSupportMRCExtension;
	void CreateMRCLayer(class UTexture* BackgroundRTTexture, class UTexture* ForegroundRTTexture);
	void DestroyMRCLayer();
	bool GetExternalCameraInfo(int32& width, int32& height, float& fov);
	bool GetExternalCameraPose(FTransform& Pose);
	void EnableMRCDebugMode(class UWorld* WorldContext, bool Enable, bool ViewInHMD, bool UseCustomTransform, const FTransform& Pose, bool UseCustomCameraInfo, int Width, int Height, float Fov);
	void DisableMRCForegroundLayer(UObject* WorldContextObject, bool Disable);
	void PauseMRC(bool Pause);
	bool								 bIsMRCForegroundLayerDisabled = false;
private:
	PFN_xrGetExternalCameraInfoPICO		 xrGetExternalCameraInfoPICO = nullptr;
	XrSpace								 ViewTrackingSpace = XR_NULL_HANDLE;
	XrSpace								 MRCSpace = XR_NULL_HANDLE;
	bool								 bIsMRCRunning = false;
	bool								 bIsMRCRunningStored = false;
	bool								 bIsMRCForegroundLayerDisabled_RebderThread = false;
	
	class AMRCCameraPICO*				 MRCSceneCapture2DPICO = nullptr;
	IStereoLayers::FLayerDesc			 MRCLayerDesc_RenderThread;
	TSharedPtr<FOpenXRLayer, ESPMode::ThreadSafe> MRCLayer;
	XrCompositionLayerQuad				 MRCQuadLayerLeft_RenderThread;
	XrCompositionLayerQuad				 MRCQuadLayerLeft_RHIThread;
	XrCompositionLayerQuad				 MRCQuadLayerRight_RenderThread;
	XrCompositionLayerQuad				 MRCQuadLayerRight_RHIThread;

	struct FMRCDebugModePICO
	{
		FMRCDebugModePICO()
			:EnableExtension(false)
			, ViewInHMD(false)
			, UseCustomTransform(false)
			, Pose(FTransform::Identity)
			, UseCustomCameraInfo(false)
			, Width(256)
			, Height(256)
			, Fov(90.0f)
		{}

		bool EnableExtension;
		bool ViewInHMD;
		bool UseCustomTransform;
		FTransform Pose;
		bool UseCustomCameraInfo;
		int Width;
		int Height;
		float Fov;
	};

	FMRCDebugModePICO					 MRCDebugMode;

	/*************************** MRC End ***************************/

	bool								 bSupportedBDCompositionLayerSettingsExt;
	XrLayerSettingsPICO					 ProjectionLayerSettings;

	XrCompositionLayerAlphaBlendFB		 BlendState = { XR_TYPE_COMPOSITION_LAYER_ALPHA_BLEND_FB };

	bool								 bSupportAdaptiveResolution = false;
	PFN_xrUpdateAdaptiveResolutionPICO	 xrUpdateAdaptiveResolutionPICO = nullptr;
	
	bool								 bSupportColorMatrixExtension = false;
	bool								 bUseColorMatrixExtension = false;
	float ColorMatrix3x3f[9] = {};
public:
	bool GetAdaptivePixelDensity(EAdaptiveResolutionSettingPICO Setting, float& PixelDensity);
	FIntPoint GetDefaultRenderTargetSize();
	void GetCurrentRenderTargetSize(uint32& InOutSizeX, uint32& InOutSizeY);
	bool SetProjectionLayerColorMatrix3x3f(bool Enable, FVector3f ColumnA, FVector3f ColumnB, FVector3f ColumnC);

	bool GetViewportSize(FIntPoint& Size);
	bool								 bDynamicResolution = false;
	float 							     CurrentDynamicPixelDensity = 1.0f;
	float							     MinimumResolutionScale = 0.6f;
	EAdaptiveResolutionSettingPICO 	     CurrentAdaptiveResolutionSetting = EAdaptiveResolutionSettingPICO::Balanced;
};
