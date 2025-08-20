#pragma once

#include "Containers/UnrealString.h"
#include "Framework/Interface/ILogService.h"
#include "Logging/LogMacros.h"
#include "Logging/LogVerbosity.h"

DECLARE_LOG_CATEGORY_EXTERN(ACTGameLog, Log, All)

class ACT_UELog final : public ACTGame::ILogService
{
protected:
    virtual void LogInfo_Impl(const std::string& FormattedMessage) override;
};
