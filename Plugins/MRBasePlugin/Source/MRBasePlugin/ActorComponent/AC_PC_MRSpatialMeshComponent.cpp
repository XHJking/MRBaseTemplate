// Fill out your copyright notice in the Description page of Project Settings.

#include "AC_PC_MRSpatialMeshComponent.h"

#include "../Actor/StaticSpatialMeshActor.h"
#include "PXR_EventManager.h"
#include "PXR_MRAsyncActions.h"
#include "PXR_MRFunctionLibrary.h"
#include "PXR_HMDFunctionLibrary.h"
#include "PXR_SpatialMeshActor.h"
#include "Kismet/GameplayStatics.h"


UAC_PC_MRSpatialMeshComponent::UAC_PC_MRSpatialMeshComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UAC_PC_MRSpatialMeshComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!bUseSpatialMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MRSpatialMesh] bUseSpatialMesh is false"));
		return;
	}

	SpatialMeshActor = Cast<APICOXRSpatialMeshActor>(UGameplayStatics::GetActorOfClass(GetWorld(), APICOXRSpatialMeshActor::StaticClass()));

	if (!IsValid(SpatialMeshActor))
	{
		UE_LOG(LogTemp, Warning, TEXT("[MRSpatialMesh] SpatialMeshActor is null"));
		return;
	}

	EventManager = UPICOXRHMDFunctionLibrary::PXR_GetEventManager();
	if (EventManager)
	{
		EventManager->MeshScanningStateUpdatedDelegate.AddDynamic(this, &UAC_PC_MRSpatialMeshComponent::OnSpatialMeshScanningStateUpdated);
		EventManager->MeshDataUpdatedDelegate.AddDynamic(this, &UAC_PC_MRSpatialMeshComponent::OnMeshDataUpdated);

		LaunchSpatialMeshScanning();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[MRSpatialMesh] EventManager is null"));
	}
}

void UAC_PC_MRSpatialMeshComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UAC_PC_MRSpatialMeshComponent::LaunchSpatialMeshScanning()
{
	LaunchMeshScanningAction = UPICOLaunchSpatialMeshScanning_AsyncAction::PXR_LaunchSpatialMeshScanning_Async();
	if (LaunchMeshScanningAction)
	{
		LaunchMeshScanningAction->OnSuccess.AddDynamic(this, &UAC_PC_MRSpatialMeshComponent::OnLaunchSpatialMeshScanningSuccess);
		LaunchMeshScanningAction->OnFailure.AddDynamic(this, &UAC_PC_MRSpatialMeshComponent::OnLaunchSpatialMeshScanningFailure);
		LaunchMeshScanningAction->Activate();
	}
}

void UAC_PC_MRSpatialMeshComponent::CloseSpatialMeshScanning()
{
	bool bSuccess = UPICOXRMRFunctionLibrary::PXR_CloseSpatialMeshScanning();
	UE_LOG(LogTemp, Log, TEXT("[MRSpatialMesh] CloseSpatialMeshScanning: %d"), bSuccess);
}

void UAC_PC_MRSpatialMeshComponent::RequestSpatialMeshInfos()
{
	RequestMeshInfosAction = UPICORequestSpatialMesh_AsyncAction::PXR_RequestSpatialMeshInfos_Async();
	if (RequestMeshInfosAction)
	{
		RequestMeshInfosAction->OnSuccess.AddDynamic(this, &UAC_PC_MRSpatialMeshComponent::OnRequestSpatialMeshInfosSuccess);
		RequestMeshInfosAction->OnFailure.AddDynamic(this, &UAC_PC_MRSpatialMeshComponent::OnRequestSpatialMeshInfosFailure);
		RequestMeshInfosAction->Activate();
	}
}

void UAC_PC_MRSpatialMeshComponent::RequestAndSaveMeshInfos(const FString& SaveFileName)
{
	PendingSaveFileName = SaveFileName;
	bPendingSaveToFile = true;
	RequestSpatialMeshInfos();
}

EPICOMRState UAC_PC_MRSpatialMeshComponent::GetMeshScanningState() const
{
	return UPICOXRMRFunctionLibrary::PXR_GetSpatialMeshScanningState();
}

void UAC_PC_MRSpatialMeshComponent::ChangeMeshLodSetting(EPICOSpatialMeshLod SpatialMeshLod)
{
	bool bSuccess = UPICOXRMRFunctionLibrary::PXR_ChangeSpatialMeshLodSetting(SpatialMeshLod);
	UE_LOG(LogTemp, Log, TEXT("[MRSpatialMesh] ChangeMeshLodSetting: %d, Success: %d"), (uint8)SpatialMeshLod, bSuccess);
}

EPICOSpatialMeshLod UAC_PC_MRSpatialMeshComponent::GetMeshLodSetting() const
{
	return UPICOXRMRFunctionLibrary::PXR_GetSpatialMeshLodSetting();
}

void UAC_PC_MRSpatialMeshComponent::ResetMeshInfosState()
{
	UPICOXRMRFunctionLibrary::PXR_ResetSpatialMeshInfosState();
}

void UAC_PC_MRSpatialMeshComponent::CF_StartDraw()
{
	if (SpatialMeshActor)
	{
		struct { } Params;
		UFunction* Fn = SpatialMeshActor->FindFunction(FName(TEXT("StartDraw")));
		if (Fn) SpatialMeshActor->ProcessEvent(Fn, &Params);
	}
}

