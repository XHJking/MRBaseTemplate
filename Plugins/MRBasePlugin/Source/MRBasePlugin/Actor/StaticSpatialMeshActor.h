// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PXR_MRTypes.h"
#include "StaticSpatialMeshActor.generated.h"

class UProceduralMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStaticMeshBuildFinished, bool, bSuccess, int32, MeshCount);

/**
 * Loads a spatial-mesh scan that was captured (and serialized to a file) on another headset
 * sharing the same LBE map, and rebuilds it as ProceduralMeshComponents for collision/occlusion.
 * With a shared large-space map both headsets share the tracking origin, so the saved
 * MeshPose transforms can be applied 1:1 without re-alignment.
 */
UCLASS(BlueprintType, Blueprintable)
class MRBASEPLUGIN_API AStaticSpatialMeshActor : public AActor
{
	GENERATED_BODY()

public:
	AStaticSpatialMeshActor();

	UPROPERTY(BlueprintAssignable, Category = "MR|StaticSpatialMesh")
	FOnStaticMeshBuildFinished OnBuildFinishedDelegate;

	UFUNCTION(BlueprintCallable, Category = "MR|StaticSpatialMesh")
	bool BuildFromMeshInfos(const TArray<FPICOSpatialMeshInfo>& MeshInfos);

	UFUNCTION(BlueprintCallable, Category = "MR|StaticSpatialMesh")
	bool LoadAndBuild(const FString& FileName);

	UFUNCTION(BlueprintCallable, Category = "MR|StaticSpatialMesh")
	void ClearRebuiltMeshes();

	UFUNCTION(BlueprintCallable, Category = "MR|StaticSpatialMesh")
	void SetRebuiltMeshesVisible(bool bVisible);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MR|StaticSpatialMesh")
	int32 GetRebuiltMeshCount() const { return RebuiltMeshComponents.Num(); }

	UFUNCTION(BlueprintCallable, Category = "MR|StaticSpatialMesh")
	static bool SaveMeshInfosToFile(const TArray<FPICOSpatialMeshInfo>& MeshInfos, const FString& FileName);

	UFUNCTION(BlueprintCallable, Category = "MR|StaticSpatialMesh")
	static bool LoadMeshInfosFromFile(const FString& FileName, TArray<FPICOSpatialMeshInfo>& OutMeshInfos);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MR|StaticSpatialMesh")
	static FString ResolveMeshFilePath(const FString& FileName);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MR|StaticSpatialMesh")
	static FString GetDefaultSaveDir();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UProceduralMeshComponent* CreateMeshComponentFor(const FPICOSpatialMeshInfo& MeshInfo);
	void BuildVertexColors(const FPICOSpatialMeshInfo& MeshInfo, TArray<FLinearColor>& OutVertexColors) const;
	FLinearColor GetColorBySemanticLabel(EPICOSemanticLabel Label) const;

protected:
	UPROPERTY(VisibleAnywhere, Category = "MR|StaticSpatialMesh")
	TObjectPtr<USceneComponent> SceneRoot;

	/**
	 * Material applied to every rebuilt mesh.
	 * - Occlusion: Unlit + Opaque + pure black (0,0,0) -> writes depth, black pixels are replaced
	 *   by the see-through camera feed, so real-world objects occlude virtual ones.
	 * - Debug: PICO SDK's Mat_WireframeMaterial.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MR|StaticSpatialMesh")
	TObjectPtr<UMaterialInterface> MeshMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MR|StaticSpatialMesh")
	bool bCreateCollision = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MR|StaticSpatialMesh")
	FName CollisionProfileName = FName(TEXT("BlockAll"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MR|StaticSpatialMesh")
	bool bMeshVisible = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MR|StaticSpatialMesh")
	bool bAutoLoadOnBeginPlay = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MR|StaticSpatialMesh", meta = (EditCondition = "bAutoLoadOnBeginPlay"))
	FString AutoLoadFileName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MR|StaticSpatialMesh")
	TMap<EPICOSemanticLabel, FLinearColor> SemanticToColors;

	/** Meshes created per frame while building, to spread the load of large scans. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MR|StaticSpatialMesh", meta = (ClampMin = "1"))
	int32 MeshesPerTick = 8;

	UPROPERTY(Transient, VisibleAnywhere, Category = "MR|StaticSpatialMesh")
	TArray<TObjectPtr<UProceduralMeshComponent>> RebuiltMeshComponents;

private:
	TArray<FPICOSpatialMeshInfo> BuildQueue;
	int32 BuildQueueIndex = 0;
	bool bBuilding = false;
};
