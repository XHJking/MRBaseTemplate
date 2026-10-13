#include "MRPawn.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "PXR_HMDFunctionLibrary.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "MotionControllerComponent.h"

AMRPawn::AMRPawn(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AutoPossessPlayer = EAutoReceiveInput::Player0;
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("MRRoot"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("MRCamera"));
	CameraComp->SetupAttachment(RootComponent);
	CameraComp->SetAutoActivate(true);

	MotionController_L = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("MotionController_L"));
	MotionController_L->SetupAttachment(RootComponent);

	MotionController_R = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("MotionController_R"));
	MotionController_R->SetupAttachment(RootComponent);
}

void AMRPawn::BeginPlay()
{
	Super::BeginPlay();

	// PICOXR needs HMD/stereo to be explicitly enabled for the engine to actually
	// drive HMD tracking + render into the XR swapchain. Without this, the scene
	// will not be composited with the VST image and you'll only see the bare
	// see-through video without any in-level content.
	
}