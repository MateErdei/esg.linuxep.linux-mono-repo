//Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include <memory>
#include <string>

#include <sys/fanotify.h>

namespace sophos_on_access_process::fanotifyhandler
{
    class IFanotifyHandler
    {
    public:
        virtual ~IFanotifyHandler() = default;

        [[nodiscard]] virtual int getFd() const = 0;
        [[nodiscard]] virtual int markMount(const std::string& path) = 0;
        [[nodiscard]] virtual int unmarkMount(const std::string& path) = 0;
        [[nodiscard]] virtual int cacheFd(const int& fd, const std::string& path) = 0;
        [[nodiscard]] virtual int clearCachedFiles() = 0;

        /**
         * Initialise fanotify
         */
        virtual void init() = 0;

        /**
         * Close fanotify descriptor
         */
        virtual void close() = 0;
    };

    using IFanotifyHandlerSharedPtr = std::shared_ptr<IFanotifyHandler>;
}
