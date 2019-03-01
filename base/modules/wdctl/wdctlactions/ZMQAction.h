/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Action.h"

#include <Common/ZMQWrapperApi/IContextSharedPtr.h>
#include <Common/ZeroMQWrapper/IReadable.h>
#include <Common/ZeroMQWrapper/ISocketRequesterPtr.h>
#include <Common/ZeroMQWrapper/IWritable.h>

namespace wdctl
{
    namespace wdctlactions
    {
        class ZMQAction : public Action
        {
        public:
            explicit ZMQAction(const wdctl::wdctlarguments::Arguments& args);

        protected:
            Common::ZMQWrapperApi::IContextSharedPtr m_context;
            /**
             * Create a requester socket to the watchdog
             *
             * @return a connected socket
             */
            Common::ZeroMQWrapper::ISocketRequesterPtr connectToWatchdog();

            /**
             * Do an operation against the watchdog.
             * @param arguments
             * @return response from watchdog or error message in [0] for failures
             */
            Common::ZeroMQWrapper::IReadable::data_t doOperationToWatchdog(
                const Common::ZeroMQWrapper::IWritable::data_t& arguments);

            bool isSuccessful(const Common::ZeroMQWrapper::IReadable::data_t&);
        };
    } // namespace wdctlactions
} // namespace wdctl
