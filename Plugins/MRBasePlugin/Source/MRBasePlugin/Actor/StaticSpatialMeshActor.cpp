// Fill out your copyright notice in the Description page of Project Settings.

#include "StaticSpatialMeshActor.h"

#include "ProceduralMeshComponent.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"

namespace StaticSpatialMeshSerialization
{
	constexpr int32 Magic = 0x314D534D; // "MMS1"
	constexpr int32 Version = 1;
	constexpr int32 MaxMeshCount = 100000;
	constexpr int32 MaxVertexCount = 10000000;
	constexpr int32 MaxIndexCount = 30000000;
}

AStaticSpatialMeshActor::AStaticSpatialMeshActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}

void AStaticSpatialMeshActor::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoLoadOnBeginPlay && !AutoLoadFileName.IsEmpty())
	{
		LoadAndBuild(AutoLoadFileName);
	}
}

void AStaticSpatialMeshActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bBuilding)
	{
		SetActorTickEnabled(false);
		return;
	}

	const int32 NumToBuild = FMath::Min(MeshesPerTick, BuildQueue.Num() - BuildQueueIndex);
	for (int32 i = 0; i < NumToBuild; ++i)
	{
		CreateMeshComponentFor(BuildQueue[BuildQueueIndex + i]);
	}
	BuildQueueIndex += NumToBuild;

	if (BuildQueueIndex >= BuildQueue.Num())
	{
		bBuilding = false;
		BuildQueue.Reset();
		BuildQueueIndex = 0;
		SetActorTickEnabled(false);
		OnBuildFinishedDelegate.Broadcast(true, RebuiltMeshComponents.Num());
	}
}

bool AStaticSpatialMeshActor::BuildFromMeshInfos(const TArray<FPICOSpatialMeshInfo>& MeshInfos)
{
	ClearRebuiltMeshes();

	if (MeshInfos.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StaticSpatialMesh] BuildFromMeshInfos: empty MeshInfos"));
		OnBuildFinishedDelegate.Broadcast(false, 0);
		return false;
	}

	BuildQueue = MeshInfos;
	BuildQueue.RemoveAll([](const FPICOSpatialMeshInfo& Info)
	{
		return Info.Vertices.Num() < 3 || Info.Indices.Num() < 3 || Info.State == EPICOSpatialMeshState::Removed;
	});

	if (BuildQueue.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StaticSpatialMesh] BuildFromMeshInfos: no valid meshes after filtering"));
		OnBuildFinishedDelegate.Broadcast(false, 0);
		return false;
	}

	bBuilding = true;
	SetActorTickEnabled(true);
	return true;
}

bool AStaticSpatialMeshActor::LoadAndBuild(const FString& FileName)
{
	TArray<FPICOSpatialMeshInfo> MeshInfos;
	if (!LoadMeshInfosFromFile(FileName, MeshInfos))
	{
		OnBuildFinishedDelegate.Broadcast(false, 0);
		return false;
	}
	return BuildFromMeshInfos(MeshInfos);
}

void AStaticSpatialMeshActor::ClearRebuiltMeshes()
{
	for (UProceduralMeshComponent* MeshComp : RebuiltMeshComponents)
	{
		if (MeshComp)
		{
			MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			MeshComp->DestroyComponent();
		}
	}
	RebuiltMeshComponents.Reset();

	BuildQueue.Reset();
	BuildQueueIndex = 0;
	bBuilding = false;
	SetActorTickEnabled(false);
}

void AStaticSpatialMeshActor::SetRebuiltMeshesVisible(bool bVisible)
{
	bMeshVisible = bVisible;
	for (UProceduralMeshComponent* MeshComp : RebuiltMeshComponents)
	{
		if (MeshComp)
		{
			MeshComp->SetVisibility(bMeshVisible);
		}
	}
}

FString AStaticSpatialMeshActor::GetDefaultSaveDir()
{
	return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("SpatialMesh"));
}

FString AStaticSpatialMeshActor::ResolveMeshFilePath(const FString& FileName)
{
	if (FPaths::IsRelative(FileName))
	{
		FString WithExt = FileName;
		if (!WithExt.EndsWith(TEXT(".picomesh")))
		{
			WithExt += TEXT(".picomesh");
		}
		return FPaths::Combine(GetDefaultSaveDir(), WithExt);
	}
	return FileName;
}

