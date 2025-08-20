#include "UELog.h"
#include "Containers/UnrealString.h"

DEFINE_LOG_CATEGORY(ACTGameLog)

void ACT_UELog::LogInfo_Impl(const std::string& FormattedMessage)
{
    FString Msg = UTF8_TO_TCHAR(FormattedMessage.c_str());
    UE_LOG(ACTGameLog, Display, TEXT("%s"), *Msg);
}
