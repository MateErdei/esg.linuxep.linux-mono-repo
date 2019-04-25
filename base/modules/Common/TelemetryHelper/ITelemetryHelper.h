/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include <vector>
#include <functional>

namespace Common::Telemetry
{
    class ITelemetryHelper
    {
    public:
        virtual ~ITelemetryHelper() = default;

        virtual void set(const std::string& key, int value) = 0;
        virtual void set(const std::string& key, unsigned int value) = 0;
        virtual void set(const std::string& key, const std::string& value) = 0;
        virtual void set(const std::string& key, const char* value) = 0;
        virtual void set(const std::string& key, bool value) = 0;

        virtual void increment(const std::string& key, int value) = 0;
        virtual void increment(const std::string& key, unsigned int value) = 0;

        virtual void append(const std::string& key, int value) = 0;
        virtual void append(const std::string& key, unsigned int value) = 0;
        virtual void append(const std::string& key, const std::string& value) = 0;
        virtual void append(const std::string& key, const char* value) = 0;
        virtual void append(const std::string& key, bool value) = 0;
        virtual void registerResetCallback(std::string cookie, std::function<void()> function) = 0;
        virtual void unregisterResetCallback(std::string cookie) = 0;
        virtual void reset() = 0;

    };
}