#include "SandboxModule.h"

#include "Core/Module/ModuleManager.h"
#include "Log/Log.h"

using namespace Lumina;

IMPLEMENT_MODULE(FSandboxModule, "SandboxModule");

void FSandboxModule::StartupModule()
{
    LOG_INFO("Sandbox Startup Module");
}

void FSandboxModule::ShutdownModule()
{
    LOG_INFO("Sandbox Shutdown Module");

}
