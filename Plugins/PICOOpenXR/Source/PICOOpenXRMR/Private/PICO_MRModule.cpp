// Copyright 2023 PICO Inc. All Rights Reserved.

#include "PICO_MRModule.h"

#include "PICO_SpatialMesh.h"
#include "PICO_SpatialAnchor.h"
#include "PICO_SceneCapture.h"

void FPICOOpenXRMRModule::StartupModule()
{
	FSpatialSensingExtensionPICO::GetInstance()->Register();
	FSpatialMeshExtensionPICO::GetInstance()->Register();
	FSpatialAnchorExtensionPICO::GetInstance()->Register();
	FSceneCaptureExtensionPICO::GetInstance()->Register();
}

void FPICOOpenXRMRModule::ShutdownModule()
{
	FSpatialSensingExtensionPICO::GetInstance()->Unregister();
	FSpatialMeshExtensionPICO::GetInstance()->Unregister();
	FSpatialAnchorExtensionPICO::GetInstance()->Unregister();
	FSceneCaptureExtensionPICO::GetInstance()->Unregister();
}

IMPLEMENT_MODULE(FPICOOpenXRMRModule, PICOOpenXRMR)