bool AStaticSpatialMeshActor::SaveMeshInfosToFile(const TArray<FPICOSpatialMeshInfo>& MeshInfos, const FString& FileName)
{
	if (MeshInfos.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StaticSpatialMesh] SaveMeshInfosToFile: nothing to save"));
		return false;
	}

	const FString FilePath = ResolveMeshFilePath(FileName);
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(FilePath), true);

	FArchive* Ar = IFileManager::Get().CreateFileWriter(*FilePath);
	if (!Ar)
	{
		UE_LOG(LogTemp, Error, TEXT("[StaticSpatialMesh] SaveMeshInfosToFile: cannot open file %s"), *FilePath);
		return false;
	}

	int32 Magic = StaticSpatialMeshSerialization::Magic;
	int32 Version = StaticSpatialMeshSerialization::Version;
	int32 NumMeshes = MeshInfos.Num();
	*Ar << Magic << Version << NumMeshes;

	for (const FPICOSpatialMeshInfo& Info : MeshInfos)
	{
		FPICOSpatialUUID UUID = Info.UUID;
		*Ar << UUID;

		FTransform MeshPose = Info.MeshPose;
		*Ar << MeshPose;

		int32 NumVertices = Info.Vertices.Num();
		*Ar << NumVertices;
		for (const FVector& Vertex : Info.Vertices)
		{
			float X = static_cast<float>(Vertex.X);
			float Y = static_cast<float>(Vertex.Y);
			float Z = static_cast<float>(Vertex.Z);
			*Ar << X << Y << Z;
		}

		int32 NumIndices = Info.Indices.Num();
		*Ar << NumIndices;
		for (int32 Index : Info.Indices)
		{
			*Ar << Index;
		}

		int32 NumSemantics = Info.Semantics.Num();
		*Ar << NumSemantics;
		for (EPICOSemanticLabel Label : Info.Semantics)
		{
			uint8 LabelValue = static_cast<uint8>(Label);
			*Ar << LabelValue;
		}

		int64 UpdateTime = Info.UpdateTime;
		*Ar << UpdateTime;
	}

	const bool bSuccess = !Ar->IsError();
	Ar->Close();

	if (bSuccess)
	{
		UE_LOG(LogTemp, Log, TEXT("[StaticSpatialMesh] Saved %d meshes to %s"), NumMeshes, *FilePath);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[StaticSpatialMesh] SaveMeshInfosToFile: write error for %s"), *FilePath);
	}
	return bSuccess;
}

bool AStaticSpatialMeshActor::LoadMeshInfosFromFile(const FString& FileName, TArray<FPICOSpatialMeshInfo>& OutMeshInfos)
{
	OutMeshInfos.Reset();

	const FString FilePath = ResolveMeshFilePath(FileName);
	FArchive* Ar = IFileManager::Get().CreateFileReader(*FilePath);
	if (!Ar)
	{
		UE_LOG(LogTemp, Error, TEXT("[StaticSpatialMesh] LoadMeshInfosFromFile: cannot open file %s"), *FilePath);
		return false;
	}

	int32 Magic = 0;
	int32 Version = 0;
	int32 NumMeshes = 0;
	*Ar << Magic << Version << NumMeshes;

	if (Magic != StaticSpatialMeshSerialization::Magic || Version > StaticSpatialMeshSerialization::Version
		|| NumMeshes < 0 || NumMeshes > StaticSpatialMeshSerialization::MaxMeshCount)
	{
		UE_LOG(LogTemp, Error, TEXT("[StaticSpatialMesh] LoadMeshInfosFromFile: invalid header (Magic=%d Version=%d Num=%d) in %s"), Magic, Version, NumMeshes, *FilePath);
		Ar->Close();
		return false;
	}

	OutMeshInfos.Reserve(NumMeshes);
	bool bSuccess = true;

	for (int32 MeshIdx = 0; MeshIdx < NumMeshes && bSuccess; ++MeshIdx)
	{
		FPICOSpatialMeshInfo Info;
		*Ar << Info.UUID;
		*Ar << Info.MeshPose;

		int32 NumVertices = 0;
		*Ar << NumVertices;
		bSuccess = NumVertices >= 0 && NumVertices <= StaticSpatialMeshSerialization::MaxVertexCount;
		if (bSuccess)
		{
			Info.Vertices.SetNum(NumVertices);
			for (FVector& Vertex : Info.Vertices)
			{
				float X = 0.f;
				float Y = 0.f;
				float Z = 0.f;
				*Ar << X << Y << Z;
				Vertex = FVector(X, Y, Z);
			}
		}

		int32 NumIndices = 0;
		if (bSuccess)
		{
			*Ar << NumIndices;
			bSuccess = NumIndices >= 0 && NumIndices <= StaticSpatialMeshSerialization::MaxIndexCount;
		}
		if (bSuccess)
		{
			Info.Indices.SetNum(NumIndices);
			for (int32& Index : Info.Indices)
			{
				*Ar << Index;
			}
		}

		int32 NumSemantics = 0;
		if (bSuccess)
		{
			*Ar << NumSemantics;
			bSuccess = NumSemantics >= 0 && NumSemantics <= StaticSpatialMeshSerialization::MaxIndexCount;
		}
		if (bSuccess)
		{
			Info.Semantics.SetNum(NumSemantics);
			for (EPICOSemanticLabel& Label : Info.Semantics)
			{
				uint8 LabelValue = 0;
				*Ar << LabelValue;
				Label = static_cast<EPICOSemanticLabel>(LabelValue);
			}
		}

		if (bSuccess)
		{
			*Ar << Info.UpdateTime;
			Info.State = EPICOSpatialMeshState::Stable;
			OutMeshInfos.Add(MoveTemp(Info));
		}
	}

	bSuccess = bSuccess && !Ar->IsError();
	Ar->Close();

	if (!bSuccess)
	{
		UE_LOG(LogTemp, Error, TEXT("[StaticSpatialMesh] LoadMeshInfosFromFile: read error in %s"), *FilePath);
		OutMeshInfos.Reset();
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("[StaticSpatialMesh] Loaded %d meshes from %s"), OutMeshInfos.Num(), *FilePath);
	return true;
}

