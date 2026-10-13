#include "MRHandInteractionPawn.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Components/WidgetInteractionComponent.h"
#include "Components/WidgetComponent.h"
#include "PXR_InputFunctionLibrary.h"
#include "InputCoreTypes.h"
#include "Engine/World.h"
#include "CollisionShape.h"
#include "CollisionQueryParams.h"

AMRHandInteractionPawn::AMRHandInteractionPawn(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AutoPossessPlayer = EAutoReceiveInput::Player0;
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("MRRoot"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("MRCamera"));
	CameraComp->SetupAttachment(RootComponent);
	CameraComp->SetAutoActivate(true);

	LeftInteraction = CreateDefaultSubobject<UWidgetInteractionComponent>(TEXT("LeftWidgetInteraction"));
	LeftInteraction->SetupAttachment(RootComponent);
	LeftInteraction->InteractionSource = EWidgetInteractionSource::Custom;
	LeftInteraction->bShowDebug = true;
	LeftInteraction->bEnableHitTesting = true;
	LeftInteraction->bAutoActivate = true;

	RightInteraction = CreateDefaultSubobject<UWidgetInteractionComponent>(TEXT("RightWidgetInteraction"));
	RightInteraction->SetupAttachment(RootComponent);
	RightInteraction->InteractionSource = EWidgetInteractionSource::Custom;
	RightInteraction->bShowDebug = true;
	RightInteraction->bEnableHitTesting = true;
	RightInteraction->bAutoActivate = true;

	TestWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("TestWidget"));
	TestWidget->SetupAttachment(RootComponent);
	TestWidget->SetVisibility(false);
	TestWidget->SetWidgetSpace(EWidgetSpace::World);
	TestWidget->SetDrawAtDesiredSize(true);
	TestWidget->SetTwoSided(true);
	TestWidget->SetTickMode(ETickMode::Automatic);
	TestWidget->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	PokeSphereRadius = 1.5f;
	PokeForwardDistance = 2.0f;
	TestWidgetSpawnDistance = 100.f;
}

void AMRHandInteractionPawn::BeginPlay()
{
	Super::BeginPlay();

	if (TestWidgetClass)
	{
		TestWidget->SetWidgetClass(TestWidgetClass);
		TestWidget->SetVisibility(true);

		const FVector CamLoc = CameraComp->GetComponentLocation();
		const FQuat CamQuat = CameraComp->GetComponentQuat();
		const FVector SpawnLoc = CamLoc + CamQuat.GetForwardVector() * TestWidgetSpawnDistance;
		TestWidget->SetWorldLocationAndRotation(SpawnLoc, CamQuat.Rotator());
	}
}

void AMRHandInteractionPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateHandInteraction(true);
	UpdateHandInteraction(false);
}

void AMRHandInteractionPawn::UpdateHandInteraction(bool bLeftHand)
{
	const EPICOXRHandType HandType = bLeftHand ? EPICOXRHandType::HandLeft : EPICOXRHandType::HandRight;
	UWidgetInteractionComponent* Interaction = bLeftHand ? LeftInteraction : RightInteraction;
	bool& bWasTouching = bLeftHand ? bWasLeftTouching : bWasRightTouching;

	if (!UPICOXRInputFunctionLibrary::IsHandTrackingEnabled())
	{
		return;
	}

	if (!UPICOXRInputFunctionLibrary::IsComputed(HandType))
	{
		if (bWasTouching)
		{
			Interaction->ReleasePointerKey(EKeys::LeftMouseButton);
			bWasTouching = false;
		}
		return;
	}

	// 食指指尖位置（戳触指针头）
	const FVector IndexTipLoc = UPICOXRInputFunctionLibrary::GetBoneLocation(HandType, EPICOXRHandJoint::IndexTip);
	// 食指中节位置，推算指尖朝向（指尖指向）
	const FVector IndexDistalLoc = UPICOXRInputFunctionLibrary::GetBoneLocation(HandType, EPICOXRHandJoint::IndexDistal);
	const FVector IndexDir = (IndexTipLoc - IndexDistalLoc).GetSafeNormal();

	// 沿指尖方向做短距离球形扫描，命中 UWidgetComponent 即视为"戳到"
	const FVector Start = IndexTipLoc;
	const FVector End = IndexTipLoc + IndexDir * PokeForwardDistance;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	Params.bReturnPhysicalMaterial = false;

	FHitResult Hit;
	GetWorld()->SweepSingleByChannel(
		Hit,
		Start,
		End,
		FQuat::Identity,
		ECC_Visibility,
		FCollisionShape::MakeSphere(PokeSphereRadius),
		Params
	);

	// 判断是否命中可见的 WidgetComponent（UWidgetComponent 自身就是 PrimitiveComponent）
	bool bTouchingWidget = false;
	if (Hit.bBlockingHit && Hit.GetComponent() != nullptr)
	{
		if (UWidgetComponent* HitWidget = Cast<UWidgetComponent>(Hit.GetComponent()))
		{
			if (HitWidget->IsWidgetVisible())
			{
				bTouchingWidget = true;
				Interaction->SetCustomHitResult(Hit);
			}
		}
	}

	if (!bTouchingWidget)
	{
		FHitResult EmptyHit;
		Interaction->SetCustomHitResult(EmptyHit);
	}

	// 接触边沿触发：碰到=按下，离开=抬起
	if (bTouchingWidget && !bWasTouching)
	{
		Interaction->PressPointerKey(EKeys::LeftMouseButton);
	}
	else if (!bTouchingWidget && bWasTouching)
	{
		Interaction->ReleasePointerKey(EKeys::LeftMouseButton);
	}
	bWasTouching = bTouchingWidget;
}
