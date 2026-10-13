// Copyright 2023 PICO Inc. All Rights Reserved.

#pragma once

#include "PICO_MovementFunctionLibrary.h"
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "PICO_FaceTrackingComponent.generated.h"

struct PICOOPENXRMOVEMENT_API FMorphTargetsManagerPICO
{
public:
	void ResetMeshMorphTargetCurves(USkinnedMeshComponent* TargetMeshComponent);
	void UpdateMeshMorphTargets(USkinnedMeshComponent* TargetMeshComponent);
	void SetMeshMorphTargetValue(FName MorphTargetName, float Value);
	float GetMeshMorphTargetValue(FName MorphTargetName) const;
	void EmptyMorphTargets();
	TMap<FName, float> MeshMorphTargetCurves;
};

UENUM(BlueprintType)
enum class EFaceBlendShapePICO : uint8
{
	// Left eye blend shapes
	EyeBlinkLeft = 0,
	EyeLookDownLeft = 1,
	EyeLookInLeft = 2,
	EyeLookOutLeft = 3,
	EyeLookUpLeft = 4,
	EyeSquintLeft = 5,
	EyeWideLeft = 6,
	// Right eye blend shapes
	EyeBlinkRight = 7,
	EyeLookDownRight = 8,
	EyeLookInRight = 9,
	EyeLookOutRight = 10,
	EyeLookUpRight = 11,
	EyeSquintRight = 12,
	EyeWideRight = 13,
	// Jaw blend shapes
	JawForward = 14,
	JawLeft = 15,
	JawRight = 16,
	JawOpen = 17,
	// Mouth blend shapes
	MouthClose = 18,
	MouthFunnel = 19,
	MouthPucker = 20,
	MouthLeft = 21,
	MouthRight = 22,
	MouthSmileLeft = 23,
	MouthSmileRight = 24,
	MouthFrownLeft = 25,
	MouthFrownRight = 26,
	MouthDimpleLeft = 27,
	MouthDimpleRight = 28,
	MouthStretchLeft = 29,
	MouthStretchRight = 30,
	MouthRollLower = 31,
	MouthRollUpper = 32,
	MouthShrugLower = 33,
	MouthShrugUpper = 34,
	MouthPressLeft = 35,
	MouthPressRight = 36,
	MouthLowerDownLeft = 37,
	MouthLowerDownRight = 38,
	MouthUpperUpLeft = 39,
	MouthUpperUpRight = 40,
	// Brow blend shapes
	BrowDownLeft = 41,
	BrowDownRight = 42,
	BrowInnerUp = 43,
	BrowOuterUpLeft = 44,
	BrowOuterUpRight = 45,
	// Cheek blend shapes
	CheekPuff = 46,
	CheekSquintLeft = 47,
	CheekSquintRight = 48,
	// Nose blend shapes
	NoseSneerLeft = 49,
	NoseSneerRight = 50,
	TongueOut = 51,
	//LIPSYNC
	PP = 52,
	CH = 53,
	o = 54,
	O = 55,
	I = 56,
	u = 57,
	RR = 58,
	XX = 59,
	aa = 60,
	i = 61,
	FF = 62,
	U = 63,
	TH = 64,
	kk = 65,
	SS = 66,
	e = 67,
	DD = 68,
	E = 69,
	nn = 70,
	sil = 71,
	COUNT = 72,
};

UCLASS(Blueprintable, ClassGroup = (PICO), meta = (BlueprintSpawnableComponent, DisplayName = "Face Tracking Component PICO"))
class PICOOPENXRMOVEMENT_API UFaceTrackingComponentPICO : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UFaceTrackingComponentPICO();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "PICO|FaceTracking", meta = (UnsafeDuringActorConstruction = "true"))
		void SetBlendShapeValue(EFaceBlendShapePICO BlendShape, float Value);

	UFUNCTION(BlueprintCallable, Category = "PICO|FaceTracking")
		float GetBlendShapeValue(EFaceBlendShapePICO BlendShape) const;

	UFUNCTION(BlueprintCallable, Category = "PICO|FaceTracking")
		void ClearBlendShapeValues();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PICO|FaceTracking")
		FName FTTargetMeshComponentName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PICO|FaceTracking")
		float InvalidFaceDataResetTime;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PICO|FaceTracking")
		TMap<EFaceBlendShapePICO, FName> BlendShapeNameMapping;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PICO|FaceTracking")
		bool bUpdateFaceTracking;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PICO|FaceTracking")
		EFaceTrackingModePICO Mode;
private:
	bool InitializeFaceTracking();

	UPROPERTY()
		USkinnedMeshComponent* FTTargetMeshComponent;

	TStaticArray<bool, static_cast<uint32>(EFaceBlendShapePICO::COUNT)> ValidBlendShape;

	FMorphTargetsManagerPICO MorphTargetsManager;

	float InvalidFaceDataTimer;

	static int FTComponentCount;

};