void UAC_PC_MRSpatialMeshComponent::CF_PauseDraw()
{
	if (SpatialMeshActor)
	{
		struct { } Params;
		UFunction* Fn = SpatialMeshActor->FindFunction(FName(TEXT("PauseDraw")));
		if (Fn) SpatialMeshActor->ProcessEvent(Fn, &Params);
	}
}

void UAC_PC_MRSpatialMeshComponent::CF_ClearMesh()
{
	if (SpatialMeshActor)
	{
		struct { } Params;
		UFunction* Fn = SpatialMeshActor->FindFunction(FName(TEXT("ClearMesh")));
		if (Fn) SpatialMeshActor->ProcessEvent(Fn, &Params);
	}
}

void UAC_PC_MRSpatialMeshComponent::CF_SetMeshVisibility(bool bVisible)
{
	if (SpatialMeshActor)
	{
		struct { bool bVisibility; } Params;
		Params.bVisibility = bVisible;
		UFunction* Fn = SpatialMeshActor->FindFunction(FName(TEXT("SetMeshVisibility")));
		if (Fn) SpatialMeshActor->ProcessEvent(Fn, &Params);
	}
}

void UAC_PC_MRSpatialMeshComponent::OnSpatialMeshScanningStateUpdated(EPICOMRState State)
{
	switch (State)
	{
		case EPICOMRState::Initialized:
			UE_LOG(LogTemp, Log, TEXT("[MRSpatialMesh] Scanning State: Initialized"));
			break;
		case EPICOMRState::Running:
			UE_LOG(LogTemp, Log, TEXT("[MRSpatialMesh] Scanning State: Running"));
			UPICOXRHMDFunctionLibrary::PXR_SetSeeThroughBackground(true);
			EventManager->ResumeDelegate.AddDynamic(this, &UAC_PC_MRSpatialMeshComponent::OnResumeDelegate);
			break;
		case EPICOMRState::Stopped:
			UE_LOG(LogTemp, Log, TEXT("[MRSpatialMesh] Scanning State: Stopped"));
			UPICOXRHMDFunctionLibrary::PXR_SetSeeThroughBackground(false);
			EventManager->ResumeDelegate.RemoveDynamic(this, &UAC_PC_MRSpatialMeshComponent::OnResumeDelegate);
			break;
	}
}

void UAC_PC_MRSpatialMeshComponent::OnMeshDataUpdated()
{
	UE_LOG(LogTemp, Log, TEXT("[MRSpatialMesh] MeshDataUpdated"));
	OnMeshDataUpdatedDelegate.Broadcast();
}

void UAC_PC_MRSpatialMeshComponent::OnLaunchSpatialMeshScanningSuccess(EPICOResult Result)
{
	UE_LOG(LogTemp, Log, TEXT("[MRSpatialMesh] LaunchSpatialMeshScanning Success, Result: %d"), (int32)Result);
	OnMeshServiceStartedDelegate.Broadcast(Result);
}

void UAC_PC_MRSpatialMeshComponent::OnLaunchSpatialMeshScanningFailure(EPICOResult Result)
{
	UE_LOG(LogTemp, Warning, TEXT("[MRSpatialMesh] LaunchSpatialMeshScanning Failure, Result: %d"), (int32)Result);
	OnMeshServiceStartedDelegate.Broadcast(Result);
}

void UAC_PC_MRSpatialMeshComponent::OnRequestSpatialMeshInfosSuccess(EPICOResult Result, const TArray<FPICOSpatialMeshInfo>& MeshInfos)
{
	UE_LOG(LogTemp, Log, TEXT("[MRSpatialMesh] RequestSpatialMeshInfos Success, Result: %d, MeshCount: %d"), (int32)Result, MeshInfos.Num());
	OnMeshInfosReceivedDelegate.Broadcast(Result, MeshInfos);

	if (bPendingSaveToFile)
	{
		bPendingSaveToFile = false;
		const bool bSaved = AStaticSpatialMeshActor::SaveMeshInfosToFile(MeshInfos, PendingSaveFileName);
		UE_LOG(LogTemp, Log, TEXT("[MRSpatialMesh] SaveMeshInfosToFile %s, File: %s"), bSaved ? TEXT("Success") : TEXT("Failed"), *AStaticSpatialMeshActor::ResolveMeshFilePath(PendingSaveFileName));
		OnMeshInfosSavedDelegate.Broadcast(bSaved, PendingSaveFileName);
		PendingSaveFileName.Reset();
	}
}

void UAC_PC_MRSpatialMeshComponent::OnRequestSpatialMeshInfosFailure(EPICOResult Result)
{
	UE_LOG(LogTemp, Warning, TEXT("[MRSpatialMesh] RequestSpatialMeshInfos Failure, Result: %d"), (int32)Result);

	if (bPendingSaveToFile)
	{
		bPendingSaveToFile = false;
		PendingSaveFileName.Reset();
		OnMeshInfosSavedDelegate.Broadcast(false, FString());
	}

	TArray<FPICOSpatialMeshInfo> EmptyMeshInfos;
	OnMeshInfosReceivedDelegate.Broadcast(Result, EmptyMeshInfos);
}

void UAC_PC_MRSpatialMeshComponent::OnResumeDelegate()
{
	UPICOXRHMDFunctionLibrary::PXR_SetSeeThroughBackground(true);
}
