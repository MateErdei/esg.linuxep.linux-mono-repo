/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IServiceExtension.h"
#include "ResultsSender.h"
#include <OsquerySDK/OsquerySDK.h>
#include <thread>

class LoggerExtension : public IServiceExtension
{
public:
    LoggerExtension( const std::string& intermediaryPath,
                     const std::string& datafeedPath,
                     const std::string& osqueryXDRConfigFilePath,
                     const std::string& pluginVarDir,
                     unsigned int dataLimit,
                     unsigned int periodInSeconds);
    ~LoggerExtension();
    void Start(const std::string& socket, bool verbose, uintmax_t maxBatchBytes, unsigned int maxBatchSeconds) override;
    void Stop() override;
    void setDataLimit(unsigned int limitBytes);
    void setDataPeriod(unsigned int periodSeconds);

private:
    void Run();
    ResultsSender m_resultsSender;
    bool m_stopped = { true };
    std::unique_ptr<std::thread> m_runnerThread;
    OsquerySDK::Flags m_flags;
    std::unique_ptr<OsquerySDK::ExtensionInterface> m_extension;
    uintmax_t m_maxBatchBytes = 10000000;
    unsigned int m_maxBatchSeconds = 15;


};