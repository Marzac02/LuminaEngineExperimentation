#include "pch.h"
#include "Scripting.h"

#include <glm/gtx/string_cast.hpp>
#include "Containers/Name.h"
#include "Core/Math/Color.h"
#include "Events/KeyCodes.h"
#include "Input/InputProcessor.h"
#include "Memory/SmartPtr.h"
#include "Paths/Paths.h"
#include "Scripting/DeferredScriptRegistry.h"
#include "Scripting/ScriptFactory.h"
#include "World/Entity/Components/TagComponent.h"
#include "World/Entity/Systems/SystemContext.h"

namespace Lumina::Scripting
{
    static TUniquePtr<FScriptingContext> GScriptingContext;
    
    int SolExceptionHandler(lua_State* L, sol::optional<const std::exception&> MaybeException, sol::string_view Desc) 
    {
        // L is the lua state, which you can wrap in a state_view if necessary
        // maybe_exception will contain exception, if it exists
        // description will either be the what() of the exception or a description saying that we hit the general-case catch(...)
        FFixedString ErrorString;
        if (MaybeException) 
        {
            const std::exception& ex = *MaybeException;
            ErrorString = ex.what();
        }
        else 
        {
            ErrorString = FFixedString(Desc.data(), Desc.length());
        }
        
        LOG_ERROR("An exception occured in a script function {0}", ErrorString.c_str());


        // you must push 1 element onto the stack to be
        // transported through as the error object in Lua
        // note that Lua -- and 99.5% of all Lua users and libraries -- expects a string
        // so we push a single string (in our case, the description of the error)
        return sol::stack::push(L, Desc);
    }
    
    inline void SolPanicHandler(sol::optional<std::string> MaybeMsg) 
    {
        LOG_ERROR("Lua is in a panic state and will now abort() the application");
        if (MaybeMsg) 
        {
            LOG_ERROR("Error Message: {0}", MaybeMsg.value());
        }
        // When this function exits, Lua will exhibit default behavior and abort()
    }
    void Initialize()
    {
        GScriptingContext = MakeUniquePtr<FScriptingContext>();
        GScriptingContext->Initialize();
    }

    void Shutdown()
    {
        GScriptingContext->Shutdown();
        GScriptingContext.reset();
    }

    FScriptingContext& FScriptingContext::Get()
    {
        return *GScriptingContext.get();
    }

    void FScriptingContext::Initialize()
    {
        State.set_exception_handler(&SolExceptionHandler);
        State.set_panic(sol::c_call<decltype(&SolPanicHandler), &SolPanicHandler>);
        State.open_libraries(
            sol::lib::base,
            sol::lib::package,
            sol::lib::coroutine,
            sol::lib::string,
            sol::lib::math,
            sol::lib::table,
            sol::lib::io);

        FDeferredScriptRegistry::Get().ProcessRegistrations(sol::state_view(State));
        RegisterCoreTypes();
        SetupInput();
    }

    void FScriptingContext::Shutdown()
    {
        ScriptRegistry = {};
    }

    void FScriptingContext::ProcessDeferredActions()
    {
        DeferredActions.ProcessAllOf<FScriptReload>([&](const FScriptReload& Reload)
        {
            LoadScriptPath(Reload.Path);
        });
    }

    SIZE_T FScriptingContext::GetScriptMemoryUsage() const
    {
        return State.memory_used();
    }

    void FScriptingContext::OnScriptReloaded(FStringView ScriptPath)
    {
        FScopeLock Lock(LoadMutex);
        
        DeferredActions.EnqueueAction<FScriptReload>(FString(ScriptPath));
    }

    void FScriptingContext::OnScriptCreated(FStringView ScriptPath)
    {
        FScopeLock Lock(LoadMutex);

    }

    void FScriptingContext::OnScriptRenamed(FStringView NewPath, FStringView OldPath)
    {
        FScopeLock Lock(LoadMutex);
        

    }

    void FScriptingContext::OnScriptDeleted(FStringView ScriptPath)
    {
        FScopeLock Lock(LoadMutex);
        
    }
    
