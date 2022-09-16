//Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "datatypes/AutoFd.h"
#include "datatypes/ISystemCallWrapper.h"
#include "sophos_threat_detector/threat_scanner/IUpdateCompleteCallback.h"

namespace sophos_on_access_process::fanotifyhandler
{
    class FanotifyHandler : public threat_scanner::IUpdateCompleteCallback
    {
        public:
            explicit FanotifyHandler(datatypes::ISystemCallWrapperSharedPtr systemCallWrapper);
            ~FanotifyHandler();
            FanotifyHandler(const FanotifyHandler&) =delete;
            FanotifyHandler& operator=(const FanotifyHandler&) =delete;

            [[nodiscard]] int getFd() const;

            /**
             * Initialise fanotify
             */
            void init() final;

            /**
             * Close fanotify descriptor
             */
            void close() final;

            void updateComplete() override;

        private:
            int clearCachedFiles() const;
            void processFaMarkError(const std::string& function) const;

            datatypes::AutoFd m_fd;
            datatypes::ISystemCallWrapperSharedPtr m_systemCallWrapper;
    };
}

