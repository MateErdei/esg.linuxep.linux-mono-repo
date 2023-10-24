// Copyright 2020-2023 Sophos Limited. All rights reserved.

#pragma once

#include "IServiceExtension.h"
#include "ResultsSender.h"
#include "BatchTimer.h"

#include "Common/Threads/LockableData.h"

#ifdef SPL_BAZEL
#include "common/livequery/include/OsquerySDK/OsquerySDK.h"
#else
#include "OsquerySDK/OsquerySDK.h"
#endif

#include <boost/thread.hpp>

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
    ~LoggerExtension() override;
    void Start(const std::string& socket, bool verbose, std::shared_ptr<std::atomic_bool> extensionFinished) override;
    void Stop(long timeoutSeconds) final;
    void setDataLimit(unsigned long long int limitBytes);
    void setMTRLicense(bool hasMTRLicense);
    void setDataPeriod(unsigned int periodSeconds);
    bool checkDataPeriodHasElapsed();
    bool getDataLimitReached();
    unsigned long long int getDataLimit();
    void reloadTags();
    void setFoldingRules(const std::vector<Json::Value>& foldingRules);
    std::vector<Json::Value> getCurrentFoldingRules();
    bool compareFoldingRules(const std::vector<Json::Value>& newFoldingRules);
    [[nodiscard]] std::vector<std::string> getFoldableQueries() const;
    int GetExitCode() override;

private:
    void Run(const std::shared_ptr<std::atomic_bool>& extensionFinished);
    ResultsSender m_resultsSender;
    BatchTimer m_batchTimer;
    bool m_stopped = true;
    Common::Threads::LockableData<bool> stopping_{false};
    std::unique_ptr<boost::thread> m_runnerThread;
    OsquerySDK::Flags m_flags;
    std::unique_ptr<OsquerySDK::ExtensionInterface> m_extension;
    uintmax_t m_maxBatchBytes = 10000000;
    unsigned int m_maxBatchSeconds = 15;
    std::vector<Json::Value> m_foldingRules;

    [[nodiscard]] bool stopping()
    {
        auto locked = stopping_.lock();
        return *locked;
    }

    void stopping(bool value)
    {
        auto locked = stopping_.lock();
        *locked = value;
    }
};