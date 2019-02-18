/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

namespace Common
{
    namespace Reactor
    {
        class IShutdownListener
        {
        public:
            virtual ~IShutdownListener() = default;

            /**
             * Used to inform the listener that a shutdown request was made.  Which is usually a linux signal sent to
             * the process.
             */
            virtual void notifyShutdownRequested() = 0;
        };
    } // namespace Reactor
} // namespace Common