    void FScriptingContext::LoadScriptsInDirectoryRecursively(FStringView Directory)
    {
        FScopeLock Lock(LoadMutex);
        
        ScriptRegistry = {};
        for (auto& Itr : std::filesystem::recursive_directory_iterator(Directory.data()))
        {
            if (Itr.is_directory())
            {
                continue;
            }

            if (Itr.path().extension() == ".lua")
            {
                FString ScriptPath = Itr.path().generic_string().c_str();
                FString ScriptDirectory = Itr.path().parent_path().generic_string().c_str();
                Paths::NormalizePath(ScriptDirectory);

                LoadScriptPath(ScriptPath);
            }
        }
    }

    void FScriptingContext::RegisterCoreTypes()
    {
        
        State.set_function("LoadObject", [](const sol::object& Name)
        {
            CObject* Object = LoadObject<CObject>(Name.as<const char*>());
            return Object ? Object->AsLua(Name.lua_state()) : sol::nil;
        });
        
        State.new_usertype<FString>("FString",
	        sol::constructors<sol::types<>, sol::types<const char*>>(),
		    "size", [](const FString& Self) { return Self.size(); },
		    "trim", [](FString& Self) { return Self.trim(); },
	        sol::meta_function::to_string, [](const FString &s) { return s.data(); },
	        sol::meta_function::length, &FString::size,
	        sol::meta_function::index, [](const FString &s, size_t i) { return s.at(i - 1); },
	        "append", sol::overload(
			    [](FString &self, const FString &other) { self.append(other); },
	            [](FString &self, const char *suffix) { self.append(suffix); }
		    )
	    );

        State.set_function("System", [](const sol::table& Descriptor)
        {
            sol::state_view lua(Descriptor.lua_state());
            
            return lua.create_table_with(
                "Type",     "System",
                "Name",     Descriptor.get_or("Name", std::string("UnnamedSystem")),
                "Stage",    Descriptor.get_or("Stage", 0),
                "Priority", Descriptor.get_or("Priority", 0),
                "Init",     Descriptor["Init"],
                "Execute",  Descriptor["Execute"],
                "Shutdown", Descriptor["Shutdown"],
                "Query",    Descriptor.get_or("Query", lua.create_table()));
        });
        

        State.set_function("Metadata", [](const sol::table& Descriptor)
        {
            sol::state_view lua(Descriptor.lua_state());
            
            return lua.create_table_with(
                "Type", "Metadata",
                "Name", Descriptor.get_or("Name", std::string("")),
                "Author", Descriptor.get_or("Author", std::string("")),
                "Version", Descriptor.get_or("Version", std::string("1.0.0")),
                "Description", Descriptor.get_or("Description", std::string(""))
            );
        });

        State.new_usertype<entt::entity>("Entity",
        sol::meta_function::to_string, [](const entt::entity& e)
        {
            return std::to_string(static_cast<uint32>(e));
        });
        
        State.new_usertype<FGuid>("FGuid",
            sol::call_constructor,
            sol::constructors<FGuid()>(),
            "IsValid", &FGuid::IsValid,
            "Hash", &FGuid::Hash,
            "New", &FGuid::New,
            "Empty", &FGuid::Empty,
            sol::meta_function::equal_to, &FGuid::operator==,
            sol::meta_function::to_string, [](const FGuid& Guid) { return Guid.ToString(); }
            );

        // vec2
        State.new_usertype<glm::vec2>("vec2",
            sol::call_constructor,
            sol::constructors<glm::vec2(), glm::vec2(float), glm::vec2(float, float)>(),
            "x", &glm::vec2::x,
            "y", &glm::vec2::y,
            
            sol::meta_function::addition, sol::overload(
                [](const glm::vec2& a, const glm::vec2& b){ return a + b; },
                [](const glm::vec2& a, float s){ return a + s; }
            ),
            sol::meta_function::subtraction, sol::overload(
                [](const glm::vec2& a, const glm::vec2& b){ return a - b; },
                [](const glm::vec2& a, float s){ return a - s; }
            ),
            sol::meta_function::multiplication, sol::overload(
                [](const glm::vec2& a, const glm::vec2& b){ return a * b; },
                [](const glm::vec2& a, float s){ return a * s; },
                [](float s, const glm::vec2& a){ return s * a; }
            ),
            sol::meta_function::division, sol::overload(
                [](const glm::vec2& a, const glm::vec2& b){ return a / b; },
                [](const glm::vec2& a, float s){ return a / s; }
            ),
            sol::meta_function::unary_minus, [](const glm::vec2& a){ return -a; },
            sol::meta_function::equal_to, [](const glm::vec2& a, const glm::vec2& b){ return a == b; },
            sol::meta_function::to_string, [](const glm::vec2& v)
            { 
                return glm::to_string(v);
            },
            
            "Length", [](const glm::vec2& v){ return glm::length(v); },
            "Normalize", [](const glm::vec2& v){ return glm::normalize(v); },
            "Dot", [](const glm::vec2& a, const glm::vec2& b){ return glm::dot(a, b); },
            "Distance", [](const glm::vec2& a, const glm::vec2& b){ return glm::distance(a, b); }
        );
        
        // vec3
        State.new_usertype<glm::vec3>("vec3",
            sol::call_constructor,
            sol::constructors<glm::vec3(), glm::vec3(float), glm::vec3(float, float, float)>(),
            "x", &glm::vec3::x,
            "y", &glm::vec3::y,
            "z", &glm::vec3::z,
            
            sol::meta_function::addition, sol::overload(
                [](const glm::vec3& a, const glm::vec3& b){ return a + b; },
                [](const glm::vec3& a, float s){ return a + s; }
            ),
            sol::meta_function::subtraction, sol::overload(
                [](const glm::vec3& a, const glm::vec3& b){ return a - b; },
                [](const glm::vec3& a, float s){ return a - s; }
            ),
            sol::meta_function::multiplication, sol::overload(
                [](const glm::vec3& a, const glm::vec3& b){ return a * b; },
                [](const glm::vec3& a, float s){ return a * s; },
                [](float s, const glm::vec3& a){ return s * a; }
            ),
            sol::meta_function::division, sol::overload(
                [](const glm::vec3& a, const glm::vec3& b){ return a / b; },
                [](const glm::vec3& a, float s){ return a / s; }
            ),
            sol::meta_function::unary_minus, [](const glm::vec3& a){ return -a; },
            sol::meta_function::equal_to, [](const glm::vec3& a, const glm::vec3& b){ return a == b; },
            sol::meta_function::to_string, [](const glm::vec3& v)
            { 
                return glm::to_string(v);
            },
            
            "Length", [](const glm::vec3& v){ return glm::length(v); },
            "Normalize", [](const glm::vec3& v){ return glm::normalize(v); },
            "Dot", [](const glm::vec3& a, const glm::vec3& b){ return glm::dot(a, b); },
            "Cross", [](const glm::vec3& a, const glm::vec3& b){ return glm::cross(a, b); },
            "Distance", [](const glm::vec3& a, const glm::vec3& b){ return glm::distance(a, b); },
            "Reflect", [](const glm::vec3& v, const glm::vec3& n){ return glm::reflect(v, n); },
            "Lerp", [](const glm::vec3& a, const glm::vec3& b, float t){ return glm::mix(a, b, t); }
        );
        
        // vec4
        State.new_usertype<glm::vec4>("vec4",
            sol::call_constructor,
            sol::constructors<glm::vec4(), glm::vec4(float), glm::vec4(float, float, float, float), glm::vec4(const glm::vec3&, float)>(),
            "x", &glm::vec4::x,
            "y", &glm::vec4::y,
            "z", &glm::vec4::z,
            "w", &glm::vec4::w,
            
            sol::meta_function::addition, sol::overload(
                [](const glm::vec4& a, const glm::vec4& b){ return a + b; },
                [](const glm::vec4& a, float s){ return a + s; }
            ),
            sol::meta_function::subtraction, sol::overload(
                [](const glm::vec4& a, const glm::vec4& b){ return a - b; },
                [](const glm::vec4& a, float s){ return a - s; }
            ),
            sol::meta_function::multiplication, sol::overload(
                [](const glm::vec4& a, const glm::vec4& b){ return a * b; },
                [](const glm::vec4& a, float s){ return a * s; },
                [](float s, const glm::vec4& a){ return s * a; }
            ),
            sol::meta_function::division, sol::overload(
                [](const glm::vec4& a, const glm::vec4& b){ return a / b; },
                [](const glm::vec4& a, float s){ return a / s; }
            ),
            sol::meta_function::unary_minus, [](const glm::vec4& a){ return -a; },
            sol::meta_function::equal_to, [](const glm::vec4& a, const glm::vec4& b){ return a == b; },
            sol::meta_function::to_string, [](const glm::vec4& v)
            {
                return glm::to_string(v);
            },
            
            "Length", [](const glm::vec4& v){ return glm::length(v); },
            "Normalize", [](const glm::vec4& v){ return glm::normalize(v); },
            "Dot", [](const glm::vec4& a, const glm::vec4& b){ return glm::dot(a, b); },
            "Distance", [](const glm::vec4& a, const glm::vec4& b){ return glm::distance(a, b); }
        );
        
        // quat
        State.new_usertype<glm::quat>("quat",
            sol::call_constructor,
            sol::constructors<
                glm::quat(), 
                glm::quat(float, float, float, float),
                glm::quat(const glm::vec3&)
            >(),
            "x", &glm::quat::x,
            "y", &glm::quat::y,
            "z", &glm::quat::z,
            "w", &glm::quat::w,
            
            sol::meta_function::multiplication, sol::overload(
                [](const glm::quat& a, const glm::quat& b){ return a * b; },
                [](const glm::quat& a, const glm::vec3& v){ return a * v; },
                [](const glm::quat& a, const glm::vec4& v){ return a * v; },
                [](const glm::quat& a, float s){ return a * s; }
            ),
            sol::meta_function::equal_to, [](const glm::quat& a, const glm::quat& b){ return a == b; },
            sol::meta_function::to_string, [](const glm::quat& q)
            {
                return glm::to_string(q);
            },
            
            "Length", [](const glm::quat& q){ return glm::length(q); },
            "Normalize", [](const glm::quat& q){ return glm::normalize(q); },
            "Conjugate", [](const glm::quat& q){ return glm::conjugate(q); },
            "Inverse", [](const glm::quat& q){ return glm::inverse(q); },
            "Dot", [](const glm::quat& a, const glm::quat& b){ return glm::dot(a, b); },
            "Slerp", [](const glm::quat& a, const glm::quat& b, float t){ return glm::slerp(a, b, t); },
            "Lerp", [](const glm::quat& a, const glm::quat& b, float t){ return glm::mix(a, b, t); },
            "EulerAngles", [](const glm::quat& q){ return glm::eulerAngles(q); },
            "AngleAxis", [](float angle, const glm::vec3& axis){ return glm::angleAxis(angle, axis); },
            "FromEuler", [](const glm::vec3& euler){ return glm::quat(euler); }
        );
        
        State.new_enum("UpdateStage",
            "FrameStart",       EUpdateStage::FrameStart,
            "PrePhysics",       EUpdateStage::PrePhysics,
            "DuringPhysics",    EUpdateStage::DuringPhysics,
            "PostPhysics",      EUpdateStage::PostPhysics,
            "FrameEnd",         EUpdateStage::FrameEnd,
            "Paused",           EUpdateStage::Paused);
        
        FSystemContext::RegisterWithLua(State);
        
    }

