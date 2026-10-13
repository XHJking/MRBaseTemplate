// Copyright® 2015-2023 PICO Technology Co., Ltd. All rights reserved.
// This plugin incorporates portions of the Unreal® Engine. Unreal® is a trademark or registered trademark of Epic Games, Inc. in the United States of America and elsewhere.
// Unreal® Engine, Copyright 1998 – 2023, Epic Games, Inc. All rights reserved. 

#pragma once

#include "CoreMinimal.h"
#include "MRMeshComponent.h"
#include "PICO_MRTypes.h"
#include "PICO_SpatialMeshComponent.generated.h"

UCLASS(hidecategories = (Object, LOD), meta = (BlueprintSpawnableComponent), ClassGroup = Rendering)
class USpatialMeshComponentPICO : public UMRMeshComponent
{
	GENERATED_BODY()
public:

	USpatialMeshComponentPICO(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintPure, Category = "PICO|MR")
	ESemanticLabelPICO GetSemanticByIndex(int32 Index);

	void AddAnchorToSceneLabel(int32 Index,ESemanticLabelPICO SceneLabel);

	void SetUpdateTime(uint64 Time);

	int64 GetUpdateTime() const;
	
protected:
	TMap<int32,ESemanticLabelPICO> IndexToAnchorSceneLabelMap; //Index ->SceneLabel
	uint64 LastUpdateTime = 0;
};