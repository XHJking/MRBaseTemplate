#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "MRPawn.generated.h"

class UMotionControllerComponent;
class USceneComponent;
class UCameraComponent;

UCLASS(Blueprintable, BlueprintType)
class MRBASEPLUGIN_API AMRPawn : public APawn
{
	GENERATED_BODY()

public:
	AMRPawn(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay() override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MR")
	UCameraComponent* CameraComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MR")
	UMotionControllerComponent* MotionController_L;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MR")
	UMotionControllerComponent* MotionController_R;
};