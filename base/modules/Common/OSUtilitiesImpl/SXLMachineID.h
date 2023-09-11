// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include <string>

namespace Common::OSUtilitiesImpl
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
} // namespace Common::OSUtilitiesImpl
