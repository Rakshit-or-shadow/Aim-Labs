#include "GameEngine.h"
#include "Modules/ModuleManager.h"

// Pulled in here so the ray-sphere hit math is compiled (and checked) with every build,
// even before gameplay code starts using it.
#include "RayTraceMath.h"

IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, GameEngine, "GameEngine");

DEFINE_LOG_CATEGORY(LogGameEngine);
