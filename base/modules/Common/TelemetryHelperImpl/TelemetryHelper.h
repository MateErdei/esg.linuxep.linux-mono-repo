/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "TelemetryObject.h"

#include "Common/TelemetryHelper/ITelemetryHelper.h"
#include "Common/UtilityImpl/StringUtils.h"

#include <functional>
#include <mutex>
#include <string>
using namespace Common::Telemetry;

class TelemetryHelper : public Common::Telemetry::ITelemetryHelper
{
public:
    void set(const std::string& key, int value) override;

    void set(const std::string& key, unsigned int value) override;

    void set(const std::string& key, const std::string& value) override;

    void set(const std::string& key, const char* value) override;

    void set(const std::string& key, bool value) override;

    void increment(const std::string& key, int value) override;

    void increment(const std::string& key, unsigned int value) override;

    void append(const std::string& key, int value) override;

    void append(const std::string& key, unsigned int value) override;

    void append(const std::string& key, const std::string& value) override;

    void append(const std::string& key, const char* value) override;

    void append(const std::string& key, bool value) override;

    void registerResetCallback(std::string cookie, std::function<void()> function) override;
    void unregisterResetCallback(std::string cookie) override;
    void reset() override;
    std::string serialise();

private:
    TelemetryObject m_root;
    std::mutex m_dataLock;
    std::mutex m_callbackLock;
    std::map<std::string, std::function<void()>> m_callbacks;

    TelemetryObject& getTelemetryObjectByKey(const std::string& keyPath);
    void clearData();
};
