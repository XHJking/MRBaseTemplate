// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "SG_PICOMR.generated.h"

/**
 * 大空间地图共享模式的锚点空间数据
 * 依赖：企业管理平台开启"大空间地图共享"，所有设备共用同一系统地图
 * 坐标基于 LocalFloor 追踪原点，跨设备通用，无需 PICO 云端
 */
USTRUCT()
struct FAnchorSpaceData
{
	GENERATED_BODY()

	// 锚点在世界空间中的变换（大空间地图共享下跨设备一致）
	UPROPERTY()
	FTransform Transform = FTransform::Identity;

	// 锚点对应的 Actor 类
	UPROPERTY()
	TSubclassOf<AActor> ActorClass;
};

/**
 *
 */
UCLASS()
class MRBASEPLUGIN_API USG_PICOMR : public USaveGame
{
	GENERATED_BODY()

public:

	UPROPERTY()
	TMap<FString, TSubclassOf<AActor>> AllAnchorsMap;

	// 大空间地图共享模式：锚点名称 → 空间数据
	UPROPERTY()
	TMap<FString, FAnchorSpaceData> AnchorSpaceMap;


};
