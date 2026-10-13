// Copyright® 2015-2023 PICO Technology Co., Ltd. All rights reserved.
// This plugin incorporates portions of the Unreal® Engine. Unreal® is a trademark or registered trademark of Epic Games, Inc. in the United States of America and elsewhere.
// Unreal® Engine, Copyright 1998 – 2023, Epic Games, Inc. All rights reserved. 

#include "PICO_MRFunctionLibrary.h"
#include "MRMeshComponent.h"
#include "PICOOpenXRRuntimeSettings.h"
#include "PICO_SceneCapture.h"
#include "PICO_SpatialAnchor.h"
#include "PICO_SpatialMesh.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"


UMRDelegateManagerPICO* UMRFunctionLibraryPICO::PICODelegateManager = nullptr;
UMRDelegateManagerPICO* UMRFunctionLibraryPICO::GetMRDelegateManagerPICO()
{
	if (PICODelegateManager == nullptr)
	{
		PICODelegateManager = NewObject<UMRDelegateManagerPICO>();
		PICODelegateManager->AddToRoot();
	}
	return PICODelegateManager;
}

bool UMRFunctionLibraryPICO::GetAnchorEntityUuidPICO(AActor* BoundActor, FSpatialUUIDPICO& OutAnchorUUID,EResultPICO& OutResult)
{
	if (!IsValid(BoundActor) || !IsValid(BoundActor->GetWorld()))
	{
		return false;
	}
	
	UAnchorComponentPICO* AnchorComponent = Cast<UAnchorComponentPICO>(BoundActor->GetComponentByClass(UAnchorComponentPICO::StaticClass()));

	if (IsValid(AnchorComponent) && AnchorComponent->IsAnchorValid())
	{
		return FSpatialAnchorExtensionPICO::GetInstance()->GetAnchorEntityUUID(AnchorComponent, OutAnchorUUID,OutResult);
	}
	return false;
	
}

bool UMRFunctionLibraryPICO::GetAnchorEntityUuidByComponentPICO(UAnchorComponentPICO* AnchorComponent, FSpatialUUIDPICO& OutAnchorUUID,EResultPICO& OutResult)
{
	if (IsValid(AnchorComponent) && AnchorComponent->IsAnchorValid())
	{
		return FSpatialAnchorExtensionPICO::GetInstance()->GetAnchorEntityUUID(AnchorComponent, OutAnchorUUID,OutResult);
	}

	return false;
}

bool UMRFunctionLibraryPICO::GetSceneBoundingBox2DPICO(const FSpatialUUIDPICO& UUID, FBoundingBox2DPICO& Box2D)
{
	return FSceneCaptureExtensionPICO::GetInstance()->GetSpatialSceneBoundingBox2D(UUID, Box2D);
}

bool UMRFunctionLibraryPICO::GetSceneBoundingPolygonPICO(const FSpatialUUIDPICO& UUID, TArray<FVector>& OutPolygonVertices)
{
	return FSceneCaptureExtensionPICO::GetInstance()->GetSpatialSceneBoundingPolygon(UUID, OutPolygonVertices);
}

bool UMRFunctionLibraryPICO::GetSceneBoundingBox3DPICO(const FSpatialUUIDPICO& UUID, FBoundingBox3DPICO& Box3D)
{
	return FSceneCaptureExtensionPICO::GetInstance()->GetSpatialSceneBoundingBox3D(UUID, Box3D);
}

bool UMRFunctionLibraryPICO::GetAnchorPoseByActorPICO(AActor* BoundActor, FTransform& OutTransform,EResultPICO& OutResult)
{
	if (!IsValid(BoundActor) || !IsValid(BoundActor->GetWorld()))
	{
		return false;
	}

	UAnchorComponentPICO* AnchorComponent = Cast<UAnchorComponentPICO>(BoundActor->GetComponentByClass(UAnchorComponentPICO::StaticClass()));
	if (IsValid(AnchorComponent) && AnchorComponent->IsAnchorValid())
	{
		return FSpatialAnchorExtensionPICO::GetInstance()->GetAnchorPose(AnchorComponent, OutTransform,OutResult);
	}

	return false;
}

bool UMRFunctionLibraryPICO::GetAnchorPoseByComponentPICO(UAnchorComponentPICO* AnchorComponent, FTransform& OutTransform,EResultPICO& OutResult)
{
	if (IsValid(AnchorComponent) && AnchorComponent->IsAnchorValid())
	{
		return FSpatialAnchorExtensionPICO::GetInstance()->GetAnchorPose(AnchorComponent, OutTransform,OutResult);
	}

	return false;
}

