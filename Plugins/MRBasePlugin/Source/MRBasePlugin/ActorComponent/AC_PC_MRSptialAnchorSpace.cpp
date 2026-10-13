// Fill out your copyright notice in the Description page of Project Settings.

#include "AC_PC_MRSptialAnchorSpace.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Misc/Guid.h"


UAC_PC_MRSptialAnchorSpace::UAC_PC_MRSptialAnchorSpace()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAC_PC_MRSptialAnchorSpace::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Log, TEXT("[AC_PC_MRSptialAnchorSpace] BeginPlay"));

	Space_LoadGame();
	Space_SpawnAllAnchors();
}

void UAC_PC_MRSptialAnchorSpace::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

FString UAC_PC_MRSptialAnchorSpace::Space_GenerateAnchorId() const
{
	// FGuid 全局唯一，跨设备拷贝 .sav 后也不会冲突
	FString NewId = FGuid::NewGuid().ToString();
	while (AnchorSpaceMap.Contains(NewId))
	{
		NewId = FGuid::NewGuid().ToString();
	}
	return NewId;
}

FString UAC_PC_MRSptialAnchorSpace::Space_AddAnchor(FTransform Transform, TSubclassOf<AActor> ActorClass)
{
	FString AnchorId = Space_GenerateAnchorId();

	FAnchorSpaceData Data;
	Data.Transform = Transform;
	Data.ActorClass = ActorClass;

	AnchorSpaceMap.Add(AnchorId, Data);
	Space_SaveGame();

	UE_LOG(LogTemp, Log, TEXT("[AnchorSpace] AddAnchor: Id=%s, Loc: %s, Count: %d"),
		*AnchorId, *Transform.GetLocation().ToString(), AnchorSpaceMap.Num());

	return AnchorId;
}

AActor* UAC_PC_MRSptialAnchorSpace::Space_AddAndSpawnAnchor(FTransform Transform, TSubclassOf<AActor> ActorClass)
{
	FString AnchorId = Space_GenerateAnchorId();

	AActor* SpawnedActor = Space_SpawnAnchorById(AnchorId, Transform, ActorClass);
	if (SpawnedActor)
	{
		FAnchorSpaceData Data;
		Data.Transform = Transform;
		Data.ActorClass = ActorClass;

		AnchorSpaceMap.Add(AnchorId, Data);
		Space_SaveGame();

		UE_LOG(LogTemp, Log, TEXT("[AnchorSpace] AddAndSpawnAnchor: Id=%s, Loc: %s, Count: %d"),
			*AnchorId, *Transform.GetLocation().ToString(), AnchorSpaceMap.Num());

		OnSpaceAnchorAddedDelegate.Broadcast(AnchorId, SpawnedActor);
	}
	return SpawnedActor;
}

void UAC_PC_MRSptialAnchorSpace::Space_RemoveAnchor(FString AnchorId)
{
	if (!AnchorSpaceMap.Contains(AnchorId))
	{
		UE_LOG(LogTemp, Warning, TEXT("[AnchorSpace] RemoveAnchor: Id=%s not found"), *AnchorId);
		return;
	}

	// 销毁已生成的 Actor
	if (TObjectPtr<AActor>* FoundActor = SpawnedAnchors.Find(AnchorId))
	{
		if (IsValid(*FoundActor))
		{
			(*FoundActor)->Destroy();
		}
		SpawnedAnchors.Remove(AnchorId);
	}

	AnchorSpaceMap.Remove(AnchorId);
	Space_SaveGame();

	UE_LOG(LogTemp, Log, TEXT("[AnchorSpace] RemoveAnchor: Id=%s, Count: %d"), *AnchorId, AnchorSpaceMap.Num());
	OnSpaceAnchorRemovedDelegate.Broadcast(AnchorId);
}

void UAC_PC_MRSptialAnchorSpace::Space_ClearAllAnchors()
{
	Space_DestroyAllSpawnedAnchors();

	int32 RemovedCount = AnchorSpaceMap.Num();
	AnchorSpaceMap.Empty();
	Space_SaveGame();

	UE_LOG(LogTemp, Log, TEXT("[AnchorSpace] ClearAllAnchors: Removed %d"), RemovedCount);
}

void UAC_PC_MRSptialAnchorSpace::Space_SpawnAllAnchors()
{
	TArray<AActor*> SpawnedActors;
	int32 NewCount = 0;
	int32 SkippedCount = 0;

	for (const TPair<FString, FAnchorSpaceData>& Pair : AnchorSpaceMap)
	{
		// 已有有效 Actor 则跳过，保证重复调用幂等
		if (TObjectPtr<AActor>* FoundActor = SpawnedAnchors.Find(Pair.Key))
		{
			if (IsValid(*FoundActor))
			{
				SpawnedActors.Add(*FoundActor);
				SkippedCount++;
				continue;
			}
			// Actor 已被外部销毁，清除脏记录后重新生成
			SpawnedAnchors.Remove(Pair.Key);
		}

		AActor* SpawnedActor = Space_SpawnAnchorById(Pair.Key, Pair.Value.Transform, Pair.Value.ActorClass);
		if (SpawnedActor)
		{
			SpawnedActors.Add(SpawnedActor);
			NewCount++;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[AnchorSpace] SpawnAllAnchors: New=%d, Skipped=%d, Total=%d"),
		NewCount, SkippedCount, AnchorSpaceMap.Num());
	OnSpaceAnchorsSpawnedDelegate.Broadcast(SpawnedActors);
}

AActor* UAC_PC_MRSptialAnchorSpace::Space_SpawnAnchor(FString AnchorId)
{
	const FAnchorSpaceData* FoundData = AnchorSpaceMap.Find(AnchorId);
	if (!FoundData)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AnchorSpace] SpawnAnchor: Id=%s not found"), *AnchorId);
		return nullptr;
	}

	return Space_SpawnAnchorById(AnchorId, FoundData->Transform, FoundData->ActorClass);
}

