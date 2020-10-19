/******************************************************************************************************

Copyright 2018-2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <functional>
#include <optional>
#include <string>
namespace ProcessUtils
{
    /** Return the content of
     * /proc/<pid>/<filename>
     * Given the nature of proc files being ephemeral, it returns optional which means that if the value is not
     * available or it fail to read the content, it will return an empty optional value.
     *
     * Look that fileSystem()->readContent can not be used as it tries to hold the size of the file which is not valid
     * for /proc files.
     * @param pid
     * @param filename
     * @return
     */
    std::optional<std::string> readProcFile(int pid, const std::string& filename);

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

    class CommMatcher
    {
        std::string m_comm;

    public:
        CommMatcher(const std::string& filename) : m_comm(filename) {};
        bool operator()(const std::string& comm) const;
        bool operator()(const ProcStat& procStat) const
        {
            return operator()(procStat.comm);
        }
    };

    /** Parse the content of /proc/pid/stat and return the ProcStat as optional for cases where the parse can not suceed
     * This method does not aim to be generic to work for all possible cases, but to be capable of correctly parsing the
     * sort of values expected from the stat file for osquery or SophosMTR.
     *
     * */
    std::optional<ProcStat> parseProcStat(const std::string& contentOfProcStat);

    std::vector<ProcStat> listAndFilterProcesses(std::function<bool(const ProcessUtils::ProcStat&)> predicate);

    std::vector<ProcStat> filterProcesses(const std::vector<ProcStat>&, std::function<bool(const ProcStat&)> filter);

    void killProcess(int pid);

    std::vector<ProcStat> listProcessesByComm(
        const std::string& partOfComm,
        const std::string& requiresFullPathContainsPath);

    /**
     *
     * @param partOfComm, found at /proc/<pid>/comm
     * @param requiresFullPathContainsPath for each candidate that matches the comm it will require that the full path
     * contains this string. This is to avoid the possible side effect of killing processes that have same name as the
     * one of the partOfComm. Example: killing other instances of osquery for example that are not managed by
     * sophos-spl.
     */
    void ensureNoExecWithThisCommIsRunning(
        const std::string& partOfComm,
        const std::string& requiresFullPathContainsPath);

} // namespace ProcessUtils
