#include "pch.h"
#include "Scripting.h"

#include "Core/Math/Color.h"
#include "Events/KeyCodes.h"
#include "Input/InputProcessor.h"
#include "Memory/SmartPtr.h"
#include "Paths/Paths.h"
#include "Scripting/DeferredScriptRegistry.h"
#include "World/Entity/Components/TagComponent.h"
#include "World/Entity/Systems/SystemContext.h"

namespace Lumina::Scripting
{
    static TUniquePtr<FScriptingContext> GScriptingContext;
    
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
        RegisteredScripts.clear();
    }

    void FScriptingContext::OnScriptReloaded(FStringView ScriptPath)
    {
        FScopeLock Lock(Mutex);

        TUniquePtr<FLuaScriptEntry>* FoundEntryPtr = nullptr;
        for (auto& [Name, PathVector] : RegisteredScripts)
        {
            for (size_t i = 0; i < PathVector.size(); ++i)
            {
                TUniquePtr<FLuaScriptEntry>& Path = PathVector[i];
                if (Paths::PathsEqual(ScriptPath, Path->Path))
                {
                    FoundEntryPtr = &Path;
                    break;
                }
            }
        
            if (FoundEntryPtr)
            {
                break;
            }
        }

        if (FoundEntryPtr)
        {
            if (TUniquePtr<FLuaScriptEntry> NewScript = LoadScript(ScriptPath))
            {
                *FoundEntryPtr = Move(NewScript);
                LOG_INFO("Script reloaded: {}", ScriptPath);
            }
            else
            {
                LOG_ERROR("Failed to reload script: {}", ScriptPath);
            }

            return;
        }

        FString ParentPath = Paths::Parent(ScriptPath);
        Paths::NormalizePath(ParentPath);

        if (TUniquePtr<FLuaScriptEntry> NewScript = LoadScript(ScriptPath))
        {
            RegisteredScripts[ParentPath].push_back(Move(NewScript));
        }
    }

    void FScriptingContext::OnScriptCreated(FStringView ScriptPath)
    {
        FScopeLock Lock(Mutex);

        FString ScriptDirectory = Paths::Parent(ScriptPath);
        Paths::NormalizePath(ScriptDirectory);

        bool bAlreadyExists = false;
        for (auto& [Name, PathVector] : RegisteredScripts)
        {
            for (size_t i = 0; i < PathVector.size(); ++i)
            {
                TUniquePtr<FLuaScriptEntry>& Path = PathVector[i];
                if (Paths::PathsEqual(ScriptPath, Path->Path))
                {
                    bAlreadyExists = true;
                    break;
                }
            }
        }

        if (bAlreadyExists)
        {
            LOG_ERROR("Newly scripted script already exists! {}", ScriptPath);
            return;
        }

        if (TUniquePtr<FLuaScriptEntry> NewScript = LoadScript(ScriptPath))
        {
            RegisteredScripts[FName(ScriptDirectory)].emplace_back(Move(NewScript));
        }
    }

    void FScriptingContext::OnScriptRenamed(FStringView NewPath, FStringView OldPath)
    {
        FScopeLock Lock(Mutex);
        

    }

    void FScriptingContext::OnScriptDeleted(FStringView ScriptPath)
    {
        FScopeLock Lock(Mutex);

        for (auto& [Name, PathVector] : RegisteredScripts)
        {
            for (size_t i = 0; i < PathVector.size(); ++i)
            {
                TUniquePtr<FLuaScriptEntry>& Path = PathVector[i];
                if (Paths::PathsEqual(ScriptPath, Path->Path))
                {
                    PathVector.erase(PathVector.begin() + i);
                    
                    LOG_INFO("Script {} Deleted", ScriptPath);

                    break;
                }
            }
        }
    }

    
    void FScriptingContext::LoadScriptsInDirectoryRecursively(FStringView Directory)
    {
        FScopeLock Lock(Mutex);
        
        RegisteredScripts.clear();
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

                if (TUniquePtr<FLuaScriptEntry> NewScript = LoadScript(ScriptPath))
                {
                    RegisteredScripts[FName(ScriptDirectory)].emplace_back(Move(NewScript));
                }
            }
        }
    }
    
    const TVector<FScriptEntryHandle>& FScriptingContext::GetScriptsUnderDirectory(FName Directory)
    {
        return RegisteredScripts[Directory];
    }

    void FScriptingContext::RegisterCoreTypes()
    {

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
            sol::meta_function::to_string, [](const glm::vec2& v){ 
                return "vec2(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ")"; 
            },
            
            "Length", [](const glm::vec2& v){ return glm::length(v); },
            "Normalize", [](const glm::vec2& v){ return glm::normalize(v); },
            "Dot", [](const glm::vec2& a, const glm::vec2& b){ return glm::dot(a, b); },
            "Distance", [](const glm::vec2& a, const glm::vec2& b){ return glm::distance(a, b); }
        );
        
        RegisterLuaConverterByName<glm::vec2>("vec2");
        
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
            sol::meta_function::to_string, [](const glm::vec3& v){ 
                return "vec3(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ", " + std::to_string(v.z) + ")"; 
            },
            
            "Length", [](const glm::vec3& v){ return glm::length(v); },
            "Normalize", [](const glm::vec3& v){ return glm::normalize(v); },
            "Dot", [](const glm::vec3& a, const glm::vec3& b){ return glm::dot(a, b); },
            "Cross", [](const glm::vec3& a, const glm::vec3& b){ return glm::cross(a, b); },
            "Distance", [](const glm::vec3& a, const glm::vec3& b){ return glm::distance(a, b); },
            "Reflect", [](const glm::vec3& v, const glm::vec3& n){ return glm::reflect(v, n); },
            "Lerp", [](const glm::vec3& a, const glm::vec3& b, float t){ return glm::mix(a, b, t); }
        );
        
        RegisterLuaConverterByName<glm::vec3>("vec3");
        
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
            sol::meta_function::to_string, [](const glm::vec4& v){ 
                return "vec4(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ", " + std::to_string(v.z) + ", " + std::to_string(v.w) + ")"; 
            },
            
            "Length", [](const glm::vec4& v){ return glm::length(v); },
            "Normalize", [](const glm::vec4& v){ return glm::normalize(v); },
            "Dot", [](const glm::vec4& a, const glm::vec4& b){ return glm::dot(a, b); },
            "Distance", [](const glm::vec4& a, const glm::vec4& b){ return glm::distance(a, b); }
        );
        
        RegisterLuaConverterByName<glm::vec4>("vec4");
        
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
                return "quat(" + std::to_string(q.w) + ", " + std::to_string(q.x) + ", " + std::to_string(q.y) + ", " + std::to_string(q.z) + ")"; 
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
        
        RegisterLuaConverterByName<glm::quat>("quat");
        
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

        State.set_function("print", [&](const sol::variadic_args& args)
        {
            FString Output;
            
            for (size_t i = 0; i < args.size(); ++i)
            {
                sol::object Obj = args[i];
                
                // Convert each argument to string
                if (Obj.is<const char*>())
                {
                    Output += Obj.as<const char*>();
                }
                else if (Obj.is<double>())
                {
                    Output += eastl::to_string(Obj.as<double>());
                }
                else if (Obj.is<int>())
                {
                    Output += eastl::to_string(Obj.as<int>());
                }
                else if (Obj.is<bool>())
                {
                    Output += Obj.as<bool>() ? "true" : "false";
                }
                else
                {
                    sol::optional<FString> Str = State["tostring"](Obj);
                    Output += Str.has_value() ? Str->c_str() : "[unknown type]";
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
        
        InputTable.new_enum("Key",
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
    
    sol::object FScriptingContext::ConvertToSolObjectFromName(FName Name, const sol::state_view& View, void* Data)
    {
        LUMINA_PROFILE_SCOPE();

        auto it = LuaConverters.find(Name);
        if (it == LuaConverters.end())
        {
            return sol::nil;
        }

        return it->second.To(View, Data);
    }

    
    FLuaConverter::ToFunctionType* FScriptingContext::GetSolConverterFunctionPtrByName(FName Name)
    {
        LUMINA_PROFILE_SCOPE();

        auto it = LuaConverters.find(Name);
        if (it == LuaConverters.end())
        {
            return nullptr;
        }

        return it->second.To;
    }
    

    void* FScriptingContext::ConvertFromSolObjectToVoidPtrByName(FName Name, const sol::object& Object)
    {
        auto it = LuaConverters.find(Name);
        if (it == LuaConverters.end())
        {
            return nullptr;
        }

        return it->second.From(Object);
    }

    TUniquePtr<FLuaScriptEntry> FScriptingContext::LoadScript(FStringView ScriptPath, bool bFailSilently)
    {
        sol::environment Environment(State, sol::create, State.globals());
    
        sol::protected_function_result Result = State.safe_script_file(ScriptPath.data(), Environment);
        if (!Result.valid())
        {
            sol::error Error = Result;
            if (!bFailSilently)
            {
                LOG_ERROR("[Sol] - Error loading script! {}", Error.what());
            }
            return nullptr;
        }
    
        sol::object ReturnedObject = Result;
        if (!ReturnedObject.is<sol::table>())
        {
            if (!bFailSilently)
            {
                LOG_ERROR("[Sol] Script {} did not return a table", ScriptPath);
            }
            return nullptr;
        }

        OnScriptLoaded.Broadcast();

        TUniquePtr<FLuaScriptEntry> Entry = MakeUniquePtr<FLuaScriptEntry>();
        Entry->Path = ScriptPath;
        Entry->Environment = Environment;
        Entry->Table = ReturnedObject.as<sol::table>();
        Entry->Type = ELuaScriptType::System;
        
        return Entry;
    }
}