void UAC_PC_MRSptialAnchorSpace::Space_DestroyAllSpawnedAnchors()
{
	for (TPair<FString, TObjectPtr<AActor>>& Pair : SpawnedAnchors)
	{
		if (IsValid(Pair.Value))
		{
			Pair.Value->Destroy();
		}
	}
	SpawnedAnchors.Empty();
}

void UAC_PC_MRSptialAnchorSpace::Space_SaveGame()
{
	if (IsValid(MRSaveGame))
	{
		MRSaveGame->AnchorSpaceMap = AnchorSpaceMap;
		bool bSuccess = UGameplayStatics::SaveGameToSlot(MRSaveGame, SlotName, 0);
		UE_LOG(LogTemp, Log, TEXT("[AnchorSpace] SaveGame: Success=%d, Count=%d"), bSuccess, AnchorSpaceMap.Num());
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[AnchorSpace] SaveGame: MRSaveGame is invalid"));
}

void UAC_PC_MRSptialAnchorSpace::Space_LoadGame()
{
	bool bExist = UGameplayStatics::DoesSaveGameExist(SlotName, 0);
	if (bExist)
	{
		MRSaveGame = Cast<USG_PICOMR>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
		if (IsValid(MRSaveGame))
		{
			AnchorSpaceMap = MRSaveGame->AnchorSpaceMap;
		}
		UE_LOG(LogTemp, Log, TEXT("[AnchorSpace] LoadGame: Success=%d, Count=%d"), bExist, AnchorSpaceMap.Num());
	}
	else
	{
		MRSaveGame = Cast<USG_PICOMR>(UGameplayStatics::CreateSaveGameObject(USG_PICOMR::StaticClass()));
		UE_LOG(LogTemp, Log, TEXT("[AnchorSpace] LoadGame: Success=%d, MRSaveGame is created"), bExist);
	}
}

int32 UAC_PC_MRSptialAnchorSpace::Space_GetAnchorCount() const
{
	return AnchorSpaceMap.Num();
}

TArray<FString> UAC_PC_MRSptialAnchorSpace::Space_GetAnchorIds() const
{
	TArray<FString> Keys;
	AnchorSpaceMap.GetKeys(Keys);
	return Keys;
}

bool UAC_PC_MRSptialAnchorSpace::Space_GetAnchorTransform(FString AnchorId, FTransform& OutTransform) const
{
	const FAnchorSpaceData* FoundData = AnchorSpaceMap.Find(AnchorId);
	if (FoundData)
	{
		OutTransform = FoundData->Transform;
		return true;
	}
	return false;
}

bool UAC_PC_MRSptialAnchorSpace::Space_HasAnchor(FString AnchorId) const
{
	return AnchorSpaceMap.Contains(AnchorId);
}

TArray<AActor*> UAC_PC_MRSptialAnchorSpace::Space_GetSpawnedAnchors() const
{
	TArray<AActor*> Result;
	for (const TPair<FString, TObjectPtr<AActor>>& Pair : SpawnedAnchors)
	{
		if (IsValid(Pair.Value))
		{
			Result.Add(Pair.Value);
		}
	}
	return Result;
}

AActor* UAC_PC_MRSptialAnchorSpace::Space_SpawnAnchorById(const FString& AnchorId, const FTransform& Transform, TSubclassOf<AActor> ActorClass)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AnchorSpace] SpawnAnchor: GetWorld() is null"));
		return nullptr;
	}

	UClass* SpawnClass = ActorClass ? *ActorClass : AActor::StaticClass();

	// 同 Id 已存在则先销毁，避免重复生成
	if (TObjectPtr<AActor>* FoundActor = SpawnedAnchors.Find(AnchorId))
	{
		if (IsValid(*FoundActor))
		{
			(*FoundActor)->Destroy();
		}
	}

	AActor* SpawnedActor = World->SpawnActor<AActor>(SpawnClass, Transform.GetLocation(), Transform.GetRotation().Rotator());
	if (SpawnedActor)
	{
		SpawnedActor->SetActorScale3D(Transform.GetScale3D());
		SpawnedAnchors.Add(AnchorId, SpawnedActor);

		UE_LOG(LogTemp, Log, TEXT("[AnchorSpace] SpawnAnchor: Id=%s, Class: %s, Loc: %s"),
			*AnchorId, *SpawnClass->GetName(), *Transform.GetLocation().ToString());
	}

	return SpawnedActor;
}
