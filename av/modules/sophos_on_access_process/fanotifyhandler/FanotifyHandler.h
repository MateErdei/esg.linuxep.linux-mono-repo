//Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "IFanotifyHandler.h"

#include "datatypes/AutoFd.h"
#include "datatypes/ISystemCallWrapper.h"
#include "sophos_threat_detector/threat_scanner/IUpdateCompleteCallback.h"

namespace sophos_on_access_process::fanotifyhandler
{
    class FanotifyHandler : public IFanotifyHandler, public threat_scanner::IUpdateCompleteCallback
    {
        public:
            explicit FanotifyHandler(datatypes::ISystemCallWrapperSharedPtr systemCallWrapper);
            ~FanotifyHandler() override;
            FanotifyHandler(const FanotifyHandler&) =delete;
            FanotifyHandler& operator=(const FanotifyHandler&) =delete;

            [[nodiscard]] int getFd() const override;
            [[nodiscard]] int markMount(const std::string& path) const override;
            [[nodiscard]] int unmarkMount(const std::string& path) const override;
            [[nodiscard]] int cacheFd(const int& dfd, const std::string& path) const override;
            [[nodiscard]] bool isInitialised() const override;

            /**
             * Initialise fanotify
             */
            void init() final;

            /**
             * Close fanotify descriptor
             */
            void close() final;

            void updateComplete() override;

            [[nodiscard]] int clearCachedFiles() const override;

        private:
            static void processFaMarkError(const std::string& function, const std::string& path);

            datatypes::AutoFd m_fd;
            datatypes::ISystemCallWrapperSharedPtr m_systemCallWrapper;
    };
}

