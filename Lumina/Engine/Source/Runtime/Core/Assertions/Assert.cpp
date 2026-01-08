#include "PCH.h"
#include "Assert.h"

#include "Core/Threading/Thread.h"


namespace Lumina::Assert
{
    static void AssertionTest()
    {
        int X = 12;
        
        DEBUG_ASSERT(X == 12, "Thingy");
    }
    
    static constexpr FStringView AssertionTypeToString(EAssertionType Type)
    {
        switch (Type)
        {
            case EAssertionType::Assert:        return "ASSERTION";
            case EAssertionType::Assume:        return "ASSUME";
            case EAssertionType::DebugAssert:   return "DEBUG ASSERTION";
            case EAssertionType::Unreachable:   return "UNREACHABLE";
            case EAssertionType::Panic:         return "PANIC";
            case EAssertionType::Alert:         return "ALERT";
            default:                            return "";
        }
    }
    
    static constexpr bool ShouldAbortOnAssertion(EAssertionType Type)
    {
        switch (Type)
        {
            case EAssertionType::Assert:        return true;
            case EAssertionType::Assume:        return true;
            case EAssertionType::DebugAssert:   return true;
            case EAssertionType::Unreachable:   return true;
            case EAssertionType::Panic:         return true;
            case EAssertionType::Alert:         return false;
            default:                            return true;
        }
    }

    static void DefaultAssertionHandler(const FAssertion& Assertion)
    {
        if (::Lumina::Logging::IsInitialized())
        {
            LOG_CRITICAL("==================================================================================");
            LOG_CRITICAL("{} FAILED: {}", AssertionTypeToString(Assertion.Type), Assertion.Expression);
            
            if (!Assertion.Message.empty())
            {
                LOG_CRITICAL("Message: {}", Assertion.Message);
            }
            
            std::basic_stacktrace Trace = std::stacktrace::current();
            size_t Skip = 2;
            for (size_t i = Skip; i < Trace.size(); ++i)
            {
                const std::stacktrace_entry& Entry = Trace[i];
                LOG_CRITICAL("  #{} {}", i - Skip, std::to_string(Entry));
            }
            
            LOG_CRITICAL("File: {}:{}", Assertion.Location.file_name(), Assertion.Location.line());
            LOG_CRITICAL("Function: {}", Assertion.Location.function_name());
            
            LOG_CRITICAL("==================================================================================");
            
            if (ShouldAbortOnAssertion(Assertion.Type))
            {
                std::abort();
            }
            
            Threading::Sleep(5);
        }
    }
    
    static AssertionHandler GAssertionHandler = DefaultAssertionHandler;
    
    void Detail::HandleAssertion(const FAssertion& Assertion)
    {
        GAssertionHandler(Assertion);
    }

    void SetAssertionHandler(AssertionHandler Handler)
    {
        GAssertionHandler = Handler ? Handler : DefaultAssertionHandler;
    }
}
