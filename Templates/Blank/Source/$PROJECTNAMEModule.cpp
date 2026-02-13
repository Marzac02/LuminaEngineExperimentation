
#include "$PROJECTNAMEModule.h"
#include "Core/Module/ModuleManager.h"
#include "Log/Log.h"

using namespace Lumina;

IMPLEMENT_MODULE(F$PROJECTNAMEModule, "$PROJECTNAME");

void F$PROJECTNAMEModule::StartupModule()
{
    LOG_INFO("$PROJECTNAME Startup Module");
}

void F$PROJECTNAMEModule::ShutdownModule()
{
    LOG_INFO("$PROJECTNAME Shutdown Module");

}
