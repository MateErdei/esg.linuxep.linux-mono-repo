///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////
#ifndef EVEREST_BASE_WATCHDOG_MAIN_H
#define EVEREST_BASE_WATCHDOG_MAIN_H

namespace watchdog
{
    namespace watchdogimpl
    {
        class watchdog_main
        {
        public:
            static int main(int argc, char* argv[]);
        protected:
            int run();
            void read_plugin_configs();
        };
    }
}

#endif //EVEREST_BASE_WATCHDOG_MAIN_H
