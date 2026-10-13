#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MRGameMode.generated.h"

UCLASS(Blueprintable, BlueprintType)
class MRBASEPLUGIN_API AMRGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMRGameMode(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MR|Pawn")
	TSubclassOf<APawn> MRPawnClassOverride;
};