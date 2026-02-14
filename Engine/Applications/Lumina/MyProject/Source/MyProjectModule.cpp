
#include "MyProjectModule.h"
#include "Core/Module/ModuleManager.h"
#include "Log/Log.h"

using namespace Lumina;

IMPLEMENT_MODULE(FMyProjectModule, "MyProject");

void FMyProjectModule::StartupModule()
{
    LOG_INFO("MyProject Startup Module");
}

void FMyProjectModule::ShutdownModule()
{
    LOG_INFO("MyProject Shutdown Module");

}