AActor* UMRFunctionLibraryPICO::SpawnActorFromLoadResultPICO(UObject* WorldContext, const FAnchorLoadResultPICO& LoadResult, UClass* ActorClass)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull);
	if (!IsValid(World))
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnInfo;
	SpawnInfo.ObjectFlags |= RF_Transient;

	AActor* AnchorActor = World->SpawnActor(ActorClass, nullptr, nullptr, SpawnInfo);
	if (!IsValid(AnchorActor))
	{
		return nullptr;
	}

	UAnchorComponentPICO* AnchorComponent = Cast<UAnchorComponentPICO>(AnchorActor->GetComponentByClass(UAnchorComponentPICO::StaticClass()));
	if (IsValid(AnchorComponent) && AnchorComponent->IsAnchorValid())
	{
		return AnchorActor;
	}
	if (AnchorComponent == nullptr)
	{
		AnchorComponent = Cast<UAnchorComponentPICO>(AnchorActor->AddComponentByClass(UAnchorComponentPICO::StaticClass(), false, FTransform::Identity, false));
	}

	AnchorComponent->SetAnchorHandle(LoadResult.AnchorHandle);
	return AnchorActor;
}

bool UMRFunctionLibraryPICO::IsAnchorValidForActorPICO(AActor* BoundActor)
{
	if (!IsValid(BoundActor))
	{
		return false;
	}

	UAnchorComponentPICO* AnchorComponent = Cast<UAnchorComponentPICO>(BoundActor->GetComponentByClass(UAnchorComponentPICO::StaticClass()));
	if (IsValid(AnchorComponent) && AnchorComponent->IsAnchorValid())
	{
		return true;
	}

	return false;
}

bool UMRFunctionLibraryPICO::IsAnchorValidForComponentPICO(UAnchorComponentPICO* AnchorComponent)
{
	if (IsValid(AnchorComponent) && AnchorComponent->IsAnchorValid())
	{
		return true;
	}

	return false;
}

FString UMRFunctionLibraryPICO::FromAnchorToStringPICO(const FSpatialHandlePICO& Anchor)
{
	return Anchor.ToString();
}

FString UMRFunctionLibraryPICO::FromUUIDToStringPICO(const FSpatialUUIDPICO& AnchorUUID)
{
	return AnchorUUID.ToString();
}

FSpatialUUIDPICO UMRFunctionLibraryPICO::FromStringToUUIDPICO(const FString& AnchorUUIDString)
{
	FSpatialUUIDPICO OutAnchorUUID={};
	// Static size for the max length of the string, two chars per hex digit, 16 digits.
	checkf(AnchorUUIDString.Len() == 32, TEXT("'%s' is not a valid UUID"), *AnchorUUIDString);
	HexToBytes(AnchorUUIDString, OutAnchorUUID.UUIDArray);

	return OutAnchorUUID;
}

bool UMRFunctionLibraryPICO::CloseSpatialMeshScanningPICO(EResultPICO& OutResult)
{
	return FSpatialMeshExtensionPICO::GetInstance()->StopProvider(OutResult);
}

bool UMRFunctionLibraryPICO::ChangeSpatialMeshLodSettingPICO(ESpatialMeshLodPICO SpatialMeshLod,EResultPICO& OutResult)
{
	bool bResult=false;
	
	if (FSpatialMeshExtensionPICO::GetInstance()->GetCurrentSpatialMeshLod()!=SpatialMeshLod)
	{
		bResult=FSpatialMeshExtensionPICO::GetInstance()->StopProvider(OutResult);

		if (bResult)
		{
			bResult=FSpatialMeshExtensionPICO::GetInstance()->DestroyProvider(OutResult);
		
			if (bResult)
			{
				FSenseDataProviderCreateInfoMeshPICO cFPICOSenseDataProviderCreateInfoMesh = {};
				cFPICOSenseDataProviderCreateInfoMesh.Lod = SpatialMeshLod;
				const UPICOOpenXRRuntimeSettings* Settings = GetDefault<UPICOOpenXRRuntimeSettings>();

				if(Settings)
				{
					if (Settings->bSemanticsAlignWithTriangle)
					{
						cFPICOSenseDataProviderCreateInfoMesh.ConfigArray.Add(ESpatialMeshConfigPICO::Spatial_Mesh_Config_Semantic);
					}
					else if (Settings->bSemanticsAlignWithVertex)
					{
						cFPICOSenseDataProviderCreateInfoMesh.ConfigArray.Add(ESpatialMeshConfigPICO::Spatial_Mesh_Config_Semantic);
						cFPICOSenseDataProviderCreateInfoMesh.ConfigArray.Add(ESpatialMeshConfigPICO::Spatial_Mesh_Config_Semantic_Align_With_Vertex);
					}
				}
				bResult=FSpatialMeshExtensionPICO::GetInstance()->CreateProvider(cFPICOSenseDataProviderCreateInfoMesh,OutResult);
			}
		}
	}
	
	return bResult;
}

ESpatialMeshLodPICO UMRFunctionLibraryPICO::GetSpatialMeshLodSettingPICO()
{
	return FSpatialMeshExtensionPICO::GetInstance()->GetCurrentSpatialMeshLod();
}

