#include "MRGameMode.h"
#include "../Pawn/MRPawn.h"
#include "UObject/UObjectGlobals.h"

AMRGameMode::AMRGameMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	if (MRPawnClassOverride.Get() != nullptr)
	{
		DefaultPawnClass = MRPawnClassOverride;
	}
	else
	{
		DefaultPawnClass = AMRPawn::StaticClass();
	}

	PlayerControllerClass = APlayerController::StaticClass();
}