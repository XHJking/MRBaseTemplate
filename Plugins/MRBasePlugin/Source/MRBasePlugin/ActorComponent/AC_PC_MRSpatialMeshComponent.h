// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AC_PC_MRSpatialMeshComponent.generated.h"

class APICOXRSpatialMeshActor;
enum class EPICOResult : uint8;
enum class EPICOMRState : uint8;
enum class EPICOSpatialMeshLod : uint8;
struct FPICOSpatialMeshInfo;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMeshServiceStarted, EPICOResult, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMeshInfosReceived, EPICOResult, Result, const TArray<FPICOSpatialMeshInfo>&, MeshInfos);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMeshDataUpdated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMeshInfosSaved, bool, bSuccess, const FString&, FileName);

UCLASS( ClassGroup=(Custom), Blueprintable)
class MRBASEPLUGIN_API UAC_PC_MRSpatialMeshComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UAC_PC_MRSpatialMeshComponent();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	UPROPERTY(BlueprintAssignable, Category = "MR|SpatialMesh")
	FOnMeshServiceStarted OnMeshServiceStartedDelegate;

	UPROPERTY(BlueprintAssignable, Category = "MR|SpatialMesh")
	FOnMeshInfosReceived OnMeshInfosReceivedDelegate;

	UPROPERTY(BlueprintAssignable, Category = "MR|SpatialMesh")
	FOnMeshDataUpdated OnMeshDataUpdatedDelegate;

	UPROPERTY(BlueprintAssignable, Category = "MR|SpatialMesh")
	FOnMeshInfosSaved OnMeshInfosSavedDelegate;

	UFUNCTION(BlueprintCallable, Category = "MR|SpatialMesh")
	void LaunchSpatialMeshScanning();

	UFUNCTION(BlueprintCallable, Category = "MR|SpatialMesh")
	void CloseSpatialMeshScanning();

	UFUNCTION(BlueprintCallable, Category = "MR|SpatialMesh")
	void RequestSpatialMeshInfos();

	UFUNCTION(BlueprintCallable, Category = "MR|SpatialMesh")
	void RequestAndSaveMeshInfos(const FString& SaveFileName);

	UFUNCTION(BlueprintCallable, Category = "MR|SpatialMesh")
	EPICOMRState GetMeshScanningState() const;

	UFUNCTION(BlueprintCallable, Category = "MR|SpatialMesh")
	void ChangeMeshLodSetting(EPICOSpatialMeshLod SpatialMeshLod);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MR|SpatialMesh")
	EPICOSpatialMeshLod GetMeshLodSetting() const;

	UFUNCTION(BlueprintCallable, Category = "MR|SpatialMesh")
	void ResetMeshInfosState();

	UFUNCTION(BlueprintCallable, Category = "MR|SpatialMesh")
	void CF_StartDraw();

	UFUNCTION(BlueprintCallable, Category = "MR|SpatialMesh")
	void CF_PauseDraw();

	UFUNCTION(BlueprintCallable, Category = "MR|SpatialMesh")
	void CF_ClearMesh();

	UFUNCTION(BlueprintCallable, Category = "MR|SpatialMesh")
	void CF_SetMeshVisibility(bool bVisible);

protected:
	UFUNCTION()
	void OnSpatialMeshScanningStateUpdated(EPICOMRState State);

	UFUNCTION()
	void OnMeshDataUpdated();

	UFUNCTION()
	void OnLaunchSpatialMeshScanningSuccess(EPICOResult Result);

	UFUNCTION()
	void OnLaunchSpatialMeshScanningFailure(EPICOResult Result);

	UFUNCTION()
	void OnRequestSpatialMeshInfosSuccess(EPICOResult Result, const TArray<FPICOSpatialMeshInfo>& MeshInfos);

	UFUNCTION()
	void OnRequestSpatialMeshInfosFailure(EPICOResult Result);

	UFUNCTION()
	void OnResumeDelegate();

protected:

	UPROPERTY()
	TObjectPtr<APICOXRSpatialMeshActor> SpatialMeshActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MR|SpatialMesh")
	bool bUseSpatialMesh = true;

private:
	UPROPERTY()
	class UPICOLaunchSpatialMeshScanning_AsyncAction* LaunchMeshScanningAction;

	UPROPERTY()
	class UPICORequestSpatialMesh_AsyncAction* RequestMeshInfosAction;

	UPROPERTY()
	class UPICOXREventManager* EventManager;

	FString PendingSaveFileName;
	bool bPendingSaveToFile = false;
};
