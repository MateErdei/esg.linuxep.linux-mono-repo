//Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "IFanotifyHandler.h"
#include "OnaccessStatusFile.h"

#include "common/LockableData.h"
#include "datatypes/AutoFd.h"
#include "datatypes/ISystemCallWrapper.h"
#include "sophos_threat_detector/threat_scanner/IUpdateCompleteCallback.h"

namespace sophos_on_access_process::fanotifyhandler
{
    class FanotifyHandler : public IFanotifyHandler, public threat_scanner::IUpdateCompleteCallback
    {
        public:
            FanotifyHandler(datatypes::ISystemCallWrapperSharedPtr systemCallWrapper);
            ~FanotifyHandler() override;
            FanotifyHandler(const FanotifyHandler&) =delete;
            FanotifyHandler& operator=(const FanotifyHandler&) =delete;

            /**
             * Get the fanotify FD.
             * Logs error if not connected.
             *
             * Called from EventReader thread
             *
             * @return
             */
            [[nodiscard]] int getFd() const override;

            /**
             * Mark a mount point as wanted - we want fanotify events from it.
             *
             * Called from Mount Monitor Thread.
             * (will be) Called from Process Control Thread
             *
             * @param path
             * @return
             */
            [[nodiscard]] int markMount(const std::string& path) override;

            /**
             * Unmark a mount point as unwanted - we do not want fanotify events from it.
             *
             * Called from Mount Monitor Thread.
             *
             * @param path
             * @return
             */
            [[nodiscard]] int unmarkMount(const std::string& path) override;

            /**
             * Mark an FD to be cached - i.e. fanotify won't return events for it.
             *
             * Called from Scan Thread(s) after scanning file.
             *
             * @param dfd
             * @param path
             * @return
             */
            [[nodiscard]] int cacheFd(const int& dfd, const std::string& path) override;

            /**
             * Clear cached files.
             * Called after ThreatDetector has updated from updateClientThread
             */
            void updateComplete() override;

            /**
             * Initialise fanotify
             */
            void init() final;

            /**
             * Close fanotify descriptor
             */
            void close() final;

            [[nodiscard]] int clearCachedFiles() override;

        private:
            static int processFaMarkError(int result, const std::string& function, const std::string& path);
            static void processFaMarkError(const std::string& function, const std::string& path);

            mutable common::LockableData<datatypes::AutoFd> m_fd;
            datatypes::ISystemCallWrapperSharedPtr m_systemCallWrapper;
            OnaccessStatusFile m_statusFile;
    };
}

