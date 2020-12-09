/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <wdctl/wdctlarguments/Arguments.h>

namespace wdctl
{
    namespace wdctlactions
    {
        class Action
        {
        public:
            using Arguments = wdctl::wdctlarguments::Arguments;
            /**
             *
             * @param args BORROWED reference to arguments
             */
            explicit Action(const Arguments& args);
            Action(const Action&) = delete;

            virtual ~Action() = default;

            virtual int run() = 0;

            void setSkipWatchdogDetection() { m_detectWatchdog = false; }

        protected:
            const Arguments& m_args;
            bool detectWatchdog() const { return m_detectWatchdog; }

        private:
            bool m_detectWatchdog = true;
        };
    } // namespace wdctlactions
} // namespace wdctl
