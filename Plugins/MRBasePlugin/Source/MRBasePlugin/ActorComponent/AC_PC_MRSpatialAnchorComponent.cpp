// Fill out your copyright notice in the Description page of Project Settings.

#include "AC_PC_MRSpatialAnchorComponent.h"

#include "PXR_EventManager.h"
#include "PXR_MRAsyncActions.h"
#include "PXR_MRFunctionLibrary.h"
#include "PXR_HMDFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "MRBasePlugin/SaveGame/SG_PICOMR.h"


UAC_PC_MRSpatialAnchorComponent::UAC_PC_MRSpatialAnchorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UAC_PC_MRSpatialAnchorComponent::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Log, TEXT("[AC_PC_MRSpatialAnchorComponent] BeginPlay"));

	LoadGame(SlotName);

	EventManager = UPICOXRHMDFunctionLibrary::PXR_GetEventManager();
	if (EventManager)
	{
		EventManager->SpatialAnchorServiceStateUpdatedDelegate.AddDynamic(this, &UAC_PC_MRSpatialAnchorComponent::OnSpatialAnchorServiceStateUpdated);
		EventManager->AnchorDataUpdatedDelegate.AddDynamic(this, &UAC_PC_MRSpatialAnchorComponent::OnAnchorDataUpdated);

		LaunchAnchorService();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[MRSpatialAnchor] EventManager is null"));
	}
}

void UAC_PC_MRSpatialAnchorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UAC_PC_MRSpatialAnchorComponent::LaunchAnchorService()
{
	LaunchAnchorServiceAction = UPICOLaunchSpatialAnchorService_AsyncAction::PXR_LaunchSpatialAnchorService_Async();
	if (LaunchAnchorServiceAction)
	{
		LaunchAnchorServiceAction->OnSuccess.AddDynamic(this, &UAC_PC_MRSpatialAnchorComponent::OnLaunchAnchorServiceSuccess);
		LaunchAnchorServiceAction->OnFailure.AddDynamic(this, &UAC_PC_MRSpatialAnchorComponent::OnLaunchAnchorServiceFailure);
		LaunchAnchorServiceAction->Activate();
	}
}

void UAC_PC_MRSpatialAnchorComponent::CloseAnchorService()
{
	UPICOXRMRFunctionLibrary::PXR_CloseSpatialAnchorService();
}

EPICOMRState UAC_PC_MRSpatialAnchorComponent::GetAnchorServiceState() const
{
	return UPICOXRMRFunctionLibrary::PXR_GetSpatialAnchorServiceState();
}

void UAC_PC_MRSpatialAnchorComponent::SpawnSpaceAnchor(UClass* AnchorClass, FTransform Transform)
{
	if (!GetWorld())
	{
		UE_LOG(LogTemp, Warning, TEXT("[MRSpatialAnchor] GetWorld() is null"));
		return;
	}

	if (!AnchorClass)
	{
		AnchorClass = AActor::StaticClass();
	}

	CurrentSpawnActor = GetWorld()->SpawnActor<AActor>(AnchorClass, Transform.GetLocation(), Transform.GetRotation().Rotator());
	if (CurrentSpawnActor)
	{
		CreateAnchorAction = UPICOCreateSpatialAnchor_AsyncAction::PXR_CreateSpatialAnchor_Async(CurrentSpawnActor, Transform);
		if (CreateAnchorAction)
		{
			CreateAnchorAction->OnSuccess.AddDynamic(this, &UAC_PC_MRSpatialAnchorComponent::OnCreateSpatialAnchorSuccess);
			CreateAnchorAction->OnFailure.AddDynamic(this, &UAC_PC_MRSpatialAnchorComponent::OnCreateSpatialAnchorFailure);
			CreateAnchorAction->Activate();
		}
	}
}

