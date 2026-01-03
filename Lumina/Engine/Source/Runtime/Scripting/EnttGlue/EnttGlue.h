#pragma once

namespace Lumina::Scripting::Glue
{
    void RegisterRegistry(sol::table EnttModule);
    void RegisterRuntimeView(sol::table EnttModule);
}