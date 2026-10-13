// Copyright 2023 PICO Inc. All Rights Reserved.

#include "PICO_HMDModule.h"
#include "ShaderCore.h"

void FPICOOpenXRHMDModule::StartupModule()
{
	HMDPICOExtension.Register();

#if PLATFORM_ANDROID
	static auto* DisableOpenXROnAndroidWithoutOculusCVAR = IConsoleManager::Get().FindConsoleVariable(TEXT("xr.DisableOpenXROnAndroidWithoutOculus"));
	if (DisableOpenXROnAndroidWithoutOculusCVAR)
	{
		DisableOpenXROnAndroidWithoutOculusCVAR->Set(false);
	}
#endif
}

void FPICOOpenXRHMDModule::ShutdownModule()
{
	HMDPICOExtension.Unregister();
}

IMPLEMENT_MODULE(FPICOOpenXRHMDModule, PICOOpenXRHMD)
