/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "TelemetryObject.h"

#include <Common/TelemetryHelper/ITelemetryHelper.h>
#include <Common/UtilityImpl/StringUtils.h>

#include <functional>
#include <mutex>
#include <string>
using namespace Common::Telemetry;

class TelemetryHelper : public ITelemetryHelper
{
public:
    static TelemetryHelper& getInstance()
    {
        static TelemetryHelper instance;
        return instance;
    }

    TelemetryHelper(TelemetryHelper const&) = delete;

    void operator=(TelemetryHelper const&) = delete;

    void set(const std::string& key, int value) override;
    void set(const std::string& key, unsigned int value) override;
    void set(const std::string& key, const std::string& value) override;
    void set(const std::string& key, const char* value) override;
    void set(const std::string& key, bool value) override;

    void increment(const std::string& key, int value) override;
    void increment(const std::string& key, unsigned int value) override;

    void appendValue(const std::string& arrayKey, int value) override;
    void appendValue(const std::string& arrayKey, unsigned int value) override;
    void appendValue(const std::string& arrayKey, const std::string& value) override;
    void appendValue(const std::string& arrayKey, const char* value) override;
    void appendValue(const std::string& arrayKey, bool value) override;

    void appendObject(const std::string& arrayKey, const std::string& key, int value) override;
    void appendObject(const std::string& arrayKey, const std::string& key, unsigned int value) override;
    void appendObject(const std::string& arrayKey, const std::string& key, const std::string& value) override;
    void appendObject(const std::string& arrayKey, const std::string& key, const char* value) override;
    void appendObject(const std::string& arrayKey, const std::string& key, bool value) override;

    void mergeJsonIn(const std::string& key, const std::string& json) override;

    TelemetryObject& getTelemetryObjectByKey(const std::string& keyPath) override;

    void registerResetCallback(std::string cookie, std::function<void()> function) override;
    void unregisterResetCallback(std::string cookie) override;
    void reset() override;

    std::string serialise();

    // Normally with a singleton the constructor is private but here we make the constructor public
    // so that plugins can instantiate multiple Telemetry Helpers and not share a root data structure if they want to.
    TelemetryHelper() = default;
private:
    TelemetryObject m_root;
    std::mutex m_dataLock;
    std::mutex m_callbackLock;

    std::map<std::string, std::function<void()>> m_callbacks;

    template<class T>
    void setInternal(const std::string& key, T value);

    template<class T>
    void incrementInternal(const std::string& key, T value);

    template<class T>
    void appendValueInternal(const std::string& key, T value);

    template<class T>
    void appendObjectInternal(const std::string& arrayKey, const std::string& key, T value);
    void clearData();
};
