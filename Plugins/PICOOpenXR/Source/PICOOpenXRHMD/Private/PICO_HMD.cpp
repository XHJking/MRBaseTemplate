// Copyright 2023 PICO Inc. All Rights Reserved.

#include "PICO_HMD.h"
#include "PICOOpenXRRuntimeSettings.h"
#include "OpenXRCore.h"
#include "IXRTrackingSystem.h"
#include "OpenXRCore.h"
#include "IOpenXRHMDModule.h"
#include "OpenXRHMD_Swapchain.h"
#include "OpenXRHMD_RenderBridge.h"
#include "PixelShaderUtils.h"
#include "Shader.h"
#include "ClearQuad.h"
#include "ScreenRendering.h"
#include "PICO_Shaders.h"
#include "HDRHelper.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "TextureResource.h"
#include "GameFramework/Actor.h"
#include "PICO_MRCCamera.h"
#include "Runtime/Launch/Resources/Version.h"
#include "PICO_DynamicResolutionState.h"

#if PLATFORM_ANDROID
#include <dlfcn.h> 
#endif //PLATFORM_ANDROID

static TAutoConsoleVariable<int32> CVarPICOEnableSuperResolution(
	TEXT("r.Mobile.PICO.EnableSuperResolution"),
	0,
	TEXT("0: Disable SuperResolution (Default)\n")
	TEXT("1: Enable SuperResolution on supported platforms\n"),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarPICOSharpeningSetting(
	TEXT("r.Mobile.PICO.SharpeningSetting"),
	0,
	TEXT("0: Disable Sharpening (Default)\n")
	TEXT("1: Enable NormalSharpening on supported platforms\n")
	TEXT("2: Enable QualitySharpening on supported platforms\n"),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarPICOSharpeningEnhanceMode(
	TEXT("r.Mobile.PICO.SharpeningEnhanceMode"),
	0,
	TEXT("0: Disable Sharpening EnhanceMode (Default)\n")
	TEXT("1: Enable Fixed Foveated on supported platforms\n")
	TEXT("2: Enable Adaptive on supported platforms\n")
	TEXT("3: Enable FixedFoveated and Adaptive on supported platforms\n"),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarPICODynamicResolutionPixelDensity(
	TEXT("r.PICO.DynamicResolution.PixelDensity"),
	0,
	TEXT("0 Static Pixel Density corresponding to Pixel Density 1.0 (default)\n")
	TEXT(">0 Manual Pixel Density Override\n"),
	ECVF_Default);

FHMDPICO::FHMDPICO()
	: LoaderHandle(nullptr)
	, bPICORuntime(false)
	, Instance(XR_NULL_HANDLE)
	, System(XR_NULL_SYSTEM_ID)
	, Session(XR_NULL_HANDLE)
	, bSupportLocalFloorLevelEXT(false)
	, bSupportDisplayRefreshRate(false)
	, CurrentDisplayRefreshRate(0)
	, bContentProtectEnabled(false)
	, CurrentDisplayTime(0)
	, IsSupportsUserPresence(false)
	, WornState(EHMDWornState::Type::Unknown)
	, bSupportMRCExtension(false)
	, bIsMRCRunning(false)
	, MRCDebugMode(FMRCDebugModePICO())
	, bSupportedBDCompositionLayerSettingsExt(false)
	, ProjectionLayerSettings({ XR_TYPE_LAYER_SETTINGS_PICO })
{
	SupportedDisplayRefreshRates.Empty();

	BlendState.srcFactorColor = XrBlendFactorFB::XR_BLEND_FACTOR_DST_ALPHA_FB;
	BlendState.dstFactorColor = XrBlendFactorFB::XR_BLEND_FACTOR_ONE_MINUS_DST_ALPHA_FB;
	BlendState.srcFactorAlpha = XrBlendFactorFB::XR_BLEND_FACTOR_ZERO_FB;
	BlendState.dstFactorAlpha = XrBlendFactorFB::XR_BLEND_FACTOR_ONE_FB;
}

void FHMDPICO::Register()
{
	RegisterOpenXRExtensionModularFeature();
	OnWorldTickStartDelegateHandle = FWorldDelegates::OnWorldTickStart.AddRaw(this, &FHMDPICO::OnWorldTickStart);
}

void FHMDPICO::Unregister()
{
	UnregisterOpenXRExtensionModularFeature();
	if (LoaderHandle)
	{
		FPlatformProcess::FreeDllHandle(LoaderHandle);
		LoaderHandle = nullptr;
	}

	if (OnWorldTickStartDelegateHandle.IsValid())
	{
		FWorldDelegates::OnWorldTickStart.Remove(OnWorldTickStartDelegateHandle);
		OnWorldTickStartDelegateHandle.Reset();
	}
}

bool FHMDPICO::GetCustomLoader(PFN_xrGetInstanceProcAddr* OutGetProcAddr)
{
#if PLATFORM_ANDROID
	// clear errors
	dlerror();

	LoaderHandle = FPlatformProcess::GetDllHandle(TEXT("libopenxr_loader_pico.so"));
	if (LoaderHandle == nullptr)
	{
		UE_LOG(LogPICOOpenXRHMD, Error, TEXT("Unable to load libopenxr_loader_pico.so, error %s"), ANSI_TO_TCHAR(dlerror()));
		return false;
	}

	// clear errors
	dlerror();

	PFN_xrGetInstanceProcAddr xrGetInstanceProcAddrPtr = (PFN_xrGetInstanceProcAddr)FPlatformProcess::GetDllExport(LoaderHandle, TEXT("xrGetInstanceProcAddr"));
	if (xrGetInstanceProcAddrPtr == nullptr)
	{
		UE_LOG(LogPICOOpenXRHMD, Error, TEXT("Unable to load OpenXR xrGetInstanceProcAddr, error %s"), ANSI_TO_TCHAR(dlerror()));
		return false;
	}
	*OutGetProcAddr = xrGetInstanceProcAddrPtr;

	extern struct android_app* GNativeAndroidApp;
	PFN_xrInitializeLoaderKHR xrInitializeLoaderKHR;
	xrGetInstanceProcAddrPtr(XR_NULL_HANDLE, "xrInitializeLoaderKHR", (PFN_xrVoidFunction*)&xrInitializeLoaderKHR);
	if (xrInitializeLoaderKHR == nullptr)
	{
		UE_LOG(LogPICOOpenXRHMD, Error, TEXT("Unable to load OpenXR xrInitializeLoaderKHR"));
		return false;
	}
	XrLoaderInitInfoAndroidKHR LoaderInitializeInfoAndroid;
	LoaderInitializeInfoAndroid.type = XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR;
	LoaderInitializeInfoAndroid.next = NULL;
	LoaderInitializeInfoAndroid.applicationVM = GNativeAndroidApp->activity->vm;
	LoaderInitializeInfoAndroid.applicationContext = GNativeAndroidApp->activity->clazz;
	XR_ENSURE(xrInitializeLoaderKHR((XrLoaderInitInfoBaseHeaderKHR*)&LoaderInitializeInfoAndroid));

	//Used to determine whether the pico runtime loads successfully
	{
		PFN_xrEnumerateInstanceExtensionProperties xrEnumerateInstanceExtensionPropertiesPtr;
		xrGetInstanceProcAddrPtr(XR_NULL_HANDLE, "xrEnumerateInstanceExtensionProperties", (PFN_xrVoidFunction*)&xrEnumerateInstanceExtensionPropertiesPtr);
		if (xrEnumerateInstanceExtensionPropertiesPtr == nullptr)
		{
			UE_LOG(LogPICOOpenXRHMD, Error, TEXT("Unable to load OpenXR xrEnumerateInstanceExtensionProperties!"));
			return false;
		}

		uint32_t ExtensionsCount = 0;
		if (XR_FAILED(xrEnumerateInstanceExtensionPropertiesPtr(nullptr, 0, &ExtensionsCount, nullptr)))
		{
			UE_LOG(LogPICOOpenXRHMD, Error, TEXT("xrEnumerateInstanceExtensionPropertiesPtr Failed!"));
			return false;
		}
	}

	UE_LOG(LogPICOOpenXRHMD, Log, TEXT("Loaded PICO OpenXR Loader"));
	bPICORuntime = true;
	return true;
#endif //PLATFORM_ANDROID
	return false;
}

bool FHMDPICO::GetOptionalExtensions(TArray<const ANSICHAR*>& OutExtensions)
{
	OutExtensions.Add("XR_FB_display_refresh_rate");
	OutExtensions.Add("XR_EXT_local_floor");
	OutExtensions.Add("XR_FB_composition_layer_secure_content");
	OutExtensions.Add("XR_EXT_performance_settings");
	OutExtensions.Add("XR_EXT_user_presence");
	OutExtensions.Add("XR_PICO_virtual_boundary");
	OutExtensions.Add("XR_PICO_external_camera");
	OutExtensions.Add("XR_PICO_layer_settings");
#if UE_VERSION_NEWER_THAN(5, 5, 0)
	OutExtensions.Add("XR_PICO_adaptive_resolution");
#endif
	OutExtensions.Add("XR_PICO_layer_color_matrix");
	
	if (UPICOOpenXRRuntimeSettings::GetBoolConfigByKey("bEnableLayerDepth"))
	{
		OutExtensions.Add("XR_FB_composition_layer_alpha_blend");
		bSupportLayerDepth = true;
	}
	
	return true;
}

void FHMDPICO::PostCreateInstance(XrInstance InInstance)
{
	Instance = InInstance;
	XrInstanceProperties InstanceProps = { XR_TYPE_INSTANCE_PROPERTIES, nullptr };
	XR_ENSURE(xrGetInstanceProperties(InInstance, &InstanceProps));
	InstanceProps.runtimeName[XR_MAX_RUNTIME_NAME_SIZE - 1] = 0; // Ensure the name is null terminated.
	FString RuntimeName = FString(InstanceProps.runtimeName);
	UE_LOG(LogPICOOpenXRHMD, Log, TEXT("PICO OpenXR PostCreateInstance RuntimeName:%s"), *RuntimeName);
}

void FHMDPICO::PostGetSystem(XrInstance InInstance, XrSystemId InSystem)
{
	System = InSystem;
	bSupportDisplayRefreshRate = IOpenXRHMDModule::Get().IsExtensionEnabled(XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_NAME);

	if (bSupportDisplayRefreshRate)
	{
		XR_ENSURE(xrGetInstanceProcAddr(InInstance, "xrEnumerateDisplayRefreshRatesFB", (PFN_xrVoidFunction*)&xrEnumerateDisplayRefreshRatesFB));
		XR_ENSURE(xrGetInstanceProcAddr(InInstance, "xrGetDisplayRefreshRateFB", (PFN_xrVoidFunction*)&xrGetDisplayRefreshRateFB));
		XR_ENSURE(xrGetInstanceProcAddr(InInstance, "xrRequestDisplayRefreshRateFB", (PFN_xrVoidFunction*)&xrRequestDisplayRefreshRateFB));
	}
	
	bSupportLocalFloorLevelEXT = IOpenXRHMDModule::Get().IsExtensionEnabled(XR_EXT_LOCAL_FLOOR_EXTENSION_NAME);
	bSupportPerformanceSettingsEXT = IOpenXRHMDModule::Get().IsExtensionEnabled(XR_EXT_PERFORMANCE_SETTINGS_EXTENSION_NAME);
	if (bSupportPerformanceSettingsEXT)
	{
		XR_ENSURE(xrGetInstanceProcAddr(InInstance, "xrPerfSettingsSetPerformanceLevelEXT", (PFN_xrVoidFunction*)&xrPerfSettingsSetPerformanceLevelEXT));
	}

	if (IOpenXRHMDModule::Get().IsExtensionEnabled(XR_EXT_USER_PRESENCE_EXTENSION_NAME))
	{
		XrSystemUserPresencePropertiesEXT SystemUserPresenceProperties = { XR_TYPE_SYSTEM_USER_PRESENCE_PROPERTIES_EXT };
		XrSystemProperties systemProperties = { XR_TYPE_SYSTEM_PROPERTIES, &SystemUserPresenceProperties };
		XR_ENSURE(xrGetSystemProperties(InInstance, System, &systemProperties));
		IsSupportsUserPresence = SystemUserPresenceProperties.supportsUserPresence == XR_TRUE;
	}

	if (IOpenXRHMDModule::Get().IsExtensionEnabled(XR_PICO_VIRTUAL_BOUNDARY_EXTENSION_NAME))
	{
		XR_ENSURE(xrGetInstanceProcAddr(InInstance, "xrGetVirtualBoundaryModePICO", (PFN_xrVoidFunction*)&xrGetVirtualBoundaryModePICO));
		XR_ENSURE(xrGetInstanceProcAddr(InInstance, "xrGetVirtualBoundaryStatusPICO", (PFN_xrVoidFunction*)&xrGetVirtualBoundaryStatusPICO));
		XR_ENSURE(xrGetInstanceProcAddr(InInstance, "xrSetVirtualBoundaryEnablePICO", (PFN_xrVoidFunction*)&xrSetVirtualBoundaryEnablePICO));
		XR_ENSURE(xrGetInstanceProcAddr(InInstance, "xrSetVirtualBoundaryVisiblePICO", (PFN_xrVoidFunction*)&xrSetVirtualBoundaryVisiblePICO));
		XR_ENSURE(xrGetInstanceProcAddr(InInstance, "xrSetVirtualBoundarySeeThroughVisiblePICO", (PFN_xrVoidFunction*)&xrSetVirtualBoundarySeeThroughVisiblePICO));
		XR_ENSURE(xrGetInstanceProcAddr(InInstance, "xrGetVirtualBoundaryTriggerPICO", (PFN_xrVoidFunction*)&xrGetVirtualBoundaryTriggerPICO));
		XR_ENSURE(xrGetInstanceProcAddr(InInstance, "xrGetVirtualBoundaryGeometryPICO", (PFN_xrVoidFunction*)&xrGetVirtualBoundaryGeometryPICO));

		bSupportedVirtualBoundary = true;
	}

	bSupportMRCExtension = IOpenXRHMDModule::Get().IsExtensionEnabled(XR_PICO_EXTERNAL_CAMERA_EXTENSION_NAME);
	if (bSupportMRCExtension && !MRCDebugMode.EnableExtension)
	{
		XR_ENSURE(xrGetInstanceProcAddr(InInstance, "xrGetExternalCameraInfoPICO", (PFN_xrVoidFunction*)&xrGetExternalCameraInfoPICO));
	}

	bSupportedBDCompositionLayerSettingsExt = IOpenXRHMDModule::Get().IsExtensionEnabled(XR_PICO_LAYER_SETTINGS_EXTENSION_NAME);
	
	bSupportAdaptiveResolution = IOpenXRHMDModule::Get().IsExtensionEnabled(XR_PICO_ADAPTIVE_RESOLUTION_EXTENSION_NAME);
	if (bSupportAdaptiveResolution)
	{
		XR_ENSURE(xrGetInstanceProcAddr(InInstance, "xrUpdateAdaptiveResolutionPICO", (PFN_xrVoidFunction*)&xrUpdateAdaptiveResolutionPICO));
	}
	
	bSupportColorMatrixExtension = IOpenXRHMDModule::Get().IsExtensionEnabled(XR_PICO_LAYER_COLOR_MATRIX_EXTENSION_NAME);
}

const void* FHMDPICO::OnCreateSession(XrInstance InInstance, XrSystemId InSystem, const void* InNext)
{
	static FName SystemName(TEXT("OpenXR"));
	if (GEngine->XRSystem.IsValid() && (GEngine->XRSystem->GetSystemName() == SystemName))
	{
		OpenXRHMD = (FOpenXRHMD*)GEngine->XRSystem.Get();
	}

	UPICOOpenXRRuntimeSettings* Settings = GetMutableDefault<UPICOOpenXRRuntimeSettings>();
	if (Settings)
	{
		check(Settings != nullptr);
		if (Settings->DisableRHIThread)
		{
#if PLATFORM_ANDROID
			GPendingRHIThreadMode = ERHIThreadMode::None;
#endif //PLATFORM_ANDROID
		}
		
		EnableContentProtect(Settings->bContentProtectEXT);
	}

	return InNext;
}

float ConvertDisplayRefreshRate(EDisplayRefreshRatePICO Rate)
{
	switch (Rate)
	{
	case EDisplayRefreshRatePICO::Default:
		return 0.0f;
		break;
	case EDisplayRefreshRatePICO::Rate72:
		return 72.0f;
		break;
	case EDisplayRefreshRatePICO::Rate90:
		return 90.0f;
		break;
	case EDisplayRefreshRatePICO::Rate120:
		return 120.0f;
		break;
	}
	return 0.0f;
}

void FHMDPICO::PostCreateSession(XrSession InSession)
{
	FReadScopeLock Lock(SessionHandleMutex);
	Session = InSession;
	UPICOOpenXRRuntimeSettings* Settings = GetMutableDefault<UPICOOpenXRRuntimeSettings>();

	if (Settings && Settings->bDynamicResolution && bSupportAdaptiveResolution)
	{
		bDynamicResolution = true;
		if (IConsoleVariable* MobileDynamicResCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("xr.MobileLDRDynamicResolution")))
		{
			MobileDynamicResCVar->Set(1);
		}

		if (IConsoleVariable* DynamicResOperationCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DynamicRes.OperationMode")))
		{
			DynamicResOperationCVar->Set(2);
		}

		MinimumResolutionScale = Settings->MinimumDynamicResolutionScale;
		CurrentAdaptiveResolutionSetting = Settings->AdaptiveResolutionSetting;
		GEngine->ChangeDynamicResolutionStateAtNextFrame(MakeShareable(new FDynamicResolutionStatePICO(this, MinimumResolutionScale)));
	}

	if (bSupportDisplayRefreshRate)
	{
		uint32_t DisplayRefreshRateCountOutput = 0;
		XR_ENSURE(xrEnumerateDisplayRefreshRatesFB(InSession, 0, &DisplayRefreshRateCountOutput, nullptr));
		if (DisplayRefreshRateCountOutput > 0)
		{
			SupportedDisplayRefreshRates.SetNum(DisplayRefreshRateCountOutput);
			XR_ENSURE(xrEnumerateDisplayRefreshRatesFB(InSession, SupportedDisplayRefreshRates.Num(), &DisplayRefreshRateCountOutput, SupportedDisplayRefreshRates.GetData()));
			for (int i = 0; i < SupportedDisplayRefreshRates.Num(); i++)
			{
				UE_LOG(LogPICOOpenXRHMD, Log, TEXT("Supported DisplayRefreshRates[%d]:%f"), i, SupportedDisplayRefreshRates[i]);
			}
		}

		XR_ENSURE(xrGetDisplayRefreshRateFB(InSession, &CurrentDisplayRefreshRate));
		UE_LOG(LogPICOOpenXRHMD, Log, TEXT("Get current default DisplayRefreshRate:%f"), CurrentDisplayRefreshRate);

		if (Settings)
		{
			float RequestRate = ConvertDisplayRefreshRate(Settings->DisplayRefreshRate);
			SetDisplayRefreshRate(RequestRate);
		}
	}

	if (ViewTrackingSpace == XR_NULL_HANDLE)
	{
		XrReferenceSpaceCreateInfo SpaceInfo;
		SpaceInfo.type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO;
		SpaceInfo.next = nullptr;
		SpaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
		SpaceInfo.poseInReferenceSpace = ToXrPose(FTransform::Identity);
		XR_ENSURE(xrCreateReferenceSpace(InSession, &SpaceInfo, &ViewTrackingSpace));
	}

	if (bSupportMRCExtension && MRCSpace == XR_NULL_HANDLE)
	{
		XrMrcSpaceCreateInfoPICO MRCSpaceCreateInfoBd = { XR_TYPE_MRC_SPACE_CREATE_INFO_PICO };
		XrReferenceSpaceCreateInfo SpaceInfo = {};
		SpaceInfo.type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO;
		SpaceInfo.next = &MRCSpaceCreateInfoBd;
		SpaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
		SpaceInfo.poseInReferenceSpace = ToXrPose(FTransform::Identity);
		XR_ENSURE(xrCreateReferenceSpace(InSession, &SpaceInfo, &MRCSpace));
	}

	if (IConsoleVariable* EnableVariableRateShadingCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.VRS.Enable")))
	{
		EnableVariableRateShadingCVar->Set(1);
	}

	if (IConsoleVariable* OpenXRFBFoveationDynamicCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("xr.OpenXRFBFoveationDynamic")))
	{
		OpenXRFBFoveationDynamicCVar->Set(true);
	}
}

void FHMDPICO::OnDestroySession(XrSession InSession)
{
	if (ViewTrackingSpace)
	{
		XR_ENSURE(xrDestroySpace(ViewTrackingSpace));
	}
	ViewTrackingSpace = XR_NULL_HANDLE;


	if (MRCSpace)
	{
		XR_ENSURE(xrDestroySpace(MRCSpace));
	}
	MRCSpace = XR_NULL_HANDLE;
}

bool FHMDPICO::UseCustomReferenceSpaceType(XrReferenceSpaceType& OutReferenceSpaceType)
{
	if (bSupportLocalFloorLevelEXT)
	{
		UPICOOpenXRRuntimeSettings* Settings = GetMutableDefault<UPICOOpenXRRuntimeSettings>();
		if (Settings && Settings->bLocalFloorLevelEXT)
		{
			OutReferenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL_FLOOR_EXT;
			return true;
		}
	}
	return false;
}

bool FHMDPICO::GetSpectatorScreenController(FHeadMountedDisplayBase* InHMDBase, TUniquePtr<FDefaultSpectatorScreenController>& OutSpectatorScreenController)
{
#if PLATFORM_ANDROID
	OutSpectatorScreenController = nullptr;
	return true;
#else // PLATFORM_ANDROID
	OutSpectatorScreenController = MakeUnique<FDefaultSpectatorScreenController>(InHMDBase);
	return false;
#endif // PLATFORM_ANDROID
}

void FHMDPICO::OnEvent(XrSession InSession, const XrEventDataBaseHeader* InHeader)
{
	const XrEventDataBuffer* EventDataBuffer = reinterpret_cast<const XrEventDataBuffer*>(InHeader);

	if (EventDataBuffer == nullptr)
	{
		return;
	}

	switch (EventDataBuffer->type)
	{
	case XR_TYPE_EVENT_DATA_DISPLAY_REFRESH_RATE_CHANGED_FB:
		if (bSupportDisplayRefreshRate)
		{
			const XrEventDataDisplayRefreshRateChangedFB* DisplayRefreshRate = reinterpret_cast<const XrEventDataDisplayRefreshRateChangedFB*>(EventDataBuffer);
			CurrentDisplayRefreshRate = DisplayRefreshRate->toDisplayRefreshRate;
			UHMDFunctionLibraryPICO::GetDelegateManagerPICO()->OnDeviceDisplayRefreshRateChanged.Broadcast(DisplayRefreshRate->fromDisplayRefreshRate, DisplayRefreshRate->toDisplayRefreshRate);
			UE_LOG(LogPICOOpenXRHMD, Log, TEXT("DisplayRefreshRate changed from %f to %f."), DisplayRefreshRate->fromDisplayRefreshRate, DisplayRefreshRate->toDisplayRefreshRate);
		}
		break;
	case XR_TYPE_EVENT_DATA_PERF_SETTINGS_EXT:
		if (bSupportPerformanceSettingsEXT)
		{
			const XrEventDataPerfSettingsEXT* PerfSettings = reinterpret_cast<const XrEventDataPerfSettingsEXT*>(EventDataBuffer);
			UHMDFunctionLibraryPICO::GetDelegateManagerPICO()->OnDevicePerformanceSettingsChanged.Broadcast(EPerfSettingsDomainPICO(PerfSettings->domain)
				, EPerfSettingsSubDomainPICO(PerfSettings->subDomain)
				, EPerfSettingsNotificationLevelPICO(PerfSettings->toLevel)
				, EPerfSettingsNotificationLevelPICO(PerfSettings->fromLevel));
			UE_LOG(LogPICOOpenXRHMD, Log, TEXT("PerformanceSettings level changed from %d to %d (domain:%d subdomain:%d"), PerfSettings->fromLevel, PerfSettings->toLevel, PerfSettings->domain, PerfSettings->subDomain);
		}
		break;
	case XR_TYPE_EVENT_DATA_USER_PRESENCE_CHANGED_EXT:
		if (IsSupportsUserPresence)
		{
			const XrEventDataUserPresenceChangedEXT* UserPresenceChanged = reinterpret_cast<const XrEventDataUserPresenceChangedEXT*>(EventDataBuffer);
			if (UserPresenceChanged->isUserPresent)
			{
				WornState = EHMDWornState::Type::Worn;
				FCoreDelegates::VRHeadsetPutOnHead.Broadcast();
			}
			else
			{
				WornState = EHMDWornState::Type::NotWorn;
				FCoreDelegates::VRHeadsetRemovedFromHead.Broadcast();
			}
		}
		break;
	case XR_TYPE_EVENT_DATA_MRC_STATUS_CHANGED_PICO:
		if (bSupportMRCExtension)
		{
			const XrEventDataMrcStatusChangedPICO* MRCStatusChanged = reinterpret_cast<const XrEventDataMrcStatusChangedPICO*>(EventDataBuffer);
			bIsMRCRunningStored = bIsMRCRunning = MRCStatusChanged->mrcStatus == 1;
			UHMDFunctionLibraryPICO::GetDelegateManagerPICO()->OnMRCStatusChanged.Broadcast(bIsMRCRunning);
		}
		break;
	default:
		break;
	}
}

const void* FHMDPICO::OnEndProjectionLayer_RHIThread(XrSession InSession, int32 InLayerIndex, const void* InNext, XrCompositionLayerFlags& OutFlags)
{
	if (bSupportLayerDepth)
	{
		OutFlags |= XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
	}

	if (bContentProtectEnabled)
	{
		ContentProtect = { XR_TYPE_COMPOSITION_LAYER_SECURE_CONTENT_FB };
		ContentProtect.next = const_cast<void*>(InNext);
		ContentProtect.flags = XR_COMPOSITION_LAYER_SECURE_CONTENT_REPLACE_LAYER_BIT_FB;
		InNext = &ContentProtect;
	}

	if (bSupportedBDCompositionLayerSettingsExt)
	{
		ProjectionLayerSettings = { XR_TYPE_LAYER_SETTINGS_PICO };

		static const auto CVarPICOSuperResolution = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.Mobile.PICO.EnableSuperResolution"));
		if (CVarPICOSuperResolution->GetValueOnAnyThread() == 1)
		{
			ProjectionLayerSettings.layerFlags |= XR_LAYER_SETTINGS_SUPER_RESOLUTION_BIT_PICO;
		}

		bool bSharpening = false;
		static const auto CVarPICOSharpening = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.Mobile.PICO.SharpeningSetting"));
		const ESharpeningTypePICO SharpeningType = static_cast<ESharpeningTypePICO>(CVarPICOSharpening->GetValueOnAnyThread());
		switch (SharpeningType)
		{
		case ESharpeningTypePICO::NormalSharpening:
		{
			ProjectionLayerSettings.layerFlags |= XR_COMPOSITION_LAYER_SETTINGS_NORMAL_SHARPENING_BIT_FB;
			bSharpening = true;
		}
		break;
		case ESharpeningTypePICO::QualitySharpening:
		{
			ProjectionLayerSettings.layerFlags |= XR_COMPOSITION_LAYER_SETTINGS_QUALITY_SHARPENING_BIT_FB;
			bSharpening = true;
		}
		break;
		default:;
		}

		if (bSharpening)
		{
			static const auto CVarPICOSharpeningEnhance = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.Mobile.PICO.SharpeningEnhanceMode"));
			const ESharpeningEnhanceModePICO EnhanceMode = static_cast<ESharpeningEnhanceModePICO>(CVarPICOSharpeningEnhance->GetValueOnAnyThread());
			switch (EnhanceMode)
			{
			case ESharpeningEnhanceModePICO::FixedFoveated:
			{
				ProjectionLayerSettings.layerFlags |= XR_LAYER_SETTINGS_FIXED_FOVEATED_NORMAL_SHARPENING_BIT_PICO;
			}
			break;
			case ESharpeningEnhanceModePICO::Adaptive:
			{
				ProjectionLayerSettings.layerFlags |= XR_LAYER_SETTINGS_SELF_ADAPTIVE_NORMAL_SHARPENING_BIT_PICO;
			}
			break;
			case ESharpeningEnhanceModePICO::Both:
			{
				ProjectionLayerSettings.layerFlags |= XR_LAYER_SETTINGS_FIXED_FOVEATED_NORMAL_SHARPENING_BIT_PICO;
				ProjectionLayerSettings.layerFlags |= XR_LAYER_SETTINGS_SELF_ADAPTIVE_NORMAL_SHARPENING_BIT_PICO;
			}
			break;
			default:;
			}
		}

		ProjectionLayerSettings.next = const_cast<void*>(InNext);
		InNext = &ProjectionLayerSettings;
	}

	if (bSupportColorMatrixExtension && bUseColorMatrixExtension)
	{
		XrLayerColorMatrixPICO LayerColorMatrix = { XR_TYPE_LAYER_COLOR_MATRIX_PICO };
		FMemory::Memcpy(LayerColorMatrix.matrix.m, ColorMatrix3x3f, 9);
		LayerColorMatrix.next = const_cast<void*>(InNext);
		InNext = &LayerColorMatrix;
	}

	return InNext;
}

void FHMDPICO::UpdateDeviceLocations(XrSession InSession, XrTime DisplayTime, XrSpace TrackingSpace)
{
	CurrentDisplayTime = DisplayTime;
	CurrentBaseSpace = TrackingSpace;
}

void FHMDPICO::OnBeginRendering_GameThread(XrSession InSession, FSceneViewFamily& InViewFamily, TArrayView<const uint32> VisibleLayers)
{
	if (bDynamicResolution)
	{
		float NewPixelDensity = 1.0;
		float PixelDensity = GetAdaptivePixelDensity(CurrentAdaptiveResolutionSetting, NewPixelDensity) ? NewPixelDensity : 1.0f;
		
		static const auto CVarPICODynamicPixelDensity = IConsoleManager::Get().FindTConsoleVariableDataFloat(TEXT("r.PICO.DynamicResolution.PixelDensity"));
		const float PixelDensityCVarOverride = CVarPICODynamicPixelDensity != nullptr ? CVarPICODynamicPixelDensity->GetValueOnAnyThread() : 0.0f;
		if (PixelDensityCVarOverride > 0.0f)
		{
			PixelDensity = PixelDensityCVarOverride;
		}

		CurrentDynamicPixelDensity = PixelDensity;
	}

	ENQUEUE_RENDER_COMMAND(UpdateMRCState)(
		[this, IsMRCForegroundLayerDisabled = bIsMRCForegroundLayerDisabled](FRHICommandListImmediate&)
		{
			bIsMRCForegroundLayerDisabled_RebderThread = IsMRCForegroundLayerDisabled;
		});
}

FIntRect FHMDPICO::GetViewportSize(const FOpenXRLayer::FPerEyeTextureData& EyeData, const IStereoLayers::FLayerDesc& Desc)
{
	FBox2D Viewport(EyeData.SwapchainSize * Desc.UVRect.Min, EyeData.SwapchainSize * Desc.UVRect.Max);
	return FIntRect(Viewport.Min.IntPoint(), Viewport.Max.IntPoint());
}

FVector2D FHMDPICO::GetQuadSize(const FOpenXRLayer::FPerEyeTextureData& EyeData, const IStereoLayers::FLayerDesc& Desc)
{
	if (Desc.Flags & IStereoLayers::LAYER_FLAG_QUAD_PRESERVE_TEX_RATIO)
	{
		float AspectRatio = EyeData.SwapchainSize.Y / EyeData.SwapchainSize.X;
		return FVector2D(Desc.QuadSize.X, Desc.QuadSize.X * AspectRatio);
	}
	return Desc.QuadSize;
}

void FHMDPICO::OnWorldTickStart(UWorld* InWorld, ELevelTick TickType, float DeltaTime)
{
	if (bSupportMRCExtension && IsInGameThread())
	{
		if (bIsMRCRunning && !MRCSceneCapture2DPICO)
		{
			FWorldContext& Context = GEngine->GetWorldContextFromWorldChecked(InWorld);
			if (((Context.WorldType == EWorldType::PIE) || (Context.WorldType == EWorldType::Game)) && (Context.World() != NULL))
			{
				FActorSpawnParameters SpawnInfo;
				SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				SpawnInfo.bNoFail = true;
				SpawnInfo.ObjectFlags = RF_Transient;
				MRCSceneCapture2DPICO = InWorld->SpawnActor<AMRCCameraPICO>(SpawnInfo);
				MRCSceneCapture2DPICO->SetActorEnableCollision(false);
			}
		}
		else if (!bIsMRCRunning && MRCSceneCapture2DPICO)
		{
			MRCSceneCapture2DPICO->Destroy();
			MRCSceneCapture2DPICO = nullptr;
		}
	}
}

void FHMDPICO::OnBeginRendering_RenderThread(XrSession InSession, FRDGBuilder& GraphBuilder)
{
	if (OpenXRHMD == nullptr || InSession == XR_NULL_HANDLE)
	{
		return;
	}

	FXRRenderBridge* RenderBridge = OpenXRHMD->GetActiveRenderBridge_GameThread(OpenXRHMD->ShouldUseSeparateRenderTarget());
	FOpenXRRenderBridge* Bridge = static_cast<FOpenXRRenderBridge*>(RenderBridge);
	uint8 UnusedActualFormat = 0;
	ETextureCreateFlags Flags = TexCreate_Dynamic | TexCreate_SRGB | TexCreate_ShaderResource | TexCreate_RenderTargetable;
	FRHICommandListImmediate& RHICmdList = FRHICommandListExecutor::GetImmediateCommandList();

	if (bSupportMRCExtension && bIsMRCRunning && !MRCLayer.IsValid())
	{
		if (Bridge && MRCLayerDesc_RenderThread.TextureObj.IsValid() && MRCLayerDesc_RenderThread.LeftTextureObj.IsValid())
		{
			MRCLayer = MakeShareable(new FOpenXRLayer(MRCLayerDesc_RenderThread));

			{
				FRHITexture* Texture = MRCLayer->Desc.TextureObj->GetResource()->TextureRHI;
				FXRSwapChainPtr SwapChain = Bridge->CreateSwapchain(InSession,
					IStereoRenderTargetManager::GetStereoLayerPixelFormat(),
					UnusedActualFormat,
					Texture->GetSizeX(),
					Texture->GetSizeY(),
#ifdef PICO_CUSTOM_ENGINE
					1,
#endif
					1,
					Texture->GetNumMips(),
					Texture->GetNumSamples(),
					Texture->GetFlags() | Flags,
					Texture->GetClearBinding());
				MRCLayer->RightEye.Swapchain = SwapChain;
				MRCLayer->RightEye.SwapchainSize = Texture->GetSizeXY();
				MRCLayer->RightEye.bStaticSwapchain = false;
				MRCLayer->RightEye.bUpdateTexture = true;
				MRCLayer->RightEye.Texture = Texture;
			}

			{
				FRHITexture* Texture = MRCLayer->Desc.LeftTextureObj->GetResource()->TextureRHI;
				FXRSwapChainPtr SwapChain = Bridge->CreateSwapchain(InSession,
					IStereoRenderTargetManager::GetStereoLayerPixelFormat(),
					UnusedActualFormat,
					Texture->GetSizeX(),
					Texture->GetSizeY(),
#ifdef PICO_CUSTOM_ENGINE
					1,
#endif
					1,
					Texture->GetNumMips(),
					Texture->GetNumSamples(),
					Texture->GetFlags() | Flags,
					Texture->GetClearBinding());
				MRCLayer->LeftEye.Swapchain = SwapChain;
				MRCLayer->LeftEye.SwapchainSize = Texture->GetSizeXY();
				MRCLayer->LeftEye.bStaticSwapchain = false;
				MRCLayer->LeftEye.bUpdateTexture = true;
				MRCLayer->LeftEye.Texture = Texture;
			}

			const bool bNoAlpha = MRCLayer->Desc.Flags & IStereoLayers::LAYER_FLAG_TEX_NO_ALPHA_CHANNEL;
			const bool bIsStereo = MRCLayer->Desc.LeftTextureObj.IsValid();
			const FTransform PositionTransform = FTransform::Identity;
			float WorldToMeters = OpenXRHMD->GetWorldToMetersScale();

			XrCompositionLayerQuad MRCQuadLayer = { XR_TYPE_COMPOSITION_LAYER_QUAD, nullptr };
			MRCQuadLayer.layerFlags = bNoAlpha ? 0 : XR_COMPOSITION_LAYER_UNPREMULTIPLIED_ALPHA_BIT | XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
			MRCQuadLayer.layerFlags |= XR_COMPOSITION_LAYER_MRC_COMPOSITION_BIT_PICO;
			MRCQuadLayer.space = ViewTrackingSpace;
			MRCQuadLayer.subImage.imageArrayIndex = 0;
			MRCQuadLayer.pose = ToXrPose(MRCLayer->Desc.Transform * PositionTransform, WorldToMeters);

			const FVector2D LayerComponentScaler(MRCLayer->Desc.Transform.GetScale3D().Y, MRCLayer->Desc.Transform.GetScale3D().Z);

			if (MRCLayer->RightEye.Swapchain.IsValid())
			{
				MRCQuadLayer.eyeVisibility = bIsStereo ? XR_EYE_VISIBILITY_RIGHT : XR_EYE_VISIBILITY_BOTH;

				MRCQuadLayer.subImage.imageRect = ToXrRect(GetViewportSize(MRCLayer->RightEye, MRCLayer->Desc));
				MRCQuadLayer.subImage.swapchain = static_cast<FOpenXRSwapchain*>(MRCLayer->RightEye.Swapchain.Get())->GetHandle();
				MRCQuadLayer.size = ToXrExtent2D(GetQuadSize(MRCLayer->RightEye, MRCLayer->Desc) * LayerComponentScaler, WorldToMeters);
				MRCQuadLayerRight_RenderThread = MRCQuadLayer;
			}

			if (MRCLayer->LeftEye.Swapchain.IsValid())
			{
				MRCQuadLayer.eyeVisibility = XR_EYE_VISIBILITY_LEFT;
				MRCQuadLayer.subImage.imageRect = ToXrRect(GetViewportSize(MRCLayer->LeftEye, MRCLayer->Desc));
				MRCQuadLayer.subImage.swapchain = static_cast<FOpenXRSwapchain*>(MRCLayer->LeftEye.Swapchain.Get())->GetHandle();
				MRCQuadLayer.size = ToXrExtent2D(GetQuadSize(MRCLayer->LeftEye, MRCLayer->Desc) * LayerComponentScaler, WorldToMeters);
				MRCQuadLayerLeft_RenderThread = MRCQuadLayer;
			}
		}
		else
		{
			MRCLayer.Reset();
		}
	}
	else if(MRCLayer.IsValid() && (!MRCLayerDesc_RenderThread.TextureObj.IsValid() || !MRCLayerDesc_RenderThread.LeftTextureObj.IsValid()))
	{
		MRCLayer.Reset();
	}

	if (MRCLayer.IsValid())
	{
		static const int64 OPENXR_SWAPCHAIN_WAIT_TIMEOUT = 100000000ll;        // 100ms in nanoseconds.
		bool bInvertX = MRCDebugMode.ViewInHMD ? true : false;
		bool bInvertY = MRCDebugMode.ViewInHMD ? false : true;

		if (MRCLayer->RightEye.Swapchain.IsValid())
		{
			FRHITexture* const RightDstTexture = MRCLayer->RightEye.Swapchain->GetTexture2DArray() ? MRCLayer->RightEye.Swapchain->GetTexture2DArray() : MRCLayer->RightEye.Swapchain->GetTexture2D();

			//Right SwapChain
			const FXRSwapChainPtr& RightDstSwapChain = MRCLayer->RightEye.Swapchain;
			RHICmdList.EnqueueLambda([RightDstSwapChain](FRHICommandListImmediate& InRHICmdList)
				{
					RightDstSwapChain->IncrementSwapChainIndex_RHIThread();
					RightDstSwapChain->WaitCurrentImage_RHIThread(OPENXR_SWAPCHAIN_WAIT_TIMEOUT);
				});

			if (bIsMRCForegroundLayerDisabled_RebderThread)
			{
				FRHIRenderPassInfo RenderPassInfo(RightDstTexture, ERenderTargetActions::DontLoad_Store);
				TransitionRenderPassTargets(RHICmdList, RenderPassInfo);
				RHICmdList.BeginRenderPass(RenderPassInfo, TEXT("ClearRT"));
				DrawClearQuad(RHICmdList, FLinearColor::Green);
				RHICmdList.EndRenderPass();
			}
			else
			{
				FRHITexture* RightSrcTexture = MRCLayer->Desc.TextureObj->GetResource()->TextureRHI;
				const FIntRect RightDstRect(FIntPoint(0, 0), MRCLayer->RightEye.SwapchainSize.IntPoint());
				CopyTexture_RenderThread(RHICmdList, RightDstTexture, RightSrcTexture, RightDstRect, FIntRect(), true, true, true, bInvertX, bInvertY);
			}

			RHICmdList.EnqueueLambda([RightDstSwapChain](FRHICommandListImmediate& InRHICmdList)
				{
					RightDstSwapChain->ReleaseCurrentImage_RHIThread(nullptr);
				});
		}

		if (MRCLayer->LeftEye.Swapchain.IsValid())
		{
			//Left SwapChain
			const FXRSwapChainPtr& LeftDstSwapChain = MRCLayer->LeftEye.Swapchain;
			RHICmdList.EnqueueLambda([LeftDstSwapChain](FRHICommandListImmediate& InRHICmdList)
				{
					LeftDstSwapChain->IncrementSwapChainIndex_RHIThread();
					LeftDstSwapChain->WaitCurrentImage_RHIThread(OPENXR_SWAPCHAIN_WAIT_TIMEOUT);
				});

			FRHITexture* LeftSrcTexture = MRCLayer->Desc.LeftTextureObj->GetResource()->TextureRHI;
			const FIntRect LeftDstRect(FIntPoint(0, 0), MRCLayer->LeftEye.SwapchainSize.IntPoint());
			FRHITexture* const LeftDstTexture = MRCLayer->LeftEye.Swapchain->GetTexture2DArray() ? MRCLayer->LeftEye.Swapchain->GetTexture2DArray() : MRCLayer->LeftEye.Swapchain->GetTexture2D();
			CopyTexture_RenderThread(RHICmdList, LeftDstTexture, LeftSrcTexture, LeftDstRect, FIntRect(), true, true, true/*BG*/, bInvertX, bInvertY);

			RHICmdList.EnqueueLambda([LeftDstSwapChain](FRHICommandListImmediate& InRHICmdList)
				{
					LeftDstSwapChain->ReleaseCurrentImage_RHIThread(nullptr);
				});
		}

		if (MRCDebugMode.ViewInHMD)
		{
			MRCQuadLayerLeft_RenderThread.layerFlags &= ~XR_COMPOSITION_LAYER_MRC_COMPOSITION_BIT_PICO;
			MRCQuadLayerRight_RenderThread.layerFlags &= ~XR_COMPOSITION_LAYER_MRC_COMPOSITION_BIT_PICO;
		}
		else
		{
			MRCQuadLayerLeft_RenderThread.layerFlags |= XR_COMPOSITION_LAYER_MRC_COMPOSITION_BIT_PICO;
			MRCQuadLayerRight_RenderThread.layerFlags |= XR_COMPOSITION_LAYER_MRC_COMPOSITION_BIT_PICO;
		}

		GraphBuilder.RHICmdList.EnqueueLambda([this, MRCLayerLeft_RenderThread = MRCQuadLayerLeft_RenderThread, MRCLayerRight_RenderThread = MRCQuadLayerRight_RenderThread](FRHICommandListImmediate&)
			{
				MRCQuadLayerLeft_RHIThread = MRCLayerLeft_RenderThread;
				MRCQuadLayerRight_RHIThread = MRCLayerRight_RenderThread;
			});
	}
}

void FHMDPICO::UpdateCompositionLayers_RHIThread(XrSession InSession, TArray<XrCompositionLayerBaseHeader*>& Headers)
{
	if (bSupportLayerDepth)
	{
#ifndef PICO_CUSTOM_ENGINE
		//Stereo Layer default support depth
		for (int32 i = 0; i < Headers.Num(); i++)
		{
			if (OpenXRHMD && OpenXRHMD->GetTrackedDeviceSpace(IXRTrackingSystem::HMDDeviceId) == Headers[i]->space)
			{
				continue;//skip face-lock layer
			}

			if (Headers[i]->type != XR_TYPE_COMPOSITION_LAYER_PROJECTION && Headers[i]->type != XR_TYPE_COMPOSITION_LAYER_PASSTHROUGH_FB)
			{
				BlendState.next = const_cast<void*>(Headers[i]->next);
				Headers[i]->next = &BlendState;
			}
		}
#endif //  PICO_CUSTOM_ENGINE
	}

	if (bSupportMRCExtension && MRCLayer.IsValid())
	{
		Headers.Add(reinterpret_cast<XrCompositionLayerBaseHeader*>(&MRCQuadLayerLeft_RHIThread));
		Headers.Add(reinterpret_cast<XrCompositionLayerBaseHeader*>(&MRCQuadLayerRight_RHIThread));
	}
}

bool FHMDPICO::GetSupportedDisplayRefreshRates(TArray<float>& Rates)
{
	Rates = SupportedDisplayRefreshRates;
	return bSupportDisplayRefreshRate;
}

bool FHMDPICO::GetCurrentDisplayRefreshRate(float& Rate, bool DoubleCheck)
{
	if (bSupportDisplayRefreshRate)
	{
		if (DoubleCheck && Session)
		{
			float DisplayRefreshRate = 0;
			if (XR_SUCCEEDED(xrGetDisplayRefreshRateFB(Session, &DisplayRefreshRate)))
			{
				if (DisplayRefreshRate == CurrentDisplayRefreshRate) // Check XR_TYPE_EVENT_DATA_DISPLAY_REFRESH_RATE_CHANGED_FB
				{
					Rate = DisplayRefreshRate;
					return true;
				}
			}
		}
		else
		{
			Rate = CurrentDisplayRefreshRate;
			return true;
		}
	}
	return false;
}

bool FHMDPICO::SetDisplayRefreshRate(float Rate)
{
	FReadScopeLock Lock(SessionHandleMutex);
	if (bSupportDisplayRefreshRate && Session)
	{
		if (SupportedDisplayRefreshRates.Contains(Rate))
		{
			UE_LOG(LogPICOOpenXRHMD, Log, TEXT("Requesting DisplayRefreshRate:%f"), Rate);
			XR_ENSURE(xrRequestDisplayRefreshRateFB(Session, Rate));
			return true;
		}
		else
		{
			UE_LOG(LogPICOOpenXRHMD, Warning, TEXT("Requested DisplayRefreshRate:%f is not supported by device!"), Rate);
		}
	}
	return false;
}

void FHMDPICO::EnableContentProtect(bool Enable)
{
	bool ContentProtectSupportedEXT = IOpenXRHMDModule::Get().IsExtensionEnabled(XR_FB_COMPOSITION_LAYER_SECURE_CONTENT_EXTENSION_NAME);
	bContentProtectEnabled = Enable && ContentProtectSupportedEXT;
	UE_LOG(LogPICOOpenXRHMD, Log, TEXT(":EnableContentProtect EXT Supported:%d, Enabled:%d"), ContentProtectSupportedEXT, bContentProtectEnabled);
}

bool FHMDPICO::SetPerformanceLevel(int domain, int level)
{
	if (Session && bSupportPerformanceSettingsEXT)
	{
		return XR_SUCCEEDED(xrPerfSettingsSetPerformanceLevelEXT(Session, XrPerfSettingsDomainEXT(domain), XrPerfSettingsLevelEXT(level)));
	}
	return false;
}

bool FHMDPICO::GetDevicePoseForTime(const EControllerHand Hand, bool UseDefaultTime, FTimespan Timespan, bool& OutTimeWasUsed, FRotator& OutOrientation, FVector& OutPosition, bool& OutbProvidedLinearVelocity, FVector& OutLinearVelocity, bool& OutbProvidedAngularVelocity, FVector& OutAngularVelocityRadPerSec, bool& OutbProvidedLinearAcceleration, FVector& OutLinearAcceleration, float InWorldToMetersScale)
{
	if (OpenXRHMD == nullptr)
	{
		return false;
	}

	int32 DeviceId = -1;
	if (Hand == EControllerHand::HMD)
	{
		DeviceId = IXRTrackingSystem::HMDDeviceId;
	}
	else
	{
		TArray<int32> Devices;
		if (OpenXRHMD->EnumerateTrackedDevices(Devices, EXRTrackedDeviceType::Controller))
		{
			if (Devices.IsValidIndex((int32)Hand))
			{
				DeviceId = Devices[(int32)Hand];
			}
		}
	}

	if (DeviceId == -1)
	{
		return false;
	}

	XrTime TargetTime = 0;
	if (UseDefaultTime)
	{
		TargetTime = CurrentDisplayTime;
	}
	else
	{
		TargetTime = ToXrTime(Timespan);
	}

	if (TargetTime == 0)
	{
		OutTimeWasUsed = false;
	}
	else
	{
		OutTimeWasUsed = true;
	}

	FQuat Orientation;
	bool Success = OpenXRHMD->GetPoseForTime(DeviceId, TargetTime, OutTimeWasUsed, Orientation, OutPosition, OutbProvidedLinearVelocity, OutLinearVelocity, OutbProvidedAngularVelocity, OutAngularVelocityRadPerSec, OutbProvidedLinearAcceleration, OutLinearAcceleration, InWorldToMetersScale);
	OutOrientation = FRotator(Orientation);
	return Success;
}

EHMDWornState::Type FHMDPICO::GetHMDWornState(bool& ResultValid)
{
	ResultValid = IsSupportsUserPresence;
	return WornState;
}

bool FHMDPICO::GetFieldOfView(float& OutHFOVInDegrees, float& OutVFOVInDegrees)
{
	if (OpenXRHMD)
	{
		OpenXRHMD->GetHMDDevice()->GetFieldOfView(OutHFOVInDegrees, OutVFOVInDegrees);
		return true;
	}
	return false;
}

bool FHMDPICO::GetInterpupillaryDistance(float& IPD)
{
	if (OpenXRHMD)
	{
		IPD = OpenXRHMD->GetInterpupillaryDistance() * OpenXRHMD->GetWorldToMetersScale();
		return true;
	}
	return false;
}

void FHMDPICO::SetBaseRotationAndBaseOffset(FRotator Rotation, FVector BaseOffset, EOrientPositionSelector::Type Options)
{
	if (OpenXRHMD)
	{
		if ((Options == EOrientPositionSelector::Orientation) || (Options == EOrientPositionSelector::OrientationAndPosition))
		{
			OpenXRHMD->SetBaseRotation(Rotation);
		}
		if ((Options == EOrientPositionSelector::Position) || (Options == EOrientPositionSelector::OrientationAndPosition))
		{
			OpenXRHMD->SetBasePosition(BaseOffset);
		}
	}
}

void FHMDPICO::GetBaseRotationAndBaseOffset(FRotator& OutRotation, FVector& OutBaseOffset)
{
	if (OpenXRHMD != nullptr)
	{
		OutRotation = OpenXRHMD->GetBaseRotation();
		OutBaseOffset = OpenXRHMD->GetBasePosition();
	}
	else
	{
		OutRotation = FRotator::ZeroRotator;
		OutBaseOffset = FVector::ZeroVector;
	}
}

FTimespan FHMDPICO::GetDisplayTime()
{
	return ToFTimespan(CurrentDisplayTime);
}

bool FHMDPICO::IsStationaryBoundaryMode(bool& bIsStationary)
{
	if (Session && bSupportedVirtualBoundary)
	{
		XrVirtualBoundaryModePICO Mode;
		if (XR_SUCCEEDED(xrGetVirtualBoundaryModePICO(Session, &Mode)))
		{
			bIsStationary = Mode == XrVirtualBoundaryModePICO::XR_VIRTUAL_BOUNDARY_MODE_STATIONARY_PICO;
			return true;
		}
	}
	return false;
}

bool FHMDPICO::GetVirtualBoundaryStatus(bool& bIsReady, bool& bIsEnable, bool& bIsVisible)
{
	if (Session && bSupportedVirtualBoundary)
	{
		XrVirtualBoundaryStatusPICO Status = { XR_TYPE_VIRTUAL_BOUNDARY_STATUS_PICO };
		if (XR_SUCCEEDED(xrGetVirtualBoundaryStatusPICO(Session, &Status)))
		{
			bIsReady = Status.isReady == XR_TRUE;
			bIsEnable = Status.isEnabled == XR_TRUE;
			bIsVisible = Status.isVisible == XR_TRUE;
			return true;
		}
	}
	return false;
}

bool FHMDPICO::SetVirtualBoundaryEnable(bool bEnable)
{
	if (Session && bSupportedVirtualBoundary)
	{
		if (XR_SUCCEEDED(xrSetVirtualBoundaryEnablePICO(Session, bEnable)))
		{
			return true;
		}
	}
	return false;
}

bool FHMDPICO::SetVirtualBoundaryVisible(bool bVisible)
{
	if (Session && bSupportedVirtualBoundary)
	{
		if (XR_SUCCEEDED(xrSetVirtualBoundaryVisiblePICO(Session, bVisible)))
		{
			return true;
		}
	}
	return false;
}

bool FHMDPICO::SetVirtualBoundarySeeThroughVisible(bool bVisible)
{
	if (Session && bSupportedVirtualBoundary)
	{
		if (XR_SUCCEEDED(xrSetVirtualBoundarySeeThroughVisiblePICO(Session, bVisible)))
		{
			return true;
		}
	}
	return false;
}

bool FHMDPICO::BoundaryintersectPointOrNode(bool bPoint, EControllerHand Node, FVector Point, EBoundaryTypePICO BoundaryType, bool& Valid, bool& IsTriggering, float& ClosestDistance, FVector& ClosestPoint, FVector& ClosestPointNormal, float InWorldToMetersScale)
{
	if (Session && bSupportedVirtualBoundary)
	{
		XrVirtualBoundaryInfoPICO VirtualBoundaryInfo = { XR_TYPE_VIRTUAL_BOUNDARY_INFO_PICO };
		VirtualBoundaryInfo.baseSpace = CurrentBaseSpace;
		VirtualBoundaryInfo.edgeType = (XrVirtualBoundaryEdgeTypePICO)BoundaryType;

		XrVirtualBoundaryTriggerPointPICO BoundaryTestPoint = { XR_TYPE_VIRTUAL_BOUNDARY_TRIGGER_POINT_PICO };
		BoundaryTestPoint.point = ToXrVector(Point, InWorldToMetersScale);
		BoundaryTestPoint.boundaryInfo = &VirtualBoundaryInfo;

		XrVirtualBoundaryTriggerNodePICO VirtualBoundaryTriggerNode = { XR_TYPE_VIRTUAL_BOUNDARY_TRIGGER_NODE_PICO };
		VirtualBoundaryTriggerNode.boundaryInfo = &VirtualBoundaryInfo;

		switch (Node)
		{
		case EControllerHand::Left:
			VirtualBoundaryTriggerNode.node = XR_VIRTUAL_BOUNDARY_TRIGGER_NODE_TYPE_LEFT_HAND_PICO;
			break;
		case EControllerHand::Right:
			VirtualBoundaryTriggerNode.node = XR_VIRTUAL_BOUNDARY_TRIGGER_NODE_TYPE_RIGHT_HAND_PICO;
			break;
		case EControllerHand::HMD:
			VirtualBoundaryTriggerNode.node = XR_VIRTUAL_BOUNDARY_TRIGGER_NODE_TYPE_HEAD_PICO;
			break;
		default:
			break;
		}

		const XrVirtualBoundaryTriggerBaseHeaderPICO* BaseHeader = nullptr;
		if (bPoint)
		{	
			BaseHeader = reinterpret_cast<const XrVirtualBoundaryTriggerBaseHeaderPICO*>(&BoundaryTestPoint);
		}
		else
		{

			BaseHeader = reinterpret_cast<const XrVirtualBoundaryTriggerBaseHeaderPICO*>(&VirtualBoundaryTriggerNode);
		}

		XrVirtualBoundaryTriggerPICO VirtualBoundaryTrigger = { XR_TYPE_VIRTUAL_BOUNDARY_TRIGGER_PICO };
		if (XR_SUCCEEDED(xrGetVirtualBoundaryTriggerPICO(Session, BaseHeader, &VirtualBoundaryTrigger)))
		{
			Valid = VirtualBoundaryTrigger.isValid == XR_TRUE;
			ClosestDistance = VirtualBoundaryTrigger.closestDistance * InWorldToMetersScale;
			IsTriggering = VirtualBoundaryTrigger.isTriggering == XR_TRUE;
			ClosestPoint = ToFVector(VirtualBoundaryTrigger.closestPoint, InWorldToMetersScale);
			ClosestPointNormal = ToFVector(VirtualBoundaryTrigger.closestPointNormal);
			return true;
		}
	}
	return false;
}

bool FHMDPICO::GetBoundaryGeometry(EBoundaryTypePICO BoundaryType, bool& Valid, TArray<FVector>& Points, float InWorldToMetersScale)
{
	if (Session && bSupportedVirtualBoundary)
	{
		XrVirtualBoundaryInfoPICO VirtualBoundaryInfo = { XR_TYPE_VIRTUAL_BOUNDARY_INFO_PICO };
		VirtualBoundaryInfo.baseSpace = CurrentBaseSpace;
		VirtualBoundaryInfo.edgeType = (XrVirtualBoundaryEdgeTypePICO)BoundaryType;

		uint32 count = 0;
		xrGetVirtualBoundaryGeometryPICO(Session, &VirtualBoundaryInfo, 0, &count, nullptr);
		if (count > 0)
		{
			TArray<XrVector3f> xrPoints;
			Points.SetNum(count);
			xrPoints.SetNum(count);
			if (XR_SUCCEEDED(xrGetVirtualBoundaryGeometryPICO(Session, &VirtualBoundaryInfo, count, &count, xrPoints.GetData())))
			{
				for (int i = 0; i < xrPoints.Num(); i++)
				{
					Points[i] = ToFVector(xrPoints[i], InWorldToMetersScale);
				}
				return true;
			}
		}
	}
	return false;
}

void FHMDPICO::CopyTexture_RenderThread(FRHICommandListImmediate& RHICmdList, FRHITexture* DstTexture, FRHITexture* SrcTexture, FIntRect DstRect, FIntRect SrcRect, bool bAlphaPremultiply, bool bNoAlpha, bool bClearGreen, bool bInvertX, bool bInvertY, bool bInvertAlpha) const
{
	ERenderTargetActions RTAction = ERenderTargetActions::Clear_Store;
	ERHIAccess FinalDstAccess = ERHIAccess::SRVMask;

	check(IsInRenderingThread());

	const uint32 ViewportWidth = DstRect.Width();
	const uint32 ViewportHeight = DstRect.Height();
	const FIntPoint TargetSize(ViewportWidth, ViewportHeight);

	const float SrcTextureWidth = SrcTexture->GetSizeX();
	const float SrcTextureHeight = SrcTexture->GetSizeY();
	float U = 0.f, V = 0.f, USize = 1.f, VSize = 1.f;
	if (SrcRect.IsEmpty())
	{
		SrcRect.Min.X = 0;
		SrcRect.Min.Y = 0;
		SrcRect.Max.X = SrcTextureWidth;
		SrcRect.Max.Y = SrcTextureHeight;
	}
	else
	{
		U = SrcRect.Min.X / SrcTextureWidth;
		V = SrcRect.Min.Y / SrcTextureHeight;
		USize = SrcRect.Width() / SrcTextureWidth;
		VSize = SrcRect.Height() / SrcTextureHeight;
	}

	if (bInvertX)
	{
		U = 1.0f - U;
		USize = -USize;
	}

	if (bInvertY)
	{
		V = 1.0f - V;
		VSize = -VSize;
	}

	RHICmdList.Transition(FRHITransitionInfo(DstTexture, ERHIAccess::Unknown, ERHIAccess::RTV));

	FRHITexture* ColorRT = DstTexture->GetTexture2DArray() ? DstTexture->GetTexture2DArray() : DstTexture->GetTexture2D();
	FRHIRenderPassInfo RenderPassInfo(ColorRT, RTAction);
	RHICmdList.BeginRenderPass(RenderPassInfo, TEXT("OpenXRHMD_CopyTexture"));
	{
		if (bClearGreen || bNoAlpha)
		{
			const FIntRect ClearRect(0, 0, DstTexture->GetSizeX(), DstTexture->GetSizeY());
			RHICmdList.SetViewport(ClearRect.Min.X, ClearRect.Min.Y, 0, ClearRect.Max.X, ClearRect.Max.Y, 1.0f);

			if (bClearGreen)
			{
				DrawClearQuad(RHICmdList, FLinearColor::Green);
			}
			else
			{
				// For opaque texture copies, we want to make sure alpha is initialized to 1.0f
				DrawClearQuadAlpha(RHICmdList, 1.0f);
			}
		}

		RHICmdList.SetViewport(DstRect.Min.X, DstRect.Min.Y, 0, DstRect.Max.X, DstRect.Max.Y, 1.0f);

		FGraphicsPipelineStateInitializer GraphicsPSOInit;
		RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

		if (bClearGreen)
		{
			GraphicsPSOInit.BlendState = TStaticBlendState<CW_RGBA, BO_Add, BF_InverseSourceAlpha, BF_SourceAlpha, BO_Add, BF_Zero, BF_InverseSourceAlpha>::GetRHI();
		}
		else if (bInvertAlpha)
		{
			GraphicsPSOInit.BlendState = TStaticBlendState<CW_RGBA, BO_Add, BF_One, BF_Zero, BO_Add, BF_Zero, BF_InverseSourceAlpha >::GetRHI();
		}
		else if (bAlphaPremultiply)
		{
			if (bNoAlpha)
			{
				GraphicsPSOInit.BlendState = TStaticBlendState<CW_RGB, BO_Add, BF_One, BF_Zero, BO_Add, BF_One, BF_Zero>::GetRHI();
			}
			else
			{
				GraphicsPSOInit.BlendState = TStaticBlendState<CW_RGBA, BO_Add, BF_SourceAlpha, BF_Zero, BO_Add, BF_One, BF_Zero>::GetRHI();
			}
		}
		else
		{
			if (bNoAlpha)
			{
				GraphicsPSOInit.BlendState = TStaticBlendState<CW_RGB>::GetRHI();
			}
			else
			{
				GraphicsPSOInit.BlendState = TStaticBlendState<CW_RGBA, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha, BO_Add, BF_One, BF_InverseSourceAlpha>::GetRHI();
			}
		}

		GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
		GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
		GraphicsPSOInit.PrimitiveType = PT_TriangleList;

		FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(OpenXRHMD->GetConfiguredShaderPlatform());

		TShaderMapRef<FScreenVS> VertexShader(ShaderMap);

		TShaderRef<FGlobalShader> PixelShader;
		TShaderRef<FDisplayMappingPixelShader> DisplayMappingPS;
		TShaderRef<FScreenPS> ScreenPS;

		bool bNeedsDisplayMapping = false;
		bool bIsInputLinear = false;
		EDisplayOutputFormat TVDisplayOutputFormat = EDisplayOutputFormat::SDR_sRGB;
		EDisplayColorGamut HMDColorGamut = EDisplayColorGamut::sRGB_D65;
		EDisplayColorGamut TVColorGamut = EDisplayColorGamut::sRGB_D65;

		FOpenXRRenderBridge* RenderBridge = static_cast<FOpenXRRenderBridge*>(OpenXRHMD->GetActiveRenderBridge_GameThread(OpenXRHMD->ShouldUseSeparateRenderTarget()));
		if (FinalDstAccess == ERHIAccess::Present && RenderBridge)
		{
			EDisplayOutputFormat HMDDisplayFormat;
			bool bHMDSupportHDR;
			if (RenderBridge->HDRGetMetaDataForStereo(HMDDisplayFormat, HMDColorGamut, bHMDSupportHDR))
			{
				bool bTVSupportHDR;
				HDRGetMetaData(TVDisplayOutputFormat, TVColorGamut, bTVSupportHDR, FVector2D(0, 0), FVector2D(0, 0), nullptr);
				if (TVDisplayOutputFormat != HMDDisplayFormat || HMDColorGamut != TVColorGamut || bTVSupportHDR != bHMDSupportHDR)
				{
					// shader assumes G 2.2 for input / ST2084/sRGB for output right now
					ensure(HMDDisplayFormat == EDisplayOutputFormat::SDR_ExplicitGammaMapping);
					ensure(TVDisplayOutputFormat == EDisplayOutputFormat::SDR_sRGB || TVDisplayOutputFormat == EDisplayOutputFormat::HDR_ACES_1000nit_ST2084 || TVDisplayOutputFormat == EDisplayOutputFormat::HDR_ACES_2000nit_ST2084);
					bNeedsDisplayMapping = true;
				}
			}

			// In Android Vulkan preview, when the sRGB swapchain texture is sampled, the data is converted to linear and written to the RGBA10A2_UNORM texture.
			// However, D3D interprets integer-valued display formats as containing sRGB data, so we need to convert the linear data back to sRGB.
			if (!IsMobileHDR() && IsMobilePlatform(OpenXRHMD->GetConfiguredShaderPlatform()) && IsSimulatedPlatform(OpenXRHMD->GetConfiguredShaderPlatform()))
			{
				bNeedsDisplayMapping = true;
				TVDisplayOutputFormat = EDisplayOutputFormat::SDR_sRGB;
				bIsInputLinear = true;
			}
		}

		bNeedsDisplayMapping &= IsFeatureLevelSupported(OpenXRHMD->GetConfiguredShaderPlatform(), ERHIFeatureLevel::ES3_1);

		bool bIsArraySource = SrcTexture->GetDesc().IsTextureArray();

		if (bNeedsDisplayMapping)
		{
			FDisplayMappingPixelShader::FPermutationDomain PermutationVector;
			PermutationVector.Set<FDisplayMappingPixelShader::FArraySource>(bIsArraySource);
			PermutationVector.Set<FDisplayMappingPixelShader::FLinearInput>(bIsInputLinear);

			TShaderMapRef<FDisplayMappingPixelShader> DisplayMappingPSRef(ShaderMap, PermutationVector);

			DisplayMappingPS = DisplayMappingPSRef;
			PixelShader = DisplayMappingPSRef;
		}
		else
		{
			if (LIKELY(!bIsArraySource))
			{
				TShaderMapRef<FScreenPS> ScreenPSRef(ShaderMap);
				ScreenPS = ScreenPSRef;
				PixelShader = ScreenPSRef;
			}
			else
			{
				TShaderMapRef<FScreenFromSlice0PS> ScreenPSRef(ShaderMap);
				ScreenPS = ScreenPSRef;
				PixelShader = ScreenPSRef;
			}
		}

		GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
		GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
		GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();

		SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit, 0);

		RHICmdList.Transition(FRHITransitionInfo(SrcTexture, ERHIAccess::Unknown, ERHIAccess::SRVMask));

		const bool bSameSize = DstRect.Size() == SrcRect.Size();
		if (ScreenPS.IsValid())
		{
			FRHISamplerState* PixelSampler = bSameSize ? TStaticSamplerState<SF_Point>::GetRHI() : TStaticSamplerState<SF_Bilinear>::GetRHI();
			SetShaderParametersLegacyPS(RHICmdList, ScreenPS, PixelSampler, SrcTexture);
		}
		else if (DisplayMappingPS.IsValid())
		{
			SetShaderParametersLegacyPS(RHICmdList, DisplayMappingPS, TVDisplayOutputFormat, TVColorGamut, HMDColorGamut, SrcTexture, bSameSize);
		}

		FModuleManager::GetModulePtr<IRendererModule>("Renderer")->DrawRectangle(
			RHICmdList,
			0, 0,
			ViewportWidth, ViewportHeight,
			U, V,
			USize, VSize,
			TargetSize,
			FIntPoint(1, 1),
			VertexShader,
			EDRF_Default);

	}
	RHICmdList.EndRenderPass();

	RHICmdList.Transition(FRHITransitionInfo(DstTexture, ERHIAccess::RTV, FinalDstAccess));
}

void FHMDPICO::CreateMRCLayer(class UTexture* BackgroundRTTexture, class UTexture* ForegroundRTTexture)
{
	ENQUEUE_RENDER_COMMAND(FCreateMRCLayer)(
		[this, BackgroundRTTexture = BackgroundRTTexture, ForegroundRTTexture = ForegroundRTTexture](FRHICommandListImmediate& RHICmdList)
		{
			if (BackgroundRTTexture && BackgroundRTTexture->GetResource() && ForegroundRTTexture && ForegroundRTTexture->GetResource())
			{
				MRCLayerDesc_RenderThread.PositionType = IStereoLayers::ELayerType::FaceLocked;
				MRCLayerDesc_RenderThread.Transform.SetLocation(FVector(100, 0, 0));
				MRCLayerDesc_RenderThread.TextureObj = ForegroundRTTexture;
				MRCLayerDesc_RenderThread.LeftTextureObj = BackgroundRTTexture;
				MRCLayerDesc_RenderThread.Flags = IStereoLayers::ELayerFlags::LAYER_FLAG_TEX_CONTINUOUS_UPDATE | IStereoLayers::ELayerFlags::LAYER_FLAG_TEX_NO_ALPHA_CHANNEL;
				MRCLayerDesc_RenderThread.QuadSize = FVector2D(100, 100);
			}
		});
}

void FHMDPICO::DestroyMRCLayer()
{
	ENQUEUE_RENDER_COMMAND(FDestroyMRCLayer)(
		[this](FRHICommandListImmediate& RHICmdList)
		{
			MRCLayerDesc_RenderThread.TextureObj.Reset();
			MRCLayerDesc_RenderThread.LeftTextureObj.Reset();
			MRCLayer.Reset();
		});
	MRCSceneCapture2DPICO = nullptr;
}

bool FHMDPICO::GetExternalCameraInfo(int32& width, int32& height, float& fov)
{
	if (MRCDebugMode.UseCustomCameraInfo)
	{
		width = MRCDebugMode.Width;
		height = MRCDebugMode.Height;
		fov = MRCDebugMode.Fov;
		return true;
	}

	if (Session && bSupportMRCExtension && xrGetExternalCameraInfoPICO)
	{
		XrExternalCameraParameterPICO ExternalCameraInfo = { XR_TYPE_EXTERNAL_CAMERA_PARAMETER_PICO };
		if (XR_SUCCEEDED(xrGetExternalCameraInfoPICO(Session, &ExternalCameraInfo)))
		{
			width = ExternalCameraInfo.width;
			height = ExternalCameraInfo.height;
			fov = ExternalCameraInfo.fov;
			return true;
		}
	}
	return false;
}

bool FHMDPICO::GetExternalCameraPose(FTransform& Pose)
{
	if (MRCDebugMode.UseCustomTransform)
	{
		Pose = MRCDebugMode.Pose;
		return true;
	}

	if (bSupportMRCExtension && Session && CurrentBaseSpace && MRCSpace && OpenXRHMD)
	{
		XrSpaceLocation NewLocation = { XR_TYPE_SPACE_LOCATION };
		const XrResult Result = xrLocateSpace(MRCSpace, CurrentBaseSpace, CurrentDisplayTime, &NewLocation);
		if (Result != XR_SUCCESS)
		{
			return false;
		}

		if (!(NewLocation.locationFlags & (XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT)))
		{
			return false;
		}

		const FQuat Orientation = ToFQuat(NewLocation.pose.orientation);
		const FVector Position = ToFVector(NewLocation.pose.position, OpenXRHMD->GetWorldToMetersScale());

		FTransform TrackingToWorld = OpenXRHMD->GetTrackingToWorldTransform();
		Pose = FTransform(Orientation, Position) * TrackingToWorld;
		return true;
	}
	return false;
}

void FHMDPICO::EnableMRCDebugMode(class UWorld* WorldContext, bool Enable, bool ViewInHMD, bool UseCustomTransform, const FTransform& Pose, bool UseCustomCameraInfo, int Width, int Height, float Fov)
{
	bSupportMRCExtension = Enable;
	bIsMRCRunning |= Enable;
	bIsMRCRunningStored |= Enable;
	if (Enable)
	{
		MRCDebugMode.EnableExtension = Enable;
		MRCDebugMode.ViewInHMD = ViewInHMD;
		MRCDebugMode.UseCustomTransform = UseCustomTransform;
		MRCDebugMode.Pose = Pose;
		MRCDebugMode.UseCustomCameraInfo = UseCustomCameraInfo;
		MRCDebugMode.Width = Width;
		MRCDebugMode.Height = Height;
		MRCDebugMode.Fov = Fov;
	}
	else
	{
		MRCDebugMode = {};
	}
}

void FHMDPICO::DisableMRCForegroundLayer(UObject* WorldContextObject, bool Disable)
{
	bIsMRCForegroundLayerDisabled = Disable;
	if (MRCSceneCapture2DPICO)
	{
		MRCSceneCapture2DPICO->DisableForegroundLayer = bIsMRCForegroundLayerDisabled;
	}
}

void FHMDPICO::PauseMRC(bool Pause)
{
	if (Pause)
	{
		if (bIsMRCRunning)
		{
			bIsMRCRunning = false;
		}
	}
	else
	{
		if (bIsMRCRunningStored && !bIsMRCRunning)
		{
			bIsMRCRunning = true;
		}
	}
}

bool FHMDPICO::GetAdaptivePixelDensity(EAdaptiveResolutionSettingPICO Setting, float& PixelDensity)
{
	if (OpenXRHMD && bSupportAdaptiveResolution && Session)
	{
		uint32 SizeX, SizeY;
		GetCurrentRenderTargetSize(SizeX, SizeY);
		FIntPoint ViewportSize =
		{
			FMath::CeilToInt(SizeX * CurrentDynamicPixelDensity),
			FMath::CeilToInt(SizeY * CurrentDynamicPixelDensity)
		};
		QuantizeSceneBufferSize(ViewportSize, ViewportSize);

		XrExtent2Di Extent2D = { ViewportSize.X, ViewportSize.Y };
		if (XR_SUCCEEDED(xrUpdateAdaptiveResolutionPICO(Session, (XrAdaptiveResolutionSettingPICO)Setting, Extent2D, &Extent2D)))
		{
			PixelDensity = (float)Extent2D.height / (float)SizeY;
			return true;
		}
	}
	return false;
}

FIntPoint FHMDPICO::GetDefaultRenderTargetSize()
{
	if (OpenXRHMD)
	{
		return OpenXRHMD->GetIdealRenderTargetSize();
	}
	return FIntPoint();
}

void FHMDPICO::GetCurrentRenderTargetSize(uint32& InOutSizeX, uint32& InOutSizeY)
{
	InOutSizeX = InOutSizeY = 0;
	if (OpenXRHMD && GEngine)
	{
		check(GEngine->GameViewport->Viewport);
		OpenXRHMD->CalculateRenderTargetSize(*GEngine->GameViewport->Viewport, InOutSizeX, InOutSizeY);
	}
}

bool FHMDPICO::SetProjectionLayerColorMatrix3x3f(bool Enable, FVector3f ColumnA, FVector3f ColumnB, FVector3f ColumnC)
{
	if (bSupportColorMatrixExtension)
	{
		bUseColorMatrixExtension = Enable;
		if (bUseColorMatrixExtension)
		{
			ColorMatrix3x3f[0] = ColumnA.X;
			ColorMatrix3x3f[1] = ColumnA.Y;
			ColorMatrix3x3f[2] = ColumnA.Z;
			ColorMatrix3x3f[3] = ColumnB.X;
			ColorMatrix3x3f[4] = ColumnB.Y;
			ColorMatrix3x3f[5] = ColumnB.Z;
			ColorMatrix3x3f[6] = ColumnC.X;
			ColorMatrix3x3f[7] = ColumnC.Y;
			ColorMatrix3x3f[8] = ColumnC.Z;
		}
		return true;
	}
	return false;
}

bool FHMDPICO::GetViewportSize(FIntPoint& ViewportSize)
{
	if (OpenXRHMD && bSupportAdaptiveResolution && Session)
	{
		uint32 SizeX, SizeY;
		GetCurrentRenderTargetSize(SizeX, SizeY);
		float PixelDensity = FMath::Clamp(CurrentDynamicPixelDensity, MinimumResolutionScale, 1.0f);
		ViewportSize =
		{
			FMath::CeilToInt(SizeX * PixelDensity),
			FMath::CeilToInt(SizeY * PixelDensity)
		};
		QuantizeSceneBufferSize(ViewportSize, ViewportSize);
		return true;
	}
	return false;
}