UProceduralMeshComponent* AStaticSpatialMeshActor::CreateMeshComponentFor(const FPICOSpatialMeshInfo& MeshInfo)
{
	UProceduralMeshComponent* MeshComp = NewObject<UProceduralMeshComponent>(this);
	MeshComp->SetMobility(EComponentMobility::Movable);
	MeshComp->bUseAsyncCooking = true;
	MeshComp->RegisterComponent();
	MeshComp->AttachToComponent(SceneRoot, FAttachmentTransformRules::KeepWorldTransform);
	MeshComp->SetWorldTransform(MeshInfo.MeshPose);
	MeshComp->SetVisibility(bMeshVisible);
	MeshComp->SetCollisionEnabled(bCreateCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	MeshComp->SetCollisionProfileName(CollisionProfileName);
	MeshComp->SetGenerateOverlapEvents(false);
	if (MeshMaterial)
	{
		MeshComp->SetMaterial(0, MeshMaterial);
	}
	AddOwnedComponent(MeshComp);

	TArray<FVector> EmptyNormals;
	TArray<FVector2D> EmptyUVs;
	TArray<FProcMeshTangent> EmptyTangents;
	TArray<FLinearColor> VertexColors;
	BuildVertexColors(MeshInfo, VertexColors);

	MeshComp->CreateMeshSection_LinearColor(0, MeshInfo.Vertices, MeshInfo.Indices, EmptyNormals, EmptyUVs, VertexColors, EmptyTangents, bCreateCollision);

	RebuiltMeshComponents.Add(MeshComp);
	return MeshComp;
}

void AStaticSpatialMeshActor::BuildVertexColors(const FPICOSpatialMeshInfo& MeshInfo, TArray<FLinearColor>& OutVertexColors) const
{
	const int32 NumVertices = MeshInfo.Vertices.Num();
	const int32 NumSemantics = MeshInfo.Semantics.Num();

	// Vertex-aligned semantics: one label per vertex.
	if (NumSemantics > 0 && NumSemantics == NumVertices)
	{
		OutVertexColors.Reserve(NumSemantics);
		for (EPICOSemanticLabel Label : MeshInfo.Semantics)
		{
			OutVertexColors.Add(GetColorBySemanticLabel(Label));
		}
	}
	// Triangle-aligned semantics cannot be mapped onto shared vertices without duplicating
	// triangles, so colors are skipped in that case (they are debug-only anyway).
}

FLinearColor AStaticSpatialMeshActor::GetColorBySemanticLabel(EPICOSemanticLabel Label) const
{
	if (const FLinearColor* Found = SemanticToColors.Find(Label))
	{
		return *Found;
	}
	return FLinearColor::FromSRGBColor(FColor::MakeRandomSeededColor(static_cast<int32>(Label)));
}
