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

        virtual int getFd() const = 0;
        virtual int markMount(const unsigned int& flags, const uint64_t& mask, const int& dfd, const std::string& path) const = 0;
        virtual int cacheFd(const unsigned int& flags, const uint64_t& mask, const int& dfd, const std::string& path) const = 0;
    };

    using IFanotifyHandlerSharedPtr = std::shared_ptr<IFanotifyHandler>;
}