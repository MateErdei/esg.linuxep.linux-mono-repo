// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "sophos_threat_detector/threat_scanner/IUpdateCompleteCallback.h"

#include <memory>
#include <string>

namespace sophos_on_access_process::fanotifyhandler
{
    class IFanotifyHandler : public threat_scanner::IUpdateCompleteCallback
    {
    public:
        [[nodiscard]] virtual int getFd() const = 0;
        [[nodiscard]] virtual int markMount(const std::string& path, bool onOpen, bool onClose) = 0;
        [[nodiscard]] virtual int unmarkMount(const std::string& path) = 0;

        /**
         * Mark an FD to be cached - i.e. fanotify won't return events for it.
         *
         * Called from Scan Thread(s) after scanning file.
         * Called from EventReaderThread after excluding a file path
         *
         * @param fd
         * @param path
         * @param surviveModify Should the mark survive file modifications - e.g. for exclusions
         * @return fanotify_mark result: 0 for success
         */
        [[nodiscard]] virtual int cacheFd(const int& fd, const std::string& path, bool surviveModify) const = 0;
        [[nodiscard]] virtual int uncacheFd(const int& fd, const std::string& path) const = 0;
        [[nodiscard]] virtual int clearCachedFiles() const = 0;

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
