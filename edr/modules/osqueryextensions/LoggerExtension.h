/******************************************************************************************************

Copyright 2020-2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IServiceExtension.h"
#include "ResultsSender.h"
#include <OsquerySDK/OsquerySDK.h>
#include <thread>

class LoggerExtension : public IServiceExtension
{
public:
    LoggerExtension(
            const std::string& intermediaryPath,
            const std::string& datafeedPath,
            const std::string& osqueryXDRConfigFilePath,
            const std::string& osqueryMTRConfigFilePath,
            const std::string& osqueryCustomConfigFilePath,
            const std::string& pluginVarDir,
            unsigned long long int dataLimit,
            unsigned int periodInSeconds,
            std::function<void(void)> dataExceededCallback,
            unsigned int maxBatchSeconds,
            uintmax_t maxBatchBytes);
    ~LoggerExtension();
    void Start(const std::string& socket, bool verbose, std::shared_ptr<std::atomic_bool> extensionFinished) override;
    // cppcheck-suppress virtualCallInConstructor
    void Stop() override;
    void setDataLimit(unsigned long long int limitBytes);
    void setDataPeriod(unsigned int periodSeconds);
    bool checkDataPeriodHasElapsed();
    bool getDataLimitReached();
    unsigned long long int getDataLimit();
    void reloadTags();
    void setFoldingRules(const std::vector<Json::Value>& foldingRules);
    std::vector<Json::Value> getCurrentFoldingRules();
    bool compareFoldingRules(const std::vector<Json::Value>& newFoldingRules);
    std::vector<std::string> getFoldableQueries() const;
    int GetExitCode() override;

private:
    void Run(std::shared_ptr<std::atomic_bool> extensionFinished);
    ResultsSender m_resultsSender;
    bool m_stopped = { true };
    bool m_stopping = { false };
    std::unique_ptr<std::thread> m_runnerThread;
    OsquerySDK::Flags m_flags;
    std::unique_ptr<OsquerySDK::ExtensionInterface> m_extension;
    uintmax_t m_maxBatchBytes = 10000000;
    unsigned int m_maxBatchSeconds = 15;
    std::vector<Json::Value> m_foldingRules;
};