EMRStatePICO UMRFunctionLibraryPICO::GetSpatialMeshScanningStatePICO()
{
	return FSpatialMeshExtensionPICO::GetInstance()->GetSenseDataProviderState();
}

EMRStatePICO UMRFunctionLibraryPICO::GetSpatialAnchorServiceStatePICO()
{
	return FSpatialAnchorExtensionPICO::GetInstance()->GetSenseDataProviderState();
}

EMRStatePICO UMRFunctionLibraryPICO::GetSceneCaptureServiceStatePICO()
{
	return FSceneCaptureExtensionPICO::GetInstance()->GetSenseDataProviderState();
}

void UMRFunctionLibraryPICO::ResetSpatialMeshInfosStatePICO()
{
	FSpatialMeshExtensionPICO::GetInstance()->ClearMeshProviderBuffer();
}

bool UMRFunctionLibraryPICO::CloseSpatialAnchorServicePICO(EResultPICO& OutResult)
{
	return FSpatialAnchorExtensionPICO::GetInstance()->StopProvider(OutResult);
}

bool UMRFunctionLibraryPICO::CloseSceneCaptureServicePICO(EResultPICO& OutResult)
{
	return FSceneCaptureExtensionPICO::GetInstance()->StopProvider(OutResult);
}


bool UMRFunctionLibraryPICO::DestroyAnchorByComponentPICO(UAnchorComponentPICO* AnchorComponent,EResultPICO& OutResult)
{
	bool bResult=false;

	if (IsValid(AnchorComponent) && AnchorComponent->IsAnchorValid())
	{
		bResult=FSpatialAnchorExtensionPICO::GetInstance()->DestroyAnchorByHandle(AnchorComponent->GetAnchorHandle(),OutResult);
		AnchorComponent->ResetAnchorHandle();
	}

	return bResult;
}


bool UMRFunctionLibraryPICO::DestroyAnchorByActorPICO(AActor* BoundActor,EResultPICO& OutResult)
{
	bool bResult=false;
	if (!IsValid(BoundActor) || !IsValid(BoundActor->GetWorld()))
	{
		return bResult;
	}

	UAnchorComponentPICO* AnchorComponent = Cast<UAnchorComponentPICO>(BoundActor->GetComponentByClass(UAnchorComponentPICO::StaticClass()));

	if (IsValid(AnchorComponent) && AnchorComponent->IsAnchorValid())
	{
		bResult=FSpatialAnchorExtensionPICO::GetInstance()->DestroyAnchorByHandle(AnchorComponent->GetAnchorHandle(),OutResult);
		AnchorComponent->ResetAnchorHandle();
	}

	return bResult;
}

bool UMRFunctionLibraryPICO::CreateSceneBoundingPolygonPICO(AActor* BoundActor,bool bNeverCreateCollision,bool bFlipPolygon,bool UseWireframe, const FTransform& Transform, const TArray<FVector>& BoundaryVertices,UMaterialInterface* DefaultMeshMaterial)
{
	auto MRMeshComponent = NewObject<UMRMeshComponent>(BoundActor);

	if (!IsValid(BoundActor)||!BoundaryVertices.Num()||!DefaultMeshMaterial)
	{
		return false;
	}
	
	MRMeshComponent->SetUsingAbsoluteLocation(true);
	MRMeshComponent->SetUsingAbsoluteRotation(true);
	MRMeshComponent->SetUsingAbsoluteScale(true);
		
	MRMeshComponent->SetEnableMeshOcclusion(false);
	MRMeshComponent->SetUseWireframe(UseWireframe);
	MRMeshComponent->SetNeverCreateCollisionMesh(bNeverCreateCollision);
	MRMeshComponent->SetEnableNavMesh(false);
		
	if (DefaultMeshMaterial)
	{
		MRMeshComponent->SetMaterial(0, DefaultMeshMaterial);
		MRMeshComponent->SetWireframeMaterial(DefaultMeshMaterial);
	}

	MRMeshComponent->SetupAttachment(BoundActor->GetRootComponent());
	MRMeshComponent->RegisterComponent();
	
	auto Vertices = BoundaryVertices;
	const auto NumPolygons = Vertices.Num();
	TArray<MRMESH_INDEX_TYPE> Indices;
	Indices.Reset(3 * NumPolygons);
	if (bFlipPolygon)
	{
		for (auto Index = 0; Index < NumPolygons; ++Index)
		{
			Indices.Add(0);
			Indices.Add((Index + 2) % NumPolygons);
			Indices.Add((Index + 1) % NumPolygons);
		}
	}
	else
	{
		for (auto Index = 0; Index < NumPolygons; ++Index)
		{
			Indices.Add(0);
			Indices.Add((Index + 1) % NumPolygons);
			Indices.Add((Index + 2) % NumPolygons);
		}
	}

			
	MRMeshComponent->UpdateMesh(Transform.GetLocation(), Transform.GetRotation(), Transform.GetScale3D(), Vertices, Indices);
	
	return true;
}
