#ifndef SRC_CORE_LOGGER_LOGGER_H
#define SRC_CORE_LOGGER_LOGGER_H

#include <iostream>
#include <string>

namespace pwg
{
	/**
	 * @brief Prints messages of different severity to the standard output.
	 */
	class Logger
	{
	public:
		Logger();
		~Logger() = default;

		enum class Module
		{
			Application,
			Window,
			IndexBuffer,
			VertexArray,
			VertexBuffer,
			IMesh,
			TriangleMesh,
			PyramidMesh,
			Renderer,
			Shader,
			Logger
		};

		enum class Severity
		{
			INFO,
			DEBUG,
			ERROR
		};


		/**
		 * @brief Prints an info log message
		 * @param module Identifier of the module from which the log message originates.
		 * @param message Message to be printed.
		 */
		static void LogInfo(const Module module, const std::string& message);


		/**
		 * @brief Prints a debug log message
		 * @param module Identifier of the module from which the log message originates.
		 * @param message Message to be printed.
		 */
		static void LogDebug(const Module module, const std::string& message);


		/**
		 * @brief Prints an error log message
		 * @param module Identifier of the module from which the log message originates.
		 * @param message Message to be printed.
		 */
		static void LogError(const Module module, const std::string& message);
	private:


		/**
		 * @brief Prints a given message 
		 * @param module Identifier of the module from which the log message originates.
		 * @param logLevel Severity of the log message.
		 * @param message Message to be printed.
		 */
		static void Log(const Module module, const Severity logLevel, const std::string& message);


		/**
		 * @brief Converts module identifier to string
		 * @param module Identifier of the module.
		 */
		static std::string ModuleToString(const Module module);
	};
} //namespace pwg
#endif // !SRC_CORE_LOGGER_LOGGER_H
