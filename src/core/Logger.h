#pragma once

#include <format>
#include <source_location>
#include <string>

namespace Video2Card::Core
{

  enum class LogLevel
  {
    Debug,
    Info,
    Warn,
    Error
  };

  class Logger
  {
public:

    static void Log(LogLevel level,
                    std::string_view message,
                    const std::source_location& location = std::source_location::current());

private:

    static std::string_view LevelToString(LogLevel level);
  };

} // namespace Video2Card::Core

#define AF_LOG_INTERNAL(level, ...)                                                                                    \
  Video2Card::Core::Logger::Log(level, std::format(__VA_ARGS__), std::source_location::current())

#ifdef NDEBUG
#define AF_DEBUG(...) ((void) 0)
#else
#define AF_DEBUG(...) AF_LOG_INTERNAL(Video2Card::Core::LogLevel::Debug, __VA_ARGS__)
#endif

#define AF_INFO(...) AF_LOG_INTERNAL(Video2Card::Core::LogLevel::Info, __VA_ARGS__)
#define AF_WARN(...) AF_LOG_INTERNAL(Video2Card::Core::LogLevel::Warn, __VA_ARGS__)
#define AF_ERROR(...) AF_LOG_INTERNAL(Video2Card::Core::LogLevel::Error, __VA_ARGS__)
