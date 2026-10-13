// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AC_PC_MRSceneCaptureComponent.generated.h"

enum class EPICOResult : uint8;
enum class EPICOMRState : uint8;
enum class EPICOSpatialSceneCaptureStatus : uint8;
struct FPICOMRSceneInfo;
struct FPICOSceneLoadInfo;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSceneCaptureServiceStarted, EPICOResult, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSceneCaptureAppLaunched, EPICOResult, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSceneInfosReceived, EPICOResult, Result, const TArray<FPICOMRSceneInfo>&, SceneInfos);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSceneCaptureDataUpdated);

UENUM(BlueprintType)
enum class EMRSceneCaptureSemanticFilter : uint8
{
	All			UMETA(DisplayName = "All"),
	Floor		UMETA(DisplayName = "Floor"),
	Ceiling		UMETA(DisplayName = "Ceiling"),
	Wall		UMETA(DisplayName = "Wall"),
	Door		UMETA(DisplayName = "Door"),
	Window		UMETA(DisplayName = "Window"),
	Opening		UMETA(DisplayName = "Opening"),
	Table		UMETA(DisplayName = "Table"),
};

UCLASS( ClassGroup=(Custom), Blueprintable)
class MRBASEPLUGIN_API UAC_PC_MRSceneCaptureComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UAC_PC_MRSceneCaptureComponent();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	UPROPERTY(BlueprintAssignable, Category = "MR|SceneCapture")
	FOnSceneCaptureServiceStarted OnSceneCaptureServiceStartedDelegate;

	UPROPERTY(BlueprintAssignable, Category = "MR|SceneCapture")
	FOnSceneCaptureAppLaunched OnSceneCaptureAppLaunchedDelegate;

	UPROPERTY(BlueprintAssignable, Category = "MR|SceneCapture")
	FOnSceneInfosReceived OnSceneInfosReceivedDelegate;

	UPROPERTY(BlueprintAssignable, Category = "MR|SceneCapture")
	FOnSceneCaptureDataUpdated OnSceneCaptureDataUpdatedDelegate;

	UFUNCTION(BlueprintCallable, Category = "MR|SceneCapture")
	void LaunchSceneCaptureService();

	UFUNCTION(BlueprintCallable, Category = "MR|SceneCapture")
	void CloseSceneCaptureService();

	UFUNCTION(BlueprintCallable, Category = "MR|SceneCapture")
	void LaunchSceneCaptureApp();

	UFUNCTION(BlueprintCallable, Category = "MR|SceneCapture")
	void RequestSceneCaptures(TArray<EMRSceneCaptureSemanticFilter> SemanticFilter);

	UFUNCTION(BlueprintCallable, Category = "MR|SceneCapture")
	EPICOMRState GetSceneCaptureServiceState() const;

	UFUNCTION(BlueprintCallable, Category = "MR|SceneCapture")
	bool GetSceneBoundingBox2D(const FPICOSpatialUUID& UUID, struct FPICOBoundingBox2D& OutBoundingBox2D);

	UFUNCTION(BlueprintCallable, Category = "MR|SceneCapture")
	bool GetSceneBoundingBox3D(const FPICOSpatialUUID& UUID, struct FPICOBoundingBox3D& OutBoundingBox3D);

	UFUNCTION(BlueprintCallable, Category = "MR|SceneCapture")
	bool GetSceneBoundingPolygon(const FPICOSpatialUUID& UUID, TArray<FVector>& OutPolygonVertices);

protected:
	UFUNCTION()
	void OnSceneCaptureServiceStateUpdated(EPICOMRState State);

	UFUNCTION()
	void OnSceneCaptureDataUpdated();

	UFUNCTION()
	void OnLaunchSceneCaptureServiceSuccess(EPICOResult Result);

	UFUNCTION()
	void OnLaunchSceneCaptureServiceFailure(EPICOResult Result);

	UFUNCTION()
	void OnLaunchSceneCaptureAppSuccess(EPICOResult Result);

	UFUNCTION()
	void OnLaunchSceneCaptureAppFailure(EPICOResult Result);

	UFUNCTION()
	void OnRequestSceneCapturesSuccess(EPICOResult Result, const TArray<FPICOMRSceneInfo>& SceneInfos);

	UFUNCTION()
	void OnRequestSceneCapturesFailure(EPICOResult Result);

private:
	UPROPERTY()
	class UPICOLaunchSceneCaptureService_AsyncAction* LaunchSceneCaptureServiceAction;

	UPROPERTY()
	class UPICOLaunchSceneCaptureApp_AsyncAction* LaunchSceneCaptureAppAction;

	UPROPERTY()
	class UPICORequestSceneCaptures_AsyncAction* RequestSceneCapturesAction;

	UPROPERTY()
	class UPICOXREventManager* EventManager;
};
