// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#ifndef TEST_PUBLIC
# define TEST_PUBLIC private
#endif

#include "IFanotifyHandler.h"

#include "common/LockableData.h"
#include "datatypes/AutoFd.h"
#include "sophos_threat_detector/threat_scanner/IUpdateCompleteCallback.h"

#include "Common/SystemCallWrapper/ISystemCallWrapper.h"

namespace sophos_on_access_process::fanotifyhandler
{
    class FanotifyHandler : public IFanotifyHandler
    {
        public:
            explicit FanotifyHandler(Common::SystemCallWrapper::ISystemCallWrapperSharedPtr systemCallWrapper);
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
            [[nodiscard]] int markMount(const std::string& path) const override;

            /**
             * Unmark a mount point as unwanted - we do not want fanotify events from it.
             *
             * Called from Mount Monitor Thread.
             *
             * @param path
             * @return
             */
            [[nodiscard]] int unmarkMount(const std::string& path) const override;

            /**
             * Mark an FD to be cached - i.e. fanotify won't return events for it.
             *
             * Called from Scan Thread(s) after scanning file.
             *
             * @param dfd
             * @param path
             * @return
             */
            [[nodiscard]] int cacheFd(const int& dfd, const std::string& path) const override;

            /**
             * Mark an FD to be un-cached - i.e. fanotify will return events for it again.
             *
             * Called from Scan Thread(s) after scanning file.
             *
             * @param dfd
             * @param path
             * @return
             */
            int uncacheFd(const int& dfd, const std::string& path) const override;

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

            [[nodiscard]] int clearCachedFiles() const override;

        private:
            static void processFaMarkError(const std::string& function, const std::string& path);
            static int processFaMarkError(int result, const std::string& function, const std::string& path);

            mutable common::LockableData<datatypes::AutoFd> m_fd;
            const Common::SystemCallWrapper::ISystemCallWrapperSharedPtr m_systemCallWrapper;
    };
}

