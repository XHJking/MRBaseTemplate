// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PICO_MR.h"

class FSpatialMeshExtensionPICO : public FMixedRealityPICO
{
public:
	FSpatialMeshExtensionPICO();
	virtual ~FSpatialMeshExtensionPICO() {}

	/** IOpenXRExtensionPlugin implementation */
	virtual FString GetDisplayName() override
	{
		return FString(TEXT("SpatialMeshPICO"));
	}
	
	virtual bool GetOptionalExtensions(TArray<const ANSICHAR*>& OutExtensions) override;
	virtual void PostGetSystem(XrInstance InInstance, XrSystemId InSystem) override;
	virtual void PostCreateSession(XrSession InSession) override;

	virtual void OnDestroySession(XrSession InSession) override;
	
	static FSpatialMeshExtensionPICO* GetInstance()
	{
		static FSpatialMeshExtensionPICO Instance;
		return &Instance;
	}

	virtual bool CreateProvider(const FSenseDataProviderCreateInfoBasePICO& CreateInfo,EResultPICO& OutResult) override;
	
	bool RequestSpatialTriangleMesh(const FPICOPollFutureDelegate& Delegate,EResultPICO& OutResult);
	bool GetSpatialTriangleMeshInfos(const XrFutureEXT& FutureHandle,
		TArray<FSpatialMeshInfoPICO>& MeshInfos,EResultPICO& OutResult);

	ESpatialMeshLodPICO GetCurrentSpatialMeshLod();
	void ClearMeshProviderBuffer();
	
private:
	bool IsContainsInLastUpdate(const FSpatialUUIDPICO& UUID);
	void SetLastUUIDToMRMeshInfoMap(const TMap<FSpatialUUIDPICO,FSpatialMeshInfoPICO>& UUIDToMRMeshInfoMap);
	int64_t GetLastUpdateTimeByUUID(const FSpatialUUIDPICO& UUID);
	
	TMap<FSpatialUUIDPICO, FSpatialMeshInfoPICO> CachedUUIDToMRMeshInfoMap;

	ESpatialMeshLodPICO CurrentLod;
	mutable FCriticalSection CriticalSection;

	bool bsupportsSpatialMeshEXT = false;
	bool bsupportsSpatialMesh = false;
};