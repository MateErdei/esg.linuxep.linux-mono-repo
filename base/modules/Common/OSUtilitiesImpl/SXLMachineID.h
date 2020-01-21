/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <string>

namespace Common
{
    namespace OSUtilitiesImpl
    {
        class SXLMachineID
        {
        public:
            /**
             * Grab the machine ID defined at INSTALL_ROOT/base/etc/machine_id.txt
             * @return return the content of the file or empty string
             */
            std::string getMachineID();
            /**
             * Write to file machine ID
             */
            void createMachineIDFile();

            /**
             * Create a machine id that is composed by: a fixed prefix: sspl-machineid + list of macs.
             * The calculated md5 of the resulted string.
             * @return machine id string
             */
            std::string generateMachineID();

        private:
            std::string machineIDPath() const;
        };

        /**
         * Receives as first argument SOPHOS_INSTALL.
         * Verify if the machine id exists. If it does not, create.
         *
         * @param argc
         * @param argv: program-name , sophos_install
         * @return 0 for success, otherwise error code
         */
        int mainEntry(int argc, char* argv[]);
    } // namespace OSUtilitiesImpl
} // namespace Common
