#include "logger.h"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace pwg
{
	std::shared_ptr<spdlog::logger> Logger::m_logger;

	void Logger::Init()
	{
		spdlog::set_pattern("%^[%T] %n: %v%$");
		m_logger = spdlog::stdout_color_mt("PWG");
		m_logger->set_level(spdlog::level::trace);
	}
}