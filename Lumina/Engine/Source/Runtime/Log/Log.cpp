#include "pch.h"
#include "Log.h"

#include <filesystem>

PRAGMA_DISABLE_ALL_WARNINGS
#include "Sinks/ConsoleSink.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include "spdlog/sinks/ringbuffer_sink.h"
PRAGMA_ENABLE_ALL_WARNINGS

namespace Lumina::Logging
{
	
	LUMINA_API std::shared_ptr<spdlog::logger> Logger;
	LUMINA_API FLogQueue Logs(300);
	
	bool IsInitialized()
	{
		return Logger != nullptr;
	}

	void Init()
	{
		spdlog::set_pattern("%^[%T] %n: %v%$");
		Logger = spdlog::stdout_color_mt("Lumina");
		Logger->sinks().push_back(std::make_shared<ConsoleSink>(Logs));
		Logger->set_level(spdlog::level::trace);
	
		LOG_TRACE("------- Log Initialized -------");

	}

	std::shared_ptr<spdlog::sinks::sink> GetSink()
	{
		return Logger->sinks().front();
	}

	void Shutdown()
	{
		LOG_TRACE("------- Log Shutdown -------");
		spdlog::shutdown();
		Logger = nullptr;
	}

	const FLogQueue& GetConsoleLogQueue()
	{
		return Logs;
	}

	std::shared_ptr<spdlog::logger> GetLogger()
	{
		return Logger;
	}
	
}
