/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <string>
#include "TelemetryObject.h"
#include "Common/UtilityImpl/StringUtils.h"
#include <mutex>
#include "Common/TelemetryHelper/ITelemetryHelper.h"
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

    std::string serialise();

private:
    TelemetryObject m_root;
    std::mutex m_lock;

    TelemetryObject& getTelemetryObjectByKey(const std::string& keyPath);

};



