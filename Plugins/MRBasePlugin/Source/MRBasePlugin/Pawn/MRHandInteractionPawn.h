#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "MRHandInteractionPawn.generated.h"

class UCameraComponent;
class UWidgetInteractionComponent;
class UWidgetComponent;
class UUserWidget;
class USceneComponent;

/**
 * 基于 PICOXR 手势的近场戳触 Pawn（最小可用版本）。
 *
 * 不使用捏合，而是用食指指尖直接戳触 3D Widget 表面：
 *   - 每帧从食指指尖沿指尖朝向做短距离球形扫描
 *   - 命中 UWidgetComponent 即视为"戳到"，自动触发鼠标按下
 *   - 指尖离开 Widget 表面自动抬起，Slider/按钮由此接管
 *
 * 使用方式：
 *   1. 把本 Pawn 设为 GameMode 的 DefaultPawnClass（或派生蓝图后使用）。
 *   2.（可选）在 TestWidgetClass 指定 UMG 蓝图，BeginPlay 会自动在眼前生成用于测试。
 *   3. PICO 4U 系统设置中开启"手部追踪"。
 *   4. 运行后用食指指尖去触碰 3D Widget 按钮或 Slider 即可。
 */
UCLASS(Blueprintable, BlueprintType)
class MRBASEPLUGIN_API AMRHandInteractionPawn : public APawn
{
	GENERATED_BODY()

public:
	AMRHandInteractionPawn(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MR|HandInteraction")
	UCameraComponent* CameraComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MR|HandInteraction")
	UWidgetInteractionComponent* LeftInteraction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MR|HandInteraction")
	UWidgetInteractionComponent* RightInteraction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MR|HandInteraction")
	UWidgetComponent* TestWidget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MR|HandInteraction")
	TSubclassOf<UUserWidget> TestWidgetClass;

	/** 指尖球形扫描半径（cm），模拟指尖实际接触面积 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MR|HandInteraction", meta = (ClampMin = "0.1", UIMin = "0.1"))
	float PokeSphereRadius;

	/** 指尖前方扫描距离（cm），用于判定"戳到"。建议 1~3cm，过大会变成远场 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MR|HandInteraction", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float PokeForwardDistance;

	/** 自测 Widget 距摄像机正前方的距离（cm） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MR|HandInteraction", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float TestWidgetSpawnDistance;

private:
	bool bWasLeftTouching = false;
	bool bWasRightTouching = false;

	void UpdateHandInteraction(bool bLeftHand);
};
