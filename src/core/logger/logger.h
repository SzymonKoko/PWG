#ifndef SRC_CORE_LOGGER_LOGGER_H
#define SRC_CORE_LOGGER_LOGGER_H

#include <iostream>
#include <string>
#include "spdlog/spdlog.h"
#include <memory>

namespace pwg
{
	/**
	 * @brief Prints messages of different severity to the standard output.
	 */
	class Logger
	{
	public:
		static void Init();

		inline static std::shared_ptr<spdlog::logger>& GetLogger() { return m_logger; }
	private:
		static std::shared_ptr<spdlog::logger> m_logger;

	};
} //namespace pwg

#define PWG_ERROR(...)	 ::pwg::Logger::GetLogger()->error(__VA_ARGS__)
#define PWG_WARN(...)	 ::pwg::Logger::GetLogger()->warn(__VA_ARGS__)
#define PWG_INFO(...)	 ::pwg::Logger::GetLogger()->info(__VA_ARGS__)
#define PWG_TRACE(...)	 ::pwg::Logger::GetLogger()->trace(__VA_ARGS__)
#define PWG_FATAL(...)	 ::pwg::Logger::GetLogger()->fatal(__VA_ARGS__)
#define PWG_DEBUG(...)	 ::pwg::Logger::GetLogger()->debug(__VA_ARGS__)
#endif // !SRC_CORE_LOGGER_LOGGER_H
