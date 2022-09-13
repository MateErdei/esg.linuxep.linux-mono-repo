//Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "IFanotifyHandler.h"

#include "datatypes/AutoFd.h"
#include "datatypes/ISystemCallWrapper.h"

namespace sophos_on_access_process::fanotifyhandler
{
    class FanotifyHandler : public IFanotifyHandler
    {
        public:
            explicit FanotifyHandler(datatypes::ISystemCallWrapperSharedPtr systemCallWrapper);
            ~FanotifyHandler();
            FanotifyHandler(const FanotifyHandler&) =delete;
            FanotifyHandler& operator=(const FanotifyHandler&) =delete;

            [[nodiscard]] int getFd() const;
            [[nodiscard]] int markMount(const unsigned int& flags, const uint64_t& mask, const int& dfd, const std::string& path) const;
            [[nodiscard]] int cacheFd(const unsigned int& flags, const uint64_t& mask, const int& dfd, const std::string& path) const;

        private:
            void processFaMarkError(const std::string& function) const;

            datatypes::AutoFd m_fd;
            datatypes::ISystemCallWrapperSharedPtr m_systemCallWrapper;
    };
}

