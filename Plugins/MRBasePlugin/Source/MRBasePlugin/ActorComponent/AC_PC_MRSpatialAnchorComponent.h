// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PXR_MRTypes.h"
#include "Components/ActorComponent.h"
#include "AC_PC_MRSpatialAnchorComponent.generated.h"

class USG_PICOMR;
enum class EPICOResult : uint8;
enum class EPICOMRState : uint8;
enum class EPICOPersistLocation : uint8;
struct FPICOSpatialUUID;
struct FAnchorLoadResult;
class UPICOAnchorComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAnchorCreated, EPICOResult, Result, AActor*, AnchorActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAnchorPersisted, EPICOResult, Result, AActor*, AnchorActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAnchorLoaded, EPICOResult, Result, const TArray<AActor*>&, LoadedActors);

UCLASS( ClassGroup=(Custom), Blueprintable)
class MRBASEPLUGIN_API UAC_PC_MRSpatialAnchorComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UAC_PC_MRSpatialAnchorComponent();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	UPROPERTY(BlueprintAssignable, Category = "MR|SpatialAnchor")
	FOnAnchorCreated OnAnchorCreatedDelegate;

	UPROPERTY(BlueprintAssignable, Category = "MR|SpatialAnchor")
	FOnAnchorPersisted OnAnchorPersistedDelegate;

	UPROPERTY(BlueprintAssignable, Category = "MR|SpatialAnchor")
	FOnAnchorLoaded OnAnchorLoadedDelegate;

	UFUNCTION(BlueprintCallable, Category = "MR|SpatialAnchor")
	void LaunchAnchorService();

	UFUNCTION(BlueprintCallable, Category = "MR|SpatialAnchor")
	void CloseAnchorService();

	UFUNCTION(BlueprintCallable, Category = "MR|SpatialAnchor")
	EPICOMRState GetAnchorServiceState() const;

	/**
	* 加载锚点并根据存档的Class去设置相应的Actor的类
	* @param AnchorClass 锚点类
	* @param Transform 锚点位置信息
	*/
	UFUNCTION(BlueprintCallable, Category = "MR|SpatialAnchor")
	void SpawnSpaceAnchor(UClass* AnchorClass, FTransform Transform);

	UFUNCTION(BlueprintCallable, Category = "MR|SpatialAnchor")
	void LoadGame(FString _SlotName);

	UFUNCTION(BlueprintCallable, Category = "MR|SpatialAnchor")
	void SaveGame(FString _SlotName);

	/**
	* 加载锚点并根据存档的Class去设置相应的Actor的类
	*/
	UFUNCTION(BlueprintCallable, Category = "MR|SpatialAnchor")
	void LoadAllAnchors();
	
	/**
	* 去持久化锚点再删除锚点
	* @param _BoundActor 需要销毁的Actor
	*/
	UFUNCTION(BlueprintCallable, Category = "MR|SpatialAnchor")
	void DestroyAnchorByBoundActor(AActor* _BoundActor);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MR|SpatialAnchor")
	TArray<AActor*> GetBoundActors() const;

protected:

	UFUNCTION(BlueprintCallable, Category = "MR|SpatialAnchor")
	void PersistAnchor(AActor* AnchorActor, EPICOPersistLocation PersistLocation);

	UFUNCTION(BlueprintCallable, Category = "MR|SpatialAnchor")
	void UnpersistAnchor(AActor* AnchorActor, EPICOPersistLocation PersistLocation);

	UFUNCTION(BlueprintCallable, Category = "MR|SpatialAnchor")
	void LoadAnchors(EPICOPersistLocation PersistLocation);

	UFUNCTION(BlueprintCallable, Category = "MR|SpatialAnchor")
	void LoadAnchorsByUUID(const TArray<FPICOSpatialUUID>& UUIDFilter);

	UFUNCTION(BlueprintCallable, Category = "MR|SpatialAnchor")
	void DestroyAnchor(AActor* AnchorActor);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MR|SpatialAnchor")
	bool IsAnchorValid(AActor* AnchorActor) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MR|SpatialAnchor")
	bool GetAnchorUUID(AActor* AnchorActor, FPICOSpatialUUID& OutUUID) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MR|SpatialAnchor")
	bool GetAnchorPose(AActor* AnchorActor, FTransform& OutTransform) const;

protected:
	UFUNCTION()
	void OnSpatialAnchorServiceStateUpdated(EPICOMRState State);

	UFUNCTION()
	void OnAnchorDataUpdated();

	UFUNCTION()
	void OnLaunchAnchorServiceSuccess(EPICOResult Result);

	UFUNCTION()
	void OnLaunchAnchorServiceFailure(EPICOResult Result);

	UFUNCTION()
	void OnCreateSpatialAnchorSuccess(EPICOResult Result, const UPICOAnchorComponent* AnchorComponent);

	UFUNCTION()
	void OnCreateSpatialAnchorFailure(EPICOResult Result);

	UFUNCTION()
	void OnPersistSpatialAnchorSuccess(EPICOResult Result, const UPICOAnchorComponent* AnchorComponent);

	UFUNCTION()
	void OnPersistSpatialAnchorFailure(EPICOResult Result);

	UFUNCTION()
	void OnUnpersistSpatialAnchorSuccess(EPICOResult Result, const UPICOAnchorComponent* AnchorComponent);

	UFUNCTION()
	void OnUnpersistSpatialAnchorFailure(EPICOResult Result);

	UFUNCTION()
	void OnLoadSpatialAnchorsSuccess(EPICOResult Result, const TArray<FAnchorLoadResult>& AnchorLoadResults);

	UFUNCTION()
	void OnLoadSpatialAnchorsFailure(EPICOResult Result);

protected:

	UPROPERTY()
	TObjectPtr<AActor> CurrentSpawnActor = nullptr;

	UPROPERTY()
	TObjectPtr<AActor> CurrentDestroyActor = nullptr;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> BoundActors;

	UPROPERTY()
	TMap<FString, TSubclassOf<AActor>> AllAnchorsMap;

	UPROPERTY(EditDefaultsOnly, Category = "SlotName")
	FString SlotName = TEXT("MRSpatialAnchor");

	UPROPERTY(EditDefaultsOnly, Category = "PICOPersistLocation")
	EPICOPersistLocation UsePersistLocation = EPICOPersistLocation::PersistLocation_Local;

	UPROPERTY()
	TObjectPtr<USG_PICOMR> MRSaveGame;

	UPROPERTY()
	TArray<FPICOSpatialUUID> NeedCreate_UUIDs;

	UPROPERTY()
	TArray<FPICOSpatialUUID> Exist_UUIDs;


private:
	UPROPERTY()
	class UPICOLaunchSpatialAnchorService_AsyncAction* LaunchAnchorServiceAction;

	UPROPERTY()
	class UPICOCreateSpatialAnchor_AsyncAction* CreateAnchorAction;

	UPROPERTY()
	class UPICOPersistSpatialAnchor_AsyncAction* PersistAnchorAction;

	UPROPERTY()
	class UPICOUnpersistSpatialAnchor_AsyncAction* UnpersistAnchorAction;

	UPROPERTY()
	class UPICORequestSpatialAnchors_AsyncAction* LoadAnchorsAction;

	UPROPERTY()
	class UPICOXREventManager* EventManager;
};
