#pragma once
#include "Core/DisableAllWarnings.h"

PRAGMA_DISABLE_ALL_WARNINGS
#include <spdlog/sinks/base_sink.h>
PRAGMA_ENABLE_ALL_WARNINGS
#include "Containers/Array.h"
#include "Core/Profiler/Profile.h"
#include "Core/Templates/LuminaTemplate.h"
#include "Log/LogMessage.h"
#include "Memory/Memory.h"


namespace Lumina
{
    
    class ConsoleSink : public spdlog::sinks::base_sink<std::mutex>
    {
    public:
        ConsoleSink(Logging::FLogQueue& OutputMessages)
            : OutputMessages(OutputMessages)
        {
            
        }

    protected:
        
        void sink_it_(const spdlog::details::log_msg& msg) override
        {
        
            std::time_t timestamp = std::chrono::system_clock::to_time_t(msg.time);
            char TimeBuffer[16];
            (void)std::strftime(TimeBuffer, sizeof(TimeBuffer), "%H:%M:%S", std::localtime(&timestamp));  // NOLINT(concurrency-mt-unsafe)

            FConsoleMessage Message;
            Message.Time.assign(TimeBuffer);
            Message.LoggerName.assign(msg.logger_name.begin(), msg.logger_name.end());
            Message.Level = msg.level;
            Message.Message.assign(msg.payload.begin(), msg.payload.end());

            OutputMessages.push_back(Move(Message));
        }

        void flush_() override
        {
            
        }

        
        Logging::FLogQueue& OutputMessages;
    };
}

