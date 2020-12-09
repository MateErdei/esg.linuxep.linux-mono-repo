/******************************************************************************************************

Copyright 2018-2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <functional>
#include <optional>
#include <string>

namespace Proc
{
    /// Related to the content of /proc/pid/stat
    struct ProcStat
    {
        /* pid: process id*/
        int pid;
        enum class ProcState
        {
            Active,
            Finished
        };
        /** translate the state of /proc/pid/stat to eihter Finished or NotFinished. Finished means the state is one of
         * following:
         *   * Z zombie
         *   * X Dead
         *   * x dead
         */
        ProcState state;
        /** Parent pid*/
        int ppid;
        /** Filename of the executable*/
        std::string comm;
    };

    /** Parse the content of /proc/pid/stat and return the ProcStat as optional for cases where the parse can not suceed
     * This method does not aim to be generic to work for all possible cases, but to be capable of correctly parsing the
     * sort of values expected from the stat file for osquery or SophosMTR.
     *
     * */
    std::optional<ProcStat> parseProcStat(const std::string& contentOfProcStat);

    void killProcess(int pid);

    int getUserIdFromStatus(const long pid);

    std::vector<int> listProcWithUserName(std::string& userName);

    void killAllProcessesInProcList(std::vector<int>& procList);

} // namespace Proc
