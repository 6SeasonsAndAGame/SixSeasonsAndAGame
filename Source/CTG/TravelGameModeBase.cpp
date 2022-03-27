// None


#include "TravelGameModeBase.h"


 void ATravelGameModeBase::MyServerTravel(FString mapName, FString gameMode, bool bAbsolute, FString additionalOptions)
 {
 //Default paths to Maps folder and GameModes folder
 FString mapsPath = TEXT("/Game/Maps/");
 FString gameModePath = TEXT("/Game/GameModes/");
 
 //Construction of the GameMode option change
 FString gameModeOption = TEXT("?game=");
 FString gameModeClass = gameMode + TEXT(".") + gameMode + TEXT("_C");
 
 //if GameMode is not set, blank out the variables used to make the game mode option
 if (gameMode.IsEmpty())
 {
     gameModeOption.Empty();
     gameModeClass.Empty();
     gameModePath.Empty();
 }
 
 //A travel URL is built starting with the map name and append any options
 // i.e.: /pathtomaps/map?option=value?option2=value2 ... etc
 FString travelURL = mapsPath + mapName + gameModeOption + gameModePath + gameModeClass + additionalOptions;
 
 ProcessServerTravel(travelURL, bAbsolute);
 }
 