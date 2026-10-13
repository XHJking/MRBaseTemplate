// Fill out your copyright notice in the Description page of Project Settings.


#include "MRGameInstance.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "PXR_EventManager.h"
#include "PXR_HMDFunctionLibrary.h"


void UMRGameInstance::Init()
{
	Super::Init();

	if (UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled())
	{
		UHeadMountedDisplayFunctionLibrary::SetTrackingOrigin(EHMDTrackingOrigin::LocalFloor);//PICO的大空间要求TrackingOrigin为LocalFloor
		UE_LOG(LogTemp, Log, TEXT("通知。已在GI中将HMDTrackingOrigin设为LocalFloor"));
	
		// int32 ret = UPICOXRHMDFunctionLibrary::PXR_SetSeeThroughBackground(true);
		// UE_LOG(LogTemp, Log, TEXT("通知。已调用PXR_SetSeeThroughBackground Ret : %d"), ret);
		//
		// UPICOXRHMDFunctionLibrary::PXR_GetEventManager()->ResumeDelegate.AddDynamic(this, &UMRGameInstance::OnResumeDelegate);
	}
}

void UMRGameInstance::OnResumeDelegate()
{
	// int32 ret = UPICOXRHMDFunctionLibrary::PXR_SetSeeThroughBackground(true);
	// UE_LOG(LogTemp, Log, TEXT("通知。在OnResumeDelegate中调用PXR_SetSeeThroughBackground Ret : %d"), ret);
}
