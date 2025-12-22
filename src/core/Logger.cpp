#include "core/Logger.h"

#include <chrono>
#include <iomanip>
#include <iostream>

namespace Video2Card::Core
{

  void Logger::Log(LogLevel level, std::string_view message, const std::source_location& location)
  {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time);

    std::ostream& out = (level == LogLevel::Error) ? std::cerr : std::cout;

    const char* colorCode = "";
    const char* resetCode = "\033[0m";

    switch (level) {
      case LogLevel::Debug:
        colorCode = "\033[36m";
        break;
      case LogLevel::Info:
        colorCode = "\033[32m";
        break;
      case LogLevel::Warn:
        colorCode = "\033[33m";
        break;
      case LogLevel::Error:
        colorCode = "\033[31m";
        break;
    }

    out << std::put_time(&tm, "[%H:%M:%S] ") << colorCode << "[" << LevelToString(level) << "] " << resetCode << "["
        << location.file_name() << ":" << location.line() << "] " << message << std::endl;
  }

  std::string_view Logger::LevelToString(LogLevel level)
  {
    switch (level) {
      case LogLevel::Debug:
        return "DEBUG";
      case LogLevel::Info:
        return "INFO";
      case LogLevel::Warn:
        return "WARN";
      case LogLevel::Error:
        return "ERROR";
      default:
        return "UNKNOWN";
    }
  }

} // namespace Video2Card::Core
