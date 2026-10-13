// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MRBasePlugin/SaveGame/SG_PICOMR.h"
#include "AC_PC_MRSptialAnchorSpace.generated.h"

class AActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSpaceAnchorAdded, FString, AnchorId, AActor*, AnchorActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSpaceAnchorRemoved, FString, AnchorId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSpaceAnchorsSpawned, const TArray<AActor*>&, SpawnedActors);

/**
 * 大空间地图共享模式的空间锚点组件
 *
 * 原理：企业管理平台开启"大空间地图共享"后，所有设备共用同一系统地图，
 * LocalFloor 追踪原点对齐同一物理空间，World 坐标跨设备通用。
 * 因此只需把锚点 Actor 的 FTransform 存入 SaveGame，拷贝 .sav 到其他设备，
 * 直接 SpawnActor 即可出现在相同物理位置。完全不依赖 PICO 云端与账号。
 *
 * 锚点 Key 为内部自动生成的随机 ID（FGuid），调用方只需提供 Transform 和 ActorClass。
 */
UCLASS( ClassGroup=(Custom), Blueprintable)
class MRBASEPLUGIN_API UAC_PC_MRSptialAnchorSpace : public UActorComponent
{
	GENERATED_BODY()

public:
	UAC_PC_MRSptialAnchorSpace();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	UPROPERTY(BlueprintAssignable, Category = "MR|AnchorSpace")
	FOnSpaceAnchorAdded OnSpaceAnchorAddedDelegate;

	UPROPERTY(BlueprintAssignable, Category = "MR|AnchorSpace")
	FOnSpaceAnchorRemoved OnSpaceAnchorRemovedDelegate;

	UPROPERTY(BlueprintAssignable, Category = "MR|AnchorSpace")
	FOnSpaceAnchorsSpawned OnSpaceAnchorsSpawnedDelegate;

	/** 添加一个锚点（自动生成随机 ID）并立即存盘，返回锚点 ID */
	UFUNCTION(BlueprintCallable, Category = "MR|AnchorSpace")
	FString Space_AddAnchor(FTransform Transform, TSubclassOf<AActor> ActorClass);

	/** 添加一个锚点（自动生成随机 ID）并立即生成 Actor，返回生成的 Actor */
	UFUNCTION(BlueprintCallable, Category = "MR|AnchorSpace")
	AActor* Space_AddAndSpawnAnchor(FTransform Transform, TSubclassOf<AActor> ActorClass);

	/** 按 ID 移除锚点记录（连同已生成的 Actor 一起销毁）并立即存盘 */
	UFUNCTION(BlueprintCallable, Category = "MR|AnchorSpace")
	void Space_RemoveAnchor(FString AnchorId);

	/** 清空所有锚点记录并立即存盘 */
	UFUNCTION(BlueprintCallable, Category = "MR|AnchorSpace")
	void Space_ClearAllAnchors();

	/** 从存档生成所有锚点 Actor（幂等：已存在的跳过，缺失的补生成） */
	UFUNCTION(BlueprintCallable, Category = "MR|AnchorSpace")
	void Space_SpawnAllAnchors();

	/** 按 ID 从存档生成单个锚点 Actor */
	UFUNCTION(BlueprintCallable, Category = "MR|AnchorSpace")
	AActor* Space_SpawnAnchor(FString AnchorId);

	/** 销毁所有已生成的锚点 Actor（不影响存档记录） */
	UFUNCTION(BlueprintCallable, Category = "MR|AnchorSpace")
	void Space_DestroyAllSpawnedAnchors();

	/** 保存锚点记录到存档 */
	UFUNCTION(BlueprintCallable, Category = "MR|AnchorSpace")
	void Space_SaveGame();

	/** 从存档加载锚点记录 */
	UFUNCTION(BlueprintCallable, Category = "MR|AnchorSpace")
	void Space_LoadGame();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MR|AnchorSpace")
	int32 Space_GetAnchorCount() const;

	/** 获取所有锚点 ID */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MR|AnchorSpace")
	TArray<FString> Space_GetAnchorIds() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MR|AnchorSpace")
	bool Space_GetAnchorTransform(FString AnchorId, FTransform& OutTransform) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MR|AnchorSpace")
	bool Space_HasAnchor(FString AnchorId) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MR|AnchorSpace")
	TArray<AActor*> Space_GetSpawnedAnchors() const;

protected:
	UPROPERTY()
	TMap<FString, FAnchorSpaceData> AnchorSpaceMap;

	UPROPERTY()
	TMap<FString, TObjectPtr<AActor>> SpawnedAnchors;

	UPROPERTY(EditDefaultsOnly, Category = "SlotName")
	FString SlotName = TEXT("MRSpatialAnchorSpace");

	UPROPERTY()
	TObjectPtr<USG_PICOMR> MRSaveGame;

private:
	/** 生成随机锚点 ID */
	FString Space_GenerateAnchorId() const;

	AActor* Space_SpawnAnchorById(const FString& AnchorId, const FTransform& Transform, TSubclassOf<AActor> ActorClass);
};
