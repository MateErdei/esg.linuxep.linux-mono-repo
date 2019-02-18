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

        protected:
            const Arguments& m_args;
        };
    } // namespace wdctlactions
} // namespace wdctl
