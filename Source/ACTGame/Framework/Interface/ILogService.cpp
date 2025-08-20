#include "ILogService.h"

namespace ACTGame
{
Logger g_Logger;

void Logger::Initialize(ILogService* logService)
{
    LogService = logService;
}
}