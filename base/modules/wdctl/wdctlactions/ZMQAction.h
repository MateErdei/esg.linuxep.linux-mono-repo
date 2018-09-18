/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "Action.h"

#include <Common/ZeroMQWrapper/IContextSharedPtr.h>
#include <Common/ZeroMQWrapper/ISocketRequesterPtr.h>
#include <Common/ZeroMQWrapper/IWritable.h>
#include <Common/ZeroMQWrapper/IReadable.h>

namespace wdctl
{
    namespace wdctlactions
    {
        class ZMQAction : public Action
        {
        public:
            explicit ZMQAction(const wdctl::wdctlarguments::Arguments& args);
        protected:
            Common::ZeroMQWrapper::IContextSharedPtr m_context;
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
            Common::ZeroMQWrapper::IReadable::data_t doOperationToWatchdog(const Common::ZeroMQWrapper::IWritable::data_t& arguments);

            bool isSuccessful(const Common::ZeroMQWrapper::IReadable::data_t&);
        };
    }
}