void UAC_PC_MRSpatialAnchorComponent::LoadGame(FString _SlotName)
{
	bool bExist = UGameplayStatics::DoesSaveGameExist(SlotName, 0);
	if (bExist)
	{
		MRSaveGame = Cast<USG_PICOMR>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
		AllAnchorsMap = MRSaveGame->AllAnchorsMap;
		UE_LOG(LogTemp, Log, TEXT("[MRSpatialAnchor] LoadGame: Success=%d"), bExist);
	}
	else
	{
		MRSaveGame = Cast<USG_PICOMR>(UGameplayStatics::CreateSaveGameObject(USG_PICOMR::StaticClass()));
		UE_LOG(LogTemp, Log, TEXT("[MRSpatialAnchor] LoadGame: Success=%d, MRSaveGame is created"), bExist);
	}
}

void UAC_PC_MRSpatialAnchorComponent::SaveGame(FString _SlotName)
{
	if (IsValid(MRSaveGame))
	{
		MRSaveGame->AllAnchorsMap = AllAnchorsMap;
		bool bSuccess = UGameplayStatics::SaveGameToSlot(MRSaveGame, SlotName, 0);
		UE_LOG(LogTemp, Log, TEXT("[MRSpatialAnchor] SaveGame: Success=%d"), bSuccess);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[MRSpatialAnchor] SaveGame: MRSaveGame is invalid"));
}

void UAC_PC_MRSpatialAnchorComponent::LoadAllAnchors()
{
	NeedCreate_UUIDs.Empty();
	TArray<FString> Keys;
	AllAnchorsMap.GetKeys(Keys);
	for (auto& Key : Keys)
	{
		FPICOSpatialUUID UUID = UPICOXRMRFunctionLibrary::PXR_FromStringToUUID(Key);
		if (Exist_UUIDs.Find(UUID) == INDEX_NONE)
		{
			NeedCreate_UUIDs.Add(UUID);
		}
	}

	if (NeedCreate_UUIDs.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[MRSpatialAnchor] LoadAllAnchors: NeedCreateUUIDs is empty"));
		return;
	}

	LoadAnchorsByUUID(NeedCreate_UUIDs);
}

void UAC_PC_MRSpatialAnchorComponent::DestroyAnchorByBoundActor(AActor* _BoundActor)
{
	CurrentDestroyActor = _BoundActor;
	UnpersistAnchor(CurrentDestroyActor, UsePersistLocation);
}

TArray<AActor*> UAC_PC_MRSpatialAnchorComponent::GetBoundActors() const
{
	return BoundActors;
}

void UAC_PC_MRSpatialAnchorComponent::PersistAnchor(AActor* AnchorActor, EPICOPersistLocation PersistLocation)
{
	if (!IsValid(AnchorActor))
	{
		UE_LOG(LogTemp, Warning, TEXT("[MRSpatialAnchor] PersistAnchor: AnchorActor is invalid"));
		return;
	}

	PersistAnchorAction = UPICOPersistSpatialAnchor_AsyncAction::PXR_PersistSpatialAnchor_Async(AnchorActor, PersistLocation);
	if (PersistAnchorAction)
	{
		PersistAnchorAction->OnSuccess.AddDynamic(this, &UAC_PC_MRSpatialAnchorComponent::OnPersistSpatialAnchorSuccess);
		PersistAnchorAction->OnFailure.AddDynamic(this, &UAC_PC_MRSpatialAnchorComponent::OnPersistSpatialAnchorFailure);
		PersistAnchorAction->Activate();
	}
}

void UAC_PC_MRSpatialAnchorComponent::UnpersistAnchor(AActor* AnchorActor, EPICOPersistLocation PersistLocation)
{
	if (!IsValid(AnchorActor))
	{
		UE_LOG(LogTemp, Warning, TEXT("[MRSpatialAnchor] UnpersistAnchor: AnchorActor is invalid"));
		return;
	}

	UnpersistAnchorAction = UPICOUnpersistSpatialAnchor_AsyncAction::PXR_UnpersistSpatialAnchor_Async(AnchorActor, PersistLocation);
	if (UnpersistAnchorAction)
	{
		UnpersistAnchorAction->OnSuccess.AddDynamic(this, &UAC_PC_MRSpatialAnchorComponent::OnUnpersistSpatialAnchorSuccess);
		UnpersistAnchorAction->OnFailure.AddDynamic(this, &UAC_PC_MRSpatialAnchorComponent::OnUnpersistSpatialAnchorFailure);
		UnpersistAnchorAction->Activate();
	}
}

void UAC_PC_MRSpatialAnchorComponent::LoadAnchors(EPICOPersistLocation PersistLocation)
{
	FPICOAnchorLoadInfo LoadInfo;
	LoadInfo.PersistLocation = PersistLocation;
	LoadInfo.UUIDFilter.Empty();

	LoadAnchorsAction = UPICORequestSpatialAnchors_AsyncAction::PXR_LoadSpatialAnchor_Async(LoadInfo);
	if (LoadAnchorsAction)
	{
		LoadAnchorsAction->OnSuccess.AddDynamic(this, &UAC_PC_MRSpatialAnchorComponent::OnLoadSpatialAnchorsSuccess);
		LoadAnchorsAction->OnFailure.AddDynamic(this, &UAC_PC_MRSpatialAnchorComponent::OnLoadSpatialAnchorsFailure);
		LoadAnchorsAction->Activate();
	}
}

void UAC_PC_MRSpatialAnchorComponent::LoadAnchorsByUUID(const TArray<FPICOSpatialUUID>& UUIDFilter)
{
	FPICOAnchorLoadInfo LoadInfo;
	LoadInfo.PersistLocation = UsePersistLocation;
	LoadInfo.UUIDFilter = UUIDFilter;

	LoadAnchorsAction = UPICORequestSpatialAnchors_AsyncAction::PXR_LoadSpatialAnchor_Async(LoadInfo);
	if (LoadAnchorsAction)
	{
		LoadAnchorsAction->OnSuccess.AddDynamic(this, &UAC_PC_MRSpatialAnchorComponent::OnLoadSpatialAnchorsSuccess);
		LoadAnchorsAction->OnFailure.AddDynamic(this, &UAC_PC_MRSpatialAnchorComponent::OnLoadSpatialAnchorsFailure);
		LoadAnchorsAction->Activate();
	}
}

void UAC_PC_MRSpatialAnchorComponent::DestroyAnchor(AActor* AnchorActor)
{
	if (!IsValid(AnchorActor))
	{
		UE_LOG(LogTemp, Warning, TEXT("[MRSpatialAnchor] DestroyAnchor: AnchorActor is invalid"));
		return;
	}

	EPICOResult OutResult = EPICOResult::PXR_Error_Unknow;
	bool bSuccess = UPICOXRMRFunctionLibrary::PXR_DestroyAnchorByActor(AnchorActor, OutResult);
	UE_LOG(LogTemp, Log, TEXT("[MRSpatialAnchor] DestroyAnchor: Success=%d Result=%d"), bSuccess, (int32)OutResult);
	FPICOSpatialUUID OutUUID;
	GetAnchorUUID(AnchorActor, OutUUID);
	Exist_UUIDs.Remove(OutUUID);
	AllAnchorsMap.Remove(OutUUID.ToString());
	BoundActors.Remove(AnchorActor);
	AnchorActor->Destroy();
	SaveGame(SlotName);
}

bool UAC_PC_MRSpatialAnchorComponent::IsAnchorValid(AActor* AnchorActor) const
{
	if (!IsValid(AnchorActor))
	{
		return false;
	}
	return UPICOXRMRFunctionLibrary::PXR_IsAnchorValidForActor(AnchorActor);
}

bool UAC_PC_MRSpatialAnchorComponent::GetAnchorUUID(AActor* AnchorActor, FPICOSpatialUUID& OutUUID) const
{
	if (!IsValid(AnchorActor))
	{
		return false;
	}
	EPICOResult OutResult = EPICOResult::PXR_Error_Unknow;
	return UPICOXRMRFunctionLibrary::PXR_GetAnchorEntityUuid(AnchorActor, OutUUID, OutResult);
}

bool UAC_PC_MRSpatialAnchorComponent::GetAnchorPose(AActor* AnchorActor, FTransform& OutTransform) const
{
	if (!IsValid(AnchorActor))
	{
		return false;
	}
	EPICOResult OutResult = EPICOResult::PXR_Error_Unknow;
	return UPICOXRMRFunctionLibrary::PXR_GetAnchorPoseByActor(AnchorActor, OutTransform, OutResult);
}

void UAC_PC_MRSpatialAnchorComponent::OnSpatialAnchorServiceStateUpdated(EPICOMRState State)
{
	switch (State)
	{
		case EPICOMRState::Initialized:
			UE_LOG(LogTemp, Log, TEXT("[MRSpatialAnchor] Service State: Initialized"));
			break;
		case EPICOMRState::Running:
			UE_LOG(LogTemp, Log, TEXT("[MRSpatialAnchor] Service State: Running"));
			UPICOXRHMDFunctionLibrary::PXR_SetSeeThroughBackground(true);
			LoadAllAnchors();
			break;
		case EPICOMRState::Stopped:
			UE_LOG(LogTemp, Log, TEXT("[MRSpatialAnchor] Service State: Stopped"));
			UPICOXRHMDFunctionLibrary::PXR_SetSeeThroughBackground(false);
			break;
	}
}

void UAC_PC_MRSpatialAnchorComponent::OnAnchorDataUpdated()
{
	UE_LOG(LogTemp, Log, TEXT("[MRSpatialAnchor] AnchorDataUpdated"));
}

void UAC_PC_MRSpatialAnchorComponent::OnLaunchAnchorServiceSuccess(EPICOResult Result)
{
	UE_LOG(LogTemp, Log, TEXT("[MRSpatialAnchor] LaunchAnchorService Success, Result: %d"), (int32)Result);
}

void UAC_PC_MRSpatialAnchorComponent::OnLaunchAnchorServiceFailure(EPICOResult Result)
{
	UE_LOG(LogTemp, Warning, TEXT("[MRSpatialAnchor] LaunchAnchorService Failure, Result: %d"), (int32)Result);
}

void UAC_PC_MRSpatialAnchorComponent::OnCreateSpatialAnchorSuccess(EPICOResult Result, const UPICOAnchorComponent* AnchorComponent)
{
	UE_LOG(LogTemp, Log, TEXT("[MRSpatialAnchor] CreateSpatialAnchor Success, Result: %d"), (int32)Result);
	
	OnAnchorCreatedDelegate.Broadcast(Result, CurrentSpawnActor);

	PersistAnchor(CurrentSpawnActor, UsePersistLocation);
}

void UAC_PC_MRSpatialAnchorComponent::OnCreateSpatialAnchorFailure(EPICOResult Result)
{
	UE_LOG(LogTemp, Warning, TEXT("[MRSpatialAnchor] CreateSpatialAnchor Failure, Result: %d"), (int32)Result);
	OnAnchorCreatedDelegate.Broadcast(Result, nullptr);
}

void UAC_PC_MRSpatialAnchorComponent::OnPersistSpatialAnchorSuccess(EPICOResult Result, const UPICOAnchorComponent* AnchorComponent)
{
	UE_LOG(LogTemp, Log, TEXT("[MRSpatialAnchor] PersistSpatialAnchor Success, Result: %d"), (int32)Result);

	FPICOSpatialUUID OutUUID;
	EPICOResult UUIDResult = EPICOResult::PXR_Error_Unknow;
	if (UPICOXRMRFunctionLibrary::PXR_GetAnchorEntityUuidByComponent(AnchorComponent, OutUUID, UUIDResult))
	{
		UE_LOG(LogTemp, Log, TEXT("[MRSpatialAnchor] Persisted Anchor UUID: %s"), *OutUUID.ToString());
	}
	
	OnAnchorPersistedDelegate.Broadcast(Result, CurrentSpawnActor);

	AllAnchorsMap.Add(OutUUID.ToString(), CurrentSpawnActor.GetClass());
	SaveGame(SlotName);
	Exist_UUIDs.Add(OutUUID);
	BoundActors.Add(CurrentSpawnActor);
}

void UAC_PC_MRSpatialAnchorComponent::OnPersistSpatialAnchorFailure(EPICOResult Result)
{
	UE_LOG(LogTemp, Warning, TEXT("[MRSpatialAnchor] PersistSpatialAnchor Failure, Result: %d"), (int32)Result);
	OnAnchorPersistedDelegate.Broadcast(Result, nullptr);
}

void UAC_PC_MRSpatialAnchorComponent::OnUnpersistSpatialAnchorSuccess(EPICOResult Result, const UPICOAnchorComponent* AnchorComponent)
{
	UE_LOG(LogTemp, Log, TEXT("[MRSpatialAnchor] UnpersistSpatialAnchor Success, Result: %d"), (int32)Result);

	DestroyAnchor(CurrentDestroyActor);
}

void UAC_PC_MRSpatialAnchorComponent::OnUnpersistSpatialAnchorFailure(EPICOResult Result)
{
	UE_LOG(LogTemp, Warning, TEXT("[MRSpatialAnchor] UnpersistSpatialAnchor Failure, Result: %d"), (int32)Result);
}

void UAC_PC_MRSpatialAnchorComponent::OnLoadSpatialAnchorsSuccess(EPICOResult Result, const TArray<FAnchorLoadResult>& AnchorLoadResults)
{
	UE_LOG(LogTemp, Log, TEXT("[MRSpatialAnchor] LoadSpatialAnchors Success, Result: %d, Count: %d"), (int32)Result, AnchorLoadResults.Num());

	TArray<AActor*> LoadedActors;

	UWorld* World = GetWorld();
	if (!World)
	{
		OnAnchorLoadedDelegate.Broadcast(Result, LoadedActors);
		return;
	}

	for (const FAnchorLoadResult& LoadResult : AnchorLoadResults)
	{
		UClass* SpawnClass = AllAnchorsMap.FindRef(LoadResult.AnchorUUID.ToString());
		if (SpawnClass == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("[MRSpatialAnchor] LoadSpatialAnchors: SpawnClass is nullptr for UUID %s"), *LoadResult.AnchorUUID.ToString());
			continue;
		}
		
		AActor* SpawnedActor = UPICOXRMRFunctionLibrary::PXR_SpawnActorFromLoadResult(World, LoadResult, SpawnClass);
		if (SpawnedActor)
		{
			UE_LOG(LogTemp, Log, TEXT("[MRSpatialAnchor] Loaded Anchor UUID: %s, Actor: %s"),
				*LoadResult.AnchorUUID.ToString(), *SpawnedActor->GetName());
			LoadedActors.Add(SpawnedActor);

			Exist_UUIDs.Add(LoadResult.AnchorUUID);
			BoundActors.Add(SpawnedActor);
		}
	}
	
	OnAnchorLoadedDelegate.Broadcast(Result, LoadedActors);
}

void UAC_PC_MRSpatialAnchorComponent::OnLoadSpatialAnchorsFailure(EPICOResult Result)
{
	UE_LOG(LogTemp, Warning, TEXT("[MRSpatialAnchor] LoadSpatialAnchors Failure, Result: %d"), (int32)Result);

	TArray<AActor*> EmptyActors;
	OnAnchorLoadedDelegate.Broadcast(Result, EmptyActors);
}
