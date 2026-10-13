// Copyright® 2015-2023 PICO Technology Co., Ltd. All rights reserved.
// This plugin incorporates portions of the Unreal® Engine. Unreal® is a trademark or registered trademark of Epic Games, Inc. in the United States of America and elsewhere.
// Unreal® Engine, Copyright 1998 – 2023, Epic Games, Inc. All rights reserved. 

#pragma once

#include "CoreMinimal.h"
#include "PICO_MRTypes.h"
#include "PICO_SpatialMeshComponent.h"
#include "PICO_SpatialMeshActor.generated.h"

UCLASS(BlueprintType,DisplayName="Spatial Mesh Actor PICO")
class ASpatialMeshActorPICO : public AActor
{
	GENERATED_BODY()

public:
	ASpatialMeshActorPICO(const FObjectInitializer& ObjectInitializer);
	
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type Reason) override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spatial Mesh PICO",meta=(ExposeOnSpawn = true))
	bool bDrawOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spatial Mesh PICO",meta=(ExposeOnSpawn = true))
	UMaterialInterface* SpatialMeshMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spatial Mesh PICO",meta=(ExposeOnSpawn = true))
	TEnumAsByte<ECollisionEnabled::Type> CollisionType=ECollisionEnabled::Type::QueryAndPhysics;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spatial Mesh PICO",meta=(ExposeOnSpawn = true))
	bool bSpatialMeshVisible=true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spatial Mesh PICO",meta=(ExposeOnSpawn = true))
	TMap<ESemanticLabelPICO,FColor> SemanticToColors;
	
	UFUNCTION(BlueprintCallable, Category = "PICO|MR")
	bool StartDraw();

	UFUNCTION(BlueprintCallable, Category = "PICO|MR")
	bool PauseDraw();

	UFUNCTION(BlueprintCallable, Category = "PICO|MR")
	void SetMeshVisibility(bool visibility);

	UFUNCTION(BlueprintCallable, Category = "PICO|MR")
	void EnableDebugFunction(bool bEnable);
	
	UFUNCTION(BlueprintCallable, Category = "PICO|MR")
	bool ClearMesh();

	UFUNCTION(BlueprintPure, Category = "PICO|MR")
	int32 GetMeshNum();

	UFUNCTION(BlueprintPure, Category = "PICO|MR")
	int32 GetDrawsNum();

	UFUNCTION(BlueprintPure, Category = "PICO|MR")
	int32 GetDrawnPrimitives();

private:
	UFUNCTION()
	void HandleRequestSpatialMeshContentsEvent(EResultPICO Result, const TArray<FSpatialMeshInfoPICO>& MeshInfos);
	UFUNCTION()
	void HandleMeshDataUpdatedEvent();
	FColor GetColorBySceneLabel(ESemanticLabelPICO SceneLabel);

	bool UpdateMeshByMeshInfo(USpatialMeshComponentPICO* SpatialMesh, const FSpatialMeshInfoPICO& MeshInfo);
	bool bEnableDebugFunction=false;

protected:
	UPROPERTY(Transient)
	TMap<FSpatialUUIDPICO, USpatialMeshComponentPICO*> EntityToMeshMap;
	TQueue<TArray<FSpatialMeshInfoPICO>> MeshInfoQueue;
	int32 NumDrawCalls=0;
	int32 DrawnPrimitives=0;
};