#pragma once

#include <format>
#include <utility>
#include <string_view>
namespace ACTGame
{
class ILogService
{
public:
    ILogService() = default;
    virtual ~ILogService() = default;

    template <typename ... Args>
    void LogInfo(const std::string_view fmt, Args&& ... args)
    {
        std::string FormattedMessage = std::format(fmt, std::forward<Args>(args)...);
        LogInfo_Impl(FormattedMessage);
    }

protected:
    virtual void LogInfo_Impl(const std::string& FormattedMessage) = 0;
};

class Logger
{
public:
    void Initialize(ILogService* logService);

    template <typename ... Args>
    void LogInfo(const char* fmt, Args&& ... args)
    {
        if (LogService)
        {
            LogService->LogInfo(fmt, std::forward<Args>(args)...);
        }
    }

private:
    ILogService* LogService{ nullptr };
};

extern Logger g_Logger;
} // namespace ACTGame
