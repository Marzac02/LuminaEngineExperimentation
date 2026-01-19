#pragma once
#include <source_location>
#include "Log/Log.h"



#if __has_include(<stacktrace>) && defined(__cpp_lib_stacktrace)
#include <stacktrace>
#define HAS_STD_STACKTRACE 1
#else
#define HAS_STD_STACKTRACE 0
#endif

namespace Lumina::Assert
{
    enum class EAssertionType : uint8
    {
        DebugAssert,
        Assert,
        Assume,
        Unreachable,
        Panic,
        Alert,
    };
    
    struct LUMINA_API FAssertion
    {
        FString                 Message;
        std::source_location    Location;
        FStringView             Expression;
        EAssertionType          Type;
    };
    
    
    using AssertionHandler = void(*)(const FAssertion&);
    
    LUMINA_API void SetAssertionHandler(AssertionHandler Handler);
    
    namespace Detail
    {
        FORCENOINLINE LUMINA_API void HandleAssertion(const FAssertion& Assertion);
    }
    
    // ========================= LUMINA ASSERTION INFO =========================
    // ASSERT_DEBUG(...)    - Checked in debug, no codegen in release.
    // ASSERT(...)          - Checked in both debug and release.
    // ASSUME(...)          - Checked in debug, compiler optimization hint in release.
    // =========================================================================
    
#define LUMINA_HANDLE_ASSERTION_HEADER \
    ::Lumina::Assert::Detail::HandleAssertion(::Lumina::Assert::FAssertion


#define LUMINA_ASSERTION_BODY(Expr, AssertType, ...) \
    if(!(Expr)) [[unlikely]] \
    { \
        LUMINA_HANDLE_ASSERTION_HEADER \
        { \
            __VA_OPT__(.Message = std::format(__VA_ARGS__).c_str(),) \
            .Location = std::source_location::current(), \
            .Expression = #Expr, \
            .Type = ::Lumina::Assert::EAssertionType::AssertType \
        }); \
    } \

    
#define LUMINA_ASSERT_INVOKE(Expr, Type, ...) \
    do { \
    LUMINA_ASSERTION_BODY(Expr, Type, __VA_ARGS__) \
    } while(0)
    

#ifdef LE_DEBUG
#define LUMINA_DEBUG_ASSERT(Expr, ...)  LUMINA_ASSERT_INVOKE(Expr,      DebugAssert,    __VA_ARGS__)
#else
#define LUMINA_DEBUG_ASSERT(...)        ((void)0)
#endif
    
#define LUMINA_ASSERT(Expr, ...)        LUMINA_ASSERT_INVOKE(Expr,      DebugAssert,    __VA_ARGS__)

#ifdef LE_DEBUG
#define LUMINA_ASSUME(Expr, ...)        LUMINA_ASSERT_INVOKE(Expr,      Assume,         __VA_ARGS__)
#else
#ifdef _MSC_VER
#define LUMINA_ASSUME(Condition, ...)   __assume(Condition)
#elif defined(__clang__)
#define LUMINA_ASSUME(Condition, ...)   __builtin_assume(Condition)
#elif defined(__GNUC__)
#define LUMINA_ASSUME(Condition, ...)   do { if (!(Condition)) __builtin_unreachable(); } while(0)
#else
#define LUMINA_ASSUME(Condition, ...)   ((void)0)
#endif
#endif
    

#define LUMINA_PANIC(...)               LUMINA_ASSERT_INVOKE(false,     Panic,          __VA_ARGS__)
    
#ifndef LE_SHIPPING
#define LUMINA_ALERT(Expr, ...)         LUMINA_ASSERT_INVOKE(Expr,      Alert,          __VA_ARGS__)
#else
#define LUMINA_ALERT(...)               ((void)0)
#endif
    
#ifdef LE_DEBUG
#define LUMINA_UNREACHABLE(...)         LUMINA_ASSERT_INVOKE(false,     Unreachable,    __VA_ARGS__); std::unreachable()
#else
#define LUMINA_UNREACHABLE(...)         std::unreachable()
#endif
    
    
    
#define DEBUG_ASSERT(...)   LUMINA_DEBUG_ASSERT(__VA_ARGS__)
#define ASSERT(...)         LUMINA_ASSERT(__VA_ARGS__)
#define ASSUME(...)         LUMINA_ASSUME(__VA_ARGS__)
#define ALERT_IF_NOT(...)   LUMINA_ALERT(__VA_ARGS__)
#define PANIC(...)          LUMINA_PANIC(__VA_ARGS__)
#define UNREACHABLE(...)    LUMINA_UNREACHABLE(__VA_ARGS__)

    

    
    
}