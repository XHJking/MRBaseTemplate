// Fill out your copyright notice in the Description page of Project Settings.

#include "AC_PC_MRSceneCaptureComponent.h"

#include "PXR_EventManager.h"
#include "PXR_MRAsyncActions.h"
#include "PXR_MRFunctionLibrary.h"
#include "PXR_HMDFunctionLibrary.h"


UAC_PC_MRSceneCaptureComponent::UAC_PC_MRSceneCaptureComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UAC_PC_MRSceneCaptureComponent::BeginPlay()
{
	Super::BeginPlay();

	EventManager = UPICOXRHMDFunctionLibrary::PXR_GetEventManager();
	if (EventManager)
	{
		EventManager->SceneCaptureServiceStateUpdatedDelegate.AddDynamic(this, &UAC_PC_MRSceneCaptureComponent::OnSceneCaptureServiceStateUpdated);
		EventManager->SceneCaptureDataUpdatedDelegate.AddDynamic(this, &UAC_PC_MRSceneCaptureComponent::OnSceneCaptureDataUpdated);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[MRSceneCapture] EventManager is null"));
	}
}

void UAC_PC_MRSceneCaptureComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UAC_PC_MRSceneCaptureComponent::LaunchSceneCaptureService()
{
	LaunchSceneCaptureServiceAction = UPICOLaunchSceneCaptureService_AsyncAction::PXR_LaunchSceneCaptureService_Async();
	if (LaunchSceneCaptureServiceAction)
	{
		LaunchSceneCaptureServiceAction->OnSuccess.AddDynamic(this, &UAC_PC_MRSceneCaptureComponent::OnLaunchSceneCaptureServiceSuccess);
		LaunchSceneCaptureServiceAction->OnFailure.AddDynamic(this, &UAC_PC_MRSceneCaptureComponent::OnLaunchSceneCaptureServiceFailure);
		LaunchSceneCaptureServiceAction->Activate();
	}
}

void UAC_PC_MRSceneCaptureComponent::CloseSceneCaptureService()
{
	bool bSuccess = UPICOXRMRFunctionLibrary::PXR_CloseSceneCaptureService();
	UE_LOG(LogTemp, Log, TEXT("[MRSceneCapture] CloseSceneCaptureService: %d"), bSuccess);
}

void UAC_PC_MRSceneCaptureComponent::LaunchSceneCaptureApp()
{
	LaunchSceneCaptureAppAction = UPICOLaunchSceneCaptureApp_AsyncAction::PXR_LaunchSceneCaptureApp_Async();
	if (LaunchSceneCaptureAppAction)
	{
		LaunchSceneCaptureAppAction->OnSuccess.AddDynamic(this, &UAC_PC_MRSceneCaptureComponent::OnLaunchSceneCaptureAppSuccess);
		LaunchSceneCaptureAppAction->OnFailure.AddDynamic(this, &UAC_PC_MRSceneCaptureComponent::OnLaunchSceneCaptureAppFailure);
		LaunchSceneCaptureAppAction->Activate();
	}
}

void UAC_PC_MRSceneCaptureComponent::RequestSceneCaptures(TArray<EMRSceneCaptureSemanticFilter> SemanticFilter)
{
	FPICOSceneLoadInfo SceneLoadInfo;

	if (SemanticFilter.Num() == 0)
	{
		SceneLoadInfo.SemanticFilter.Add(EPICOSemanticLabel::Unknown);
		SceneLoadInfo.SemanticFilter.Add(EPICOSemanticLabel::Floor);
		SceneLoadInfo.SemanticFilter.Add(EPICOSemanticLabel::Ceiling);
		SceneLoadInfo.SemanticFilter.Add(EPICOSemanticLabel::Wall);
		SceneLoadInfo.SemanticFilter.Add(EPICOSemanticLabel::Door);
		SceneLoadInfo.SemanticFilter.Add(EPICOSemanticLabel::Window);
		SceneLoadInfo.SemanticFilter.Add(EPICOSemanticLabel::Opening);
		SceneLoadInfo.SemanticFilter.Add(EPICOSemanticLabel::Table);
	}
	else
	{
		for (EMRSceneCaptureSemanticFilter Filter : SemanticFilter)
		{
			switch (Filter)
			{
				case EMRSceneCaptureSemanticFilter::All:
					SceneLoadInfo.SemanticFilter.Add(EPICOSemanticLabel::Unknown);
					SceneLoadInfo.SemanticFilter.Add(EPICOSemanticLabel::Floor);
					SceneLoadInfo.SemanticFilter.Add(EPICOSemanticLabel::Ceiling);
					SceneLoadInfo.SemanticFilter.Add(EPICOSemanticLabel::Wall);
					SceneLoadInfo.SemanticFilter.Add(EPICOSemanticLabel::Door);
					SceneLoadInfo.SemanticFilter.Add(EPICOSemanticLabel::Window);
					SceneLoadInfo.SemanticFilter.Add(EPICOSemanticLabel::Opening);
					SceneLoadInfo.SemanticFilter.Add(EPICOSemanticLabel::Table);
					break;
				case EMRSceneCaptureSemanticFilter::Floor:
					SceneLoadInfo.SemanticFilter.Add(EPICOSemanticLabel::Floor);
					break;
				case EMRSceneCaptureSemanticFilter::Ceiling:
					SceneLoadInfo.SemanticFilter.Add(EPICOSemanticLabel::Ceiling);
					break;
				case EMRSceneCaptureSemanticFilter::Wall:
					SceneLoadInfo.SemanticFilter.Add(EPICOSemanticLabel::Wall);
					break;
				case EMRSceneCaptureSemanticFilter::Door:
					SceneLoadInfo.SemanticFilter.Add(EPICOSemanticLabel::Door);
					break;
				case EMRSceneCaptureSemanticFilter::Window:
					SceneLoadInfo.SemanticFilter.Add(EPICOSemanticLabel::Window);
					break;
				case EMRSceneCaptureSemanticFilter::Opening:
					SceneLoadInfo.SemanticFilter.Add(EPICOSemanticLabel::Opening);
					break;
				case EMRSceneCaptureSemanticFilter::Table:
					SceneLoadInfo.SemanticFilter.Add(EPICOSemanticLabel::Table);
					break;
			}
		}
	}

	RequestSceneCapturesAction = UPICORequestSceneCaptures_AsyncAction::PXR_RequestSceneCaptures_Async(SceneLoadInfo);
	if (RequestSceneCapturesAction)
	{
		RequestSceneCapturesAction->OnSuccess.AddDynamic(this, &UAC_PC_MRSceneCaptureComponent::OnRequestSceneCapturesSuccess);
		RequestSceneCapturesAction->OnFailure.AddDynamic(this, &UAC_PC_MRSceneCaptureComponent::OnRequestSceneCapturesFailure);
		RequestSceneCapturesAction->Activate();
	}
}

