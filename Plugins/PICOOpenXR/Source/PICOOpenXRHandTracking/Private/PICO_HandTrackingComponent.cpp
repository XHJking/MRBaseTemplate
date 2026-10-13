// Copyright 2023 PICO Inc. All Rights Reserved.

#include "PICO_HandTrackingComponent.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Engine/SkeletalMesh.h"
#include "PICO_HandTrackingFunctionLibrary.h"

UHandTrackingComponentPICO::UHandTrackingComponentPICO(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
	, SkeletonMeshType(EHandTypePICO::HandLeft)
	, ApplyLocationToEveryBone(false)
	, AutoHide(false)
	, bHandTrackingAvailable(false)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;

	for (int KeypointIndex = 0; KeypointIndex < EHandKeypointCount; KeypointIndex++)
	{
		BoneMappings.Add(EHandKeypoint(KeypointIndex), TEXT(""));
	}
}

void UHandTrackingComponentPICO::BeginPlay()
{
	Super::BeginPlay();
	FXRMotionControllerData Data;
	UHeadMountedDisplayFunctionLibrary::GetMotionControllerData(nullptr, EControllerHand(SkeletonMeshType), Data);
	if (Data.DeviceVisualType == EXRVisualType::Hand)
	{
		bHandTrackingAvailable = true;
	}
	if (AutoHide)
	{
		SetHiddenInGame(true, true);
	}
}

void UHandTrackingComponentPICO::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	bool bHidden = true;
	if (bHandTrackingAvailable && GetSkinnedAsset())
	{
		FXRMotionControllerData Data;
		UHeadMountedDisplayFunctionLibrary::GetMotionControllerData(nullptr, EControllerHand(SkeletonMeshType), Data);
		if (Data.bValid)
		{
			bHidden = false;
			for (auto& BoneMapping : BoneMappings)
			{
				const auto& HandKeypoint = BoneMapping.Key;
				const auto& BoneName = BoneMapping.Value;
				int32 BoneIndex = GetSkinnedAsset()->GetRefSkeleton().FindBoneIndex(BoneName);
				if (BoneIndex >= 0)
				{
					if(Data.HandKeyPositions.IsValidIndex((uint8)HandKeypoint))
					{
						const FQuat& WorldRotation = Data.HandKeyRotations[(uint8)HandKeypoint];
						const FVector& WorldLocation = Data.HandKeyPositions[(uint8)HandKeypoint];

						SetBoneRotationByName(BoneName, WorldRotation.Rotator(), EBoneSpaces::WorldSpace);

						if(HandKeypoint == EHandKeypoint::Wrist || ApplyLocationToEveryBone)
						{
							SetBoneLocationByName(BoneName, WorldLocation, EBoneSpaces::WorldSpace);
						}

						if(HandKeypoint == EHandKeypoint::Wrist)
						{
							this->SetWorldLocation(WorldLocation);
							this->SetWorldRotation(WorldRotation);
						}

						if (AutoScaleComponent)
						{
							float Scale = 1.0f;
							UHandTrackingFunctionLibraryPICO::GetHandTrackingMeshScalePICO(EControllerHand(SkeletonMeshType), Scale);
							this->SetRelativeScale3D(FVector(Scale));
						}
					}
				}
			}
		}
		else
		{
			bHidden = true;
		}
	}

	if (AutoHide && bHidden != bHiddenInGame)
	{
		SetHiddenInGame(bHidden, true);
	}
}
