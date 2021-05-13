/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ShutdownTimer.h"

#include "Logger.h"

#include <thirdparty/nlohmann-json/json.hpp>
#include <fstream>

using json = nlohmann::json;
using namespace sspl::sophosthreatdetectorimpl;

ShutdownTimer::ShutdownTimer(fs::path configFile)
: m_configFile(configFile)
{
    reset();
}

void ShutdownTimer::reset()
{
    m_scanStartTime = getCurrentEpoch();
    LOGDEBUG("Reset timer to: " << m_scanStartTime);
}

long ShutdownTimer::timeout()
{
    long shutdownTime = m_scanStartTime + getDefaultTimeout();
    LOGDEBUG("Shutdown due at: " << shutdownTime);
    return shutdownTime - getCurrentEpoch();
}

long ShutdownTimer::getDefaultTimeout()
{
    long defaultTimeout = 60 * 60;
    std::ifstream fs(m_configFile, std::ifstream::in);

    if (fs.good())
    {
        try
        {
            std::stringstream settingsString;
            settingsString << fs.rdbuf();

            auto settingsJson = json::parse(settingsString.str());
            defaultTimeout = settingsJson["shutdownTimeout"];
        }
        catch (const std::exception& e)
        {
            LOGERROR("Unexpected error when reading threat detector config: " << e.what());
        }
    }
    LOGDEBUG("Default shutdown timeout set to " << defaultTimeout << " seconds.");
    return defaultTimeout;
}

long ShutdownTimer::getCurrentEpoch()
{
    auto now = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::seconds>(now).count();
}
