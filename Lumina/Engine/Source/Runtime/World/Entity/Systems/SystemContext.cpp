#include "pch.h"
#include "SystemContext.h"
#include "sol/sol.hpp"
#include "World/World.h"
#include "World/Entity/Components/DirtyComponent.h"
#include "World/Entity/Components/LuaComponent.h"
#include "world/entity/components/namecomponent.h"

namespace Lumina
{
    FSystemContext::FSystemContext(CWorld* InWorld)
        : Registry(InWorld->EntityRegistry)
        , Dispatcher(InWorld->SingletonDispatcher)
    {
        
    }
    
    FSystemContext::~FSystemContext()
    {
        
    }

    void FSystemContext::RegisterWithLua(sol::state& Lua)
    {
        sol::usertype<FSystemContext> ContextType = Lua.new_usertype<FSystemContext>("SystemContext", sol::no_constructor);
        
        ContextType["GetDeltaTime"]     = &FSystemContext::GetDeltaTime;
        ContextType["GetTime"]          = &FSystemContext::GetTime;
        ContextType["GetUpdateStage"]   = &FSystemContext::GetUpdateStage;
        ContextType["GetTransform"]     = &FSystemContext::GetEntityTransform;
        ContextType["View"]             = &FSystemContext::MakeLuaView;
        ContextType["BindEvent"]        = &FSystemContext::BindLuaEvent;
        ContextType["DirtyTransform"]   = &FSystemContext::MarkEntityTransformDirty;
        ContextType["Emplace"]          = &FSystemContext::LuaEmplace;
        ContextType["SetActiveCamera"]  = &FSystemContext::LuaSetActiveCamera;
        
    }


    STransformComponent& FSystemContext::GetEntityTransform(uint32 Entity)
    {
        return Get<STransformComponent>((entt::entity)Entity);
    }

    void FSystemContext::MarkEntityTransformDirty(uint32 Entity)
    {
        EmplaceOrReplace<FNeedsTransformUpdate>((entt::entity)(Entity));  
    }

    entt::entity FSystemContext::Create(const FName& Name, const FTransform& Transform) const
    {
        entt::entity EntityID = Registry.create();
        Registry.emplace<STransformComponent>(EntityID, Transform);
        Registry.emplace<SNameComponent>(EntityID, Name);
        Registry.emplace_or_replace<FNeedsTransformUpdate>(EntityID);
        return EntityID;
    }

    void FSystemContext::LuaSetActiveCamera(uint32 Entity)
    {
        Dispatcher.trigger<FSwitchActiveCameraEvent>(FSwitchActiveCameraEvent{(entt::entity)Entity});
    }

    sol::table FSystemContext::LuaEmplace(uint32 Entity, sol::table Type)
    {
        const char* ComponentName = Type["__type"].get<const char*>();
        entt::hashed_string HashedString = entt::hashed_string(ComponentName);
            
        // This type is resolvable, so we use the emplacement function there.
        if (entt::meta_type Meta = entt::resolve(HashedString))
        {
            using namespace entt::literals;
            entt::meta_any NewComponent = Meta.invoke("addcomponent"_hs, {}, (entt::entity)Entity, entt::forward_as_meta(Registry));
            void* ComponentType = *NewComponent.try_cast<void*>();
                
            return Scripting::FScriptingContext::Get().ConvertToSolObjectFromName(FName(ComponentName), Type.lua_state(), ComponentType);
        }

        // This type is not native resolvable, so we're assuming the user created a new lua type.
            
        SLuaComponent& NewComponent = Registry.storage<SLuaComponent>(HashedString).emplace((entt::entity)Entity);
            
        NewComponent.TypeName = ComponentName;
        NewComponent.LuaTable = Type;

        return NewComponent.LuaTable;
    }

    sol::table FSystemContext::MakeLuaView(sol::variadic_args Types)
    {
        LUMINA_PROFILE_SECTION("FSystemContext::ScriptView");
        using namespace entt::literals;

        entt::runtime_view RuntimeView;

        struct FComponentInfo
        {
            FName Name;
            entt::basic_sparse_set<>* Storage;
            Scripting::FLuaConverter::ToFunctionType* ConversionFunctionPtr;
        };
        
        TFixedVector<FComponentInfo, 4> Storages;
        

        for (auto Arg : Types)
        {
            if (Arg.is<sol::table>())
            {
                sol::table ComponentTable = Arg.as<sol::table>();
                if (!ComponentTable["__type"].valid())
                {
                    LOG_WARN("No __type found for component!");
                    continue;
                }
                
                FName LuaName = ComponentTable["__type"].get<const char*>();

                auto HashedComponentName = entt::hashed_string(LuaName.c_str());
                entt::meta_type Meta = entt::resolve(HashedComponentName);
            
                if (entt::basic_sparse_set<>* Storage = Registry.storage(Meta.info().hash()))
                {
                    Scripting::FLuaConverter::ToFunctionType* FunctionPtr = Scripting::FScriptingContext::Get().GetSolConverterFunctionPtrByName(LuaName);
                    Storages.emplace_back(FComponentInfo{ LuaName, Storage, FunctionPtr });
                    RuntimeView.iterate(*Storage);
                }
            }
            else if (Arg.is<const char*>())
            {
                FName LuaName = Arg.get<const char*>();
                uint32 HashedComponentName = entt::hashed_string(LuaName.c_str());
                if (entt::basic_sparse_set<>* Storage = Registry.storage(HashedComponentName))
                {
                    Scripting::FLuaConverter::ToFunctionType* FunctionPtr = Scripting::FScriptingContext::Get().GetSolConverterFunctionPtrByName("STagComponent");
                    Storages.emplace_back(FComponentInfo{ LuaName, Storage, FunctionPtr });
                    RuntimeView.iterate(*Storage);
                }
            }
        }
        
        sol::state_view StateView(Types.lua_state());

        int EstimatedSize = (int)RuntimeView.size_hint();
        sol::table ViewTable = StateView.create_table(EstimatedSize);

        uint32 NumEntities = 0;
        int TableSize = 1 + (int)Storages.size();
        RuntimeView.each([&] (entt::entity EntityID)
        {
            NumEntities++;

            sol::table Row = StateView.create_table(0, TableSize);
            Row["Entity"] = (int)EntityID;
            
            for (FComponentInfo& Info : Storages)
            {
                Row[Info.Name.c_str()] = Info.ConversionFunctionPtr(StateView, Info.Storage->value(EntityID));
            }
            
            ViewTable[(uint32)EntityID] = Row;
        });

        sol::table meta = StateView.create_table();
        meta[sol::meta_function::length] = [NumEntities](sol::table) { return NumEntities; };
        ViewTable[sol::metatable_key] = meta;
        
        return ViewTable;
    }
    

    void FSystemContext::BindLuaEvent(sol::table Table, sol::function Function)
    {
        
    }
}
