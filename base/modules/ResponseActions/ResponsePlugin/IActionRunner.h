// Copyright 2023 Sophos Limited. All rights reserved.
#pragma once

#include <string>
namespace ResponsePlugin
{

    /**
     * This class is not thread-safe, so should be used from one thread only
     */
    class IActionRunner
    {
    public:
        virtual void runAction(
            const std::string& action,
            const std::string& correlationId,
            const std::string& type,
            int timeout) = 0;
        /**
         * If an action is running, killAction() must be called before destructing this class
         */
        virtual void killAction() = 0;
        virtual bool getIsRunning() = 0;

        virtual ~IActionRunner() = default;
    };

} // namespace ResponsePlugin