    void FScriptingContext::SetupInput()
    {
        State.set_function("print", [&](sol::this_state s, const sol::variadic_args& args)
        {
            sol::state_view lua(s);
            sol::protected_function LuaStringFunc = lua["tostring"];
    
            FFixedString Output;
    
            for (size_t i = 0; i < args.size(); ++i)
            {
                sol::object Obj = args[i];
        
                sol::protected_function_result Result = LuaStringFunc(Obj);
        
                if (Result.valid())
                {
                    if (sol::optional<const char*> str = Result)
                    {
                        Output += *str;
                    }
                    else
                    {
                        Output += "[tostring error]";
                    }
                }
                else
                {
                    sol::error err = Result;
                    Output += "[error: ";
                    Output += err.what();
                    Output += "]";
                }
        
                if (i < args.size() - 1)
                {
                    Output += "\t";
                }
            }
    
            LOG_INFO("[Lua] {}", Output);
        });


        sol::table GLMTable = State.create_named_table("glm");
        
        // Vector operations
        GLMTable.set_function("Normalize", [](glm::vec3 Vec) { return glm::normalize(Vec); });
        GLMTable.set_function("Length", [](glm::vec3 Vec) { return glm::length(Vec); });
        GLMTable.set_function("Dot", [](glm::vec3 A, glm::vec3 B) { return glm::dot(A, B); });
        GLMTable.set_function("Cross", [](glm::vec3 A, glm::vec3 B) { return glm::cross(A, B); });
        GLMTable.set_function("Distance", [](glm::vec3 A, glm::vec3 B) { return glm::distance(A, B); });
        GLMTable.set_function("Mix", [](glm::vec3 A, glm::vec3 B, float t) { return glm::mix(A, B, t); });
        GLMTable.set_function("ClampVec3", [](glm::vec3 V, float Min, float Max) { return glm::clamp(V, Min, Max); });
        GLMTable.set_function("LerpVec3", [](glm::vec3 A, glm::vec3 B, float t) { return glm::mix(A, B, t); });
        
        // Quaternion operations
        GLMTable.set_function("QuatLookAt", [](glm::vec3 Forward, glm::vec3 Up) { return glm::quatLookAt(Forward, Up); });
        GLMTable.set_function("QuatRotate", [](glm::quat Q, float AngleRad, glm::vec3 Axis) { return glm::rotate(Q, AngleRad, Axis); });
        GLMTable.set_function("QuatFromEuler", [](glm::vec3 Euler) { return glm::quat(Euler); });
        GLMTable.set_function("QuatToEuler", [](glm::quat Q) { return glm::eulerAngles(Q); });
        GLMTable.set_function("QuatMul", [](glm::quat A, glm::quat B) { return A * B; });
        GLMTable.set_function("QuatInverse", [](glm::quat Q) { return glm::inverse(Q); });
        
        // Matrix operations
        GLMTable.set_function("LookAt", [](glm::vec3 Eye, glm::vec3 Center, glm::vec3 Up) { return glm::lookAt(Eye, Center, Up); });
        GLMTable.set_function("QuatLookAt", [](const glm::vec3& Direction, const glm::vec3& Up) { return glm::quatLookAt(Direction, Up); });
        GLMTable.set_function("Perspective", [](float FOV, float Aspect, float Near, float Far) { return glm::perspective(FOV, Aspect, Near, Far); });
        GLMTable.set_function("Translate", [](glm::mat4 M, glm::vec3 V) { return glm::translate(M, V); });
        GLMTable.set_function("Rotate", [](glm::mat4 M, float AngleRad, glm::vec3 Axis) { return glm::rotate(M, AngleRad, Axis); });
        GLMTable.set_function("Scale", [](glm::mat4 M, glm::vec3 S) { return glm::scale(M, S); });
        
        // Math utilities
        GLMTable.set_function("Radians", [](float Deg) { return glm::radians(Deg); });
        GLMTable.set_function("Degrees", [](float Rad) { return glm::degrees(Rad); });
        GLMTable.set_function("ClampFloat", [](float Value, float Min, float Max) { return glm::clamp(Value, Min, Max); });
        GLMTable.set_function("MixFloat", [](float A, float B, float t) { return glm::mix(A, B, t); });
        GLMTable.set_function("Lerp", [](float A, float B, float t) { return glm::mix(A, B, t); });
        GLMTable.set_function("Min", [](float A, float B) { return glm::min(A, B); });
        GLMTable.set_function("Max", [](float A, float B) { return glm::max(A, B); });
        GLMTable.set_function("Sign", [](float Value) { return (Value > 0.0f) ? 1.0f : (Value < 0.0f ? -1.0f : 0.0f); });

        // Quaternion constructors
        GLMTable.set_function("QuatAngleAxis", [](float AngleRad, glm::vec3 Axis) { return glm::angleAxis(AngleRad, Axis); });
        GLMTable.set_function("QuatAxis", [](glm::quat Q) { return glm::axis(Q); });
        GLMTable.set_function("QuatAngle", [](glm::quat Q) { return glm::angle(Q); });

        // Quaternion interpolation
        GLMTable.set_function("QuatSlerp", [](glm::quat A, glm::quat B, float t) { return glm::slerp(A, B, t); });


        sol::table ColorTable = State.create_named_table("Color");
        ColorTable.set_function("RandomColor4", []() ->glm::vec4 { return FColor::MakeRandom(1); });
        ColorTable.set_function("RandomColor3", []() ->glm::vec3 { return FColor::MakeRandom(1); });
        
        
        
        
        sol::table InputTable = State.create_named_table("Input");
        InputTable.set_function("GetMouseX",        [] () { return FInputProcessor::Get().GetMouseX(); }),
        InputTable.set_function("GetMouseY",        [] () { return FInputProcessor::Get().GetMouseY(); }),
        InputTable.set_function("GetMouseDeltaX",   [] () { return FInputProcessor::Get().GetMouseDeltaX(); }),
        InputTable.set_function("GetMouseDeltaY",   [] () { return FInputProcessor::Get().GetMouseDeltaY(); }),
        
        InputTable.set_function("EnableCursor",     [] () { FInputProcessor::Get().SetCursorMode(GLFW_CURSOR_NORMAL); }),
        InputTable.set_function("DisableCursor",    [] () { FInputProcessor::Get().SetCursorMode(GLFW_CURSOR_DISABLED); }),
        InputTable.set_function("HideCursor",       [] () { FInputProcessor::Get().SetCursorMode(GLFW_CURSOR_HIDDEN); }),
        
        InputTable.set_function("IsKeyDown",        [] (EKeyCode Key) { return FInputProcessor::Get().IsKeyDown(Key); }),
        InputTable.set_function("IsKeyUp",          [] (EKeyCode Key) { return FInputProcessor::Get().IsKeyUp(Key); }),
        InputTable.set_function("IsKeyRepeated",    [] (EKeyCode Key) { return FInputProcessor::Get().IsKeyRepeated(Key); }),
        
        InputTable.new_enum("EKeyCode",
            "Space",        EKeyCode::Space,
            "Apostrophe",   EKeyCode::Apostrophe,
            "Comma",        EKeyCode::Comma,
            "Minus",        EKeyCode::Minus,
            "Period",       EKeyCode::Period,
            "Slash",        EKeyCode::Slash,
        
            "D0",           EKeyCode::D0,
            "D1",           EKeyCode::D1,
            "D2",           EKeyCode::D2,
            "D3",           EKeyCode::D3,
            "D4",           EKeyCode::D4,
            "D5",           EKeyCode::D5,
            "D6",           EKeyCode::D6,
            "D7",           EKeyCode::D7,
            "D8",           EKeyCode::D8,
            "D9",           EKeyCode::D9,
        
            "Semicolon",    EKeyCode::Semicolon,
            "Equal",        EKeyCode::Equal,
        
            "A",            EKeyCode::A,
            "B",            EKeyCode::B,
            "C",            EKeyCode::C,
            "D",            EKeyCode::D,
            "E",            EKeyCode::E,
            "F",            EKeyCode::F,
            "G",            EKeyCode::G,
            "H",            EKeyCode::H,
            "I",            EKeyCode::I,
            "J",            EKeyCode::J,
            "K",            EKeyCode::K,
            "L",            EKeyCode::L,
            "M",            EKeyCode::M,
            "N",            EKeyCode::N,
            "O",            EKeyCode::O,
            "P",            EKeyCode::P,
            "Q",            EKeyCode::Q,
            "R",            EKeyCode::R,
            "S",            EKeyCode::S,
            "T",            EKeyCode::T,
            "U",            EKeyCode::U,
            "V",            EKeyCode::V,
            "W",            EKeyCode::W,
            "X",            EKeyCode::X,
            "Y",            EKeyCode::Y,
            "Z",            EKeyCode::Z,
        
            "LeftBracket",  EKeyCode::LeftBracket,
            "Backslash",    EKeyCode::Backslash,
            "RightBracket", EKeyCode::RightBracket,
            "GraveAccent",  EKeyCode::GraveAccent,
        
            "World1",       EKeyCode::World1,
            "World2",       EKeyCode::World2,
        
            "Escape",       EKeyCode::Escape,
            "Enter",        EKeyCode::Enter,
            "Tab",          EKeyCode::Tab,
            "Backspace",    EKeyCode::Backspace,
            "Insert",       EKeyCode::Insert,
            "Delete",       EKeyCode::Delete,
            "Right",        EKeyCode::Right,
            "Left",         EKeyCode::Left,
            "Down",         EKeyCode::Down,
            "Up",           EKeyCode::Up,
            "PageUp",       EKeyCode::PageUp,
            "PageDown",     EKeyCode::PageDown,
            "Home",         EKeyCode::Home,
            "End",          EKeyCode::End,
            "CapsLock",     EKeyCode::CapsLock,
            "ScrollLock",   EKeyCode::ScrollLock,
            "NumLock",      EKeyCode::NumLock,
            "PrintScreen",  EKeyCode::PrintScreen,
            "Pause",        EKeyCode::Pause,
        
            "F1",           EKeyCode::F1,
            "F2",           EKeyCode::F2,
            "F3",           EKeyCode::F3,
            "F4",           EKeyCode::F4,
            "F5",           EKeyCode::F5,
            "F6",           EKeyCode::F6,
            "F7",           EKeyCode::F7,
            "F8",           EKeyCode::F8,
            "F9",           EKeyCode::F9,
            "F10",          EKeyCode::F10,
            "F11",          EKeyCode::F11,
            "F12",          EKeyCode::F12,
            "F13",          EKeyCode::F13,
            "F14",          EKeyCode::F14,
            "F15",          EKeyCode::F15,
            "F16",          EKeyCode::F16,
            "F17",          EKeyCode::F17,
            "F18",          EKeyCode::F18,
            "F19",          EKeyCode::F19,
            "F20",          EKeyCode::F20,
            "F21",          EKeyCode::F21,
            "F22",          EKeyCode::F22,
            "F23",          EKeyCode::F23,
            "F24",          EKeyCode::F24,
            "F25",          EKeyCode::F25,
        
            "KP0",          EKeyCode::KP0,
            "KP1",          EKeyCode::KP1,
            "KP2",          EKeyCode::KP2,
            "KP3",          EKeyCode::KP3,
            "KP4",          EKeyCode::KP4,
            "KP5",          EKeyCode::KP5,
            "KP6",          EKeyCode::KP6,
            "KP7",          EKeyCode::KP7,
            "KP8",          EKeyCode::KP8,
            "KP9",          EKeyCode::KP9,
            "KPDecimal",    EKeyCode::KPDecimal,
            "KPDivide",     EKeyCode::KPDivide,
            "KPMultiply",   EKeyCode::KPMultiply,
            "KPSubtract",   EKeyCode::KPSubtract,
            "KPAdd",        EKeyCode::KPAdd,
            "KPEnter",      EKeyCode::KPEnter,
            "KPEqual",      EKeyCode::KPEqual,
        
            "LeftShift",    EKeyCode::LeftShift,
            "LeftControl",  EKeyCode::LeftControl,
            "LeftAlt",      EKeyCode::LeftAlt,
            "LeftSuper",    EKeyCode::LeftSuper,
            "RightShift",   EKeyCode::RightShift,
            "RightControl", EKeyCode::RightControl,
            "RightAlt",     EKeyCode::RightAlt,
            "RightSuper",   EKeyCode::RightSuper,
            "Menu",         EKeyCode::Menu
        );
    }

