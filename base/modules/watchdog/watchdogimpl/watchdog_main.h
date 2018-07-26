///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////
#pragma once


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