EPICOMRState UAC_PC_MRSceneCaptureComponent::GetSceneCaptureServiceState() const
{
	return UPICOXRMRFunctionLibrary::PXR_GetSceneCaptureServiceState();
}

bool UAC_PC_MRSceneCaptureComponent::GetSceneBoundingBox2D(const FPICOSpatialUUID& UUID, FPICOBoundingBox2D& OutBoundingBox2D)
{
	return UPICOXRMRFunctionLibrary::PXR_GetSceneBoundingBox2D(UUID, OutBoundingBox2D);
}

bool UAC_PC_MRSceneCaptureComponent::GetSceneBoundingBox3D(const FPICOSpatialUUID& UUID, FPICOBoundingBox3D& OutBoundingBox3D)
{
	return UPICOXRMRFunctionLibrary::PXR_GetSceneBoundingBox3D(UUID, OutBoundingBox3D);
}

bool UAC_PC_MRSceneCaptureComponent::GetSceneBoundingPolygon(const FPICOSpatialUUID& UUID, TArray<FVector>& OutPolygonVertices)
{
	return UPICOXRMRFunctionLibrary::PXR_GetSceneBoundingPolygon(UUID, OutPolygonVertices);
}

void UAC_PC_MRSceneCaptureComponent::OnSceneCaptureServiceStateUpdated(EPICOMRState State)
{
	switch (State)
	{
		case EPICOMRState::Initialized:
			UE_LOG(LogTemp, Log, TEXT("[MRSceneCapture] Service State: Initialized"));
			break;
		case EPICOMRState::Running:
			UE_LOG(LogTemp, Log, TEXT("[MRSceneCapture] Service State: Running"));
			break;
		case EPICOMRState::Stopped:
			UE_LOG(LogTemp, Log, TEXT("[MRSceneCapture] Service State: Stopped"));
			break;
	}
}

void UAC_PC_MRSceneCaptureComponent::OnSceneCaptureDataUpdated()
{
	UE_LOG(LogTemp, Log, TEXT("[MRSceneCapture] SceneCaptureDataUpdated"));
	OnSceneCaptureDataUpdatedDelegate.Broadcast();
}

void UAC_PC_MRSceneCaptureComponent::OnLaunchSceneCaptureServiceSuccess(EPICOResult Result)
{
	UE_LOG(LogTemp, Log, TEXT("[MRSceneCapture] LaunchSceneCaptureService Success, Result: %d"), (int32)Result);
	OnSceneCaptureServiceStartedDelegate.Broadcast(Result);
}

void UAC_PC_MRSceneCaptureComponent::OnLaunchSceneCaptureServiceFailure(EPICOResult Result)
{
	UE_LOG(LogTemp, Warning, TEXT("[MRSceneCapture] LaunchSceneCaptureService Failure, Result: %d"), (int32)Result);
	OnSceneCaptureServiceStartedDelegate.Broadcast(Result);
}

void UAC_PC_MRSceneCaptureComponent::OnLaunchSceneCaptureAppSuccess(EPICOResult Result)
{
	UE_LOG(LogTemp, Log, TEXT("[MRSceneCapture] LaunchSceneCaptureApp Success, Result: %d"), (int32)Result);
	OnSceneCaptureAppLaunchedDelegate.Broadcast(Result);
}

void UAC_PC_MRSceneCaptureComponent::OnLaunchSceneCaptureAppFailure(EPICOResult Result)
{
	UE_LOG(LogTemp, Warning, TEXT("[MRSceneCapture] LaunchSceneCaptureApp Failure, Result: %d"), (int32)Result);
	OnSceneCaptureAppLaunchedDelegate.Broadcast(Result);
}

void UAC_PC_MRSceneCaptureComponent::OnRequestSceneCapturesSuccess(EPICOResult Result, const TArray<FPICOMRSceneInfo>& SceneInfos)
{
	UE_LOG(LogTemp, Log, TEXT("[MRSceneCapture] RequestSceneCaptures Success, Result: %d, SceneCount: %d"), (int32)Result, SceneInfos.Num());
	OnSceneInfosReceivedDelegate.Broadcast(Result, SceneInfos);
}

void UAC_PC_MRSceneCaptureComponent::OnRequestSceneCapturesFailure(EPICOResult Result)
{
	UE_LOG(LogTemp, Warning, TEXT("[MRSceneCapture] RequestSceneCaptures Failure, Result: %d"), (int32)Result);

	TArray<FPICOMRSceneInfo> EmptySceneInfos;
	OnSceneInfosReceivedDelegate.Broadcast(Result, EmptySceneInfos);
}