    TVector<entt::entity> FScriptingContext::LoadScriptPath(FStringView ScriptPath, bool bFailSilently)
    {
        sol::environment Environment(State, sol::create, State.globals());
        
        FString NormalizedPath(ScriptPath);
        Paths::NormalizePath(NormalizedPath);
        for (entt::entity EntityToRemove : PathToScriptEntities[NormalizedPath])
        {
            ScriptRegistry.destroy(EntityToRemove);
        };
        
        PathToScriptEntities[NormalizedPath].clear();
    
        sol::protected_function_result Result = State.safe_script_file(ScriptPath.data(), Environment);
        if (!Result.valid())
        {
            sol::error Error = Result;
            if (!bFailSilently)
            {
                LOG_ERROR("[Sol] - Error loading script! {}", Error.what());
            }
            return {};
        }

        TVector<CScriptFactory*> Factories;
        CScriptFactoryRegistry::Get().GetFactories(Factories);

        sol::object ReturnedObject = Result;
        if (!ReturnedObject.is<sol::table>())
        {
            if (!bFailSilently)
            {
                LOG_WARN("[Sol] Script {} did not return a table", ScriptPath);
            }
            return {};
        }

        sol::table ScriptTable = ReturnedObject.as<sol::table>();
        TVector<entt::entity> NewScriptEntities;
        
        for (const auto& [key, value] : ScriptTable)
        {
            if (!value.is<sol::table>() || !key.is<const char*>())
            {
                continue;
            }
            
            sol::table EntryTable = value.as<sol::table>();
    
            sol::object TypeObj = EntryTable["Type"];
            if (!TypeObj.valid() || !TypeObj.is<const char*>())
            {
                continue;
            }
    
            for (CScriptFactory* Factory : Factories)
            {
                if (entt::entity Entity = Factory->ProcessScript(key.as<const char*>(), EntryTable, ScriptRegistry); Entity != entt::null)
                {
                    PathToScriptEntities[NormalizedPath].push_back(Entity);
                    NewScriptEntities.push_back(Entity);
                    break;
                }
            }
        }
        
        OnScriptLoaded.Broadcast();
        
        return NewScriptEntities;
    }
}
