// Copyright Epic Games, Inc. All Rights Reserved.

#include "NemesisGameMode.h"
#include "NemesisCharacter.h"
#include "UObject/ConstructorHelpers.h"

#include "NemesisCharacter.h"
#include "NemesisPlayerController.h"

ANemesisGameMode::ANemesisGameMode()
{
	// set default pawn class to our Blueprinted character
	//static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPersonCPP/Blueprints/ThirdPersonCharacter"));
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ParagonAurora/Characters/Heroes/Aurora/AuroraPlayerCharacter"));

	DefaultPawnClass = ANemesisCharacter::StaticClass();
	PlayerControllerClass = ANemesisPlayerController::StaticClass();

	if (PlayerPawnBPClass.Class != NULL)
	{
		//DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
