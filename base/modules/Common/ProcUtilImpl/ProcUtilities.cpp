/******************************************************************************************************

Copyright 2018-2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "ProcUtilities.h"

#include "Logger.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/Process/IProcess.h>
#include <Common/FileSystemImpl/FilePermissionsImpl.h>

#include <signal.h>
#include <sstream>
#include <thread>
#include <unistd.h>


namespace
{
    std::optional<long int> convertTolong(const std::string& number)
    {
        std::stringstream numbStr(number);
        long int longInt;
        numbStr >> longInt;
        if (numbStr.fail())
        {
            return std::nullopt;
        }
        return longInt;
    }

    bool isProcessRunning(int pid)
    {
        auto contentOfProcstat = Common::FileSystem::fileSystem()->readProcFile(pid, "stat");
        if (!contentOfProcstat.has_value())
        {
            return false;
        }
        // there is no reason
        Proc::ProcStat procStatOfPid = Proc::parseProcStat(contentOfProcstat.value()).value();
        return procStatOfPid.state == Proc::ProcStat::ProcState::Active;
    }

    std::optional<std::string> readLink(const std::string& fullPath)
    {
        std::array<char, 4096> buffer;
        auto sizeExec = ::readlink(fullPath.c_str(), buffer.data(), buffer.size());
        if (sizeExec == -1 || sizeExec == 0)
        {
            return std::optional<std::string> {};
        }
        std::string linkPath { buffer.begin(), buffer.begin() + sizeExec };
        if (linkPath.at(0) != '/')
        {
            // not a full path, try to get the full path
            std::string fullPathLink = Common::FileSystem::join(Common::FileSystem::dirName(fullPath), linkPath);
            if (Common::FileSystem::fileSystem()->exists(fullPathLink))
            {
                return fullPathLink;
            }
        }
        return linkPath;
    }

} // namespace

std::optional<Proc::ProcStat> Proc::parseProcStat(const std::string& contentOfProcStat)
{
    static std::string finishedStates = "xXZ";

    std::stringstream sstream;
    sstream.str(contentOfProcStat);
    ProcStat procStat;

    sstream >> procStat.pid;
    sstream >> procStat.comm;
    char state;
    sstream >> state;
    if (finishedStates.find(state) != std::string::npos)
    {
        procStat.state = Proc::ProcStat::ProcState::Finished;
    }
    else
    {
        procStat.state = Proc::ProcStat::ProcState::Active;
    }

    sstream >> procStat.ppid;
    if (sstream.bad() || sstream.fail())
    {
        return std::optional<Proc::ProcStat> {};
    }
    return procStat;
}

std::vector<Proc::ProcStat> Proc::listAndFilterProcesses(std::function<bool(const Proc::ProcStat&)> predicate)
{
    std::vector<Proc::ProcStat> procStatList {};
    std::vector<std::string> entries = Common::FileSystem::fileSystem()->listFilesAndDirectories("/proc");
    for (const auto& entry : entries)
    {
        auto basename = Common::FileSystem::basename(entry);
        std::optional<long int> pid = convertTolong(basename);
        if (pid.has_value())
        {
            auto procFileContents =
                Common::FileSystem::fileSystem()->readProcFile(pid.value(), "stat");
            if (procFileContents.has_value())
            {
                std::optional<Proc::ProcStat> procStat = Proc::parseProcStat(procFileContents.value());
                if (procStat.has_value())
                {
                    if (predicate(procStat.value()))
                    {
                        procStatList.emplace_back(procStat.value());
                    }
                }
            }
        }
    }
    return procStatList;
}

std::vector<Proc::ProcStat> Proc::listProcesses()
{
    return listAndFilterProcesses([](const Proc::ProcStat&) { return true; });
}

std::vector<Proc::ProcStat> Proc::filterProcesses(
    const std::vector<Proc::ProcStat>& processes,
    std::function<bool(const Proc::ProcStat&)> filter)
{
    std::vector<ProcStat> response;
    for (auto& procStat : processes)
    {
        if (filter(procStat))
        {
            response.push_back(procStat);
        }
    }
    return response;
}

void Proc::killProcess(int pid)
{
    int count = 0;
    if (pid == -1 || pid == 1)
    {
        throw std::runtime_error("For safety reason it will refuse to kill init process");
    }
    if (::kill(pid, SIGTERM) == -1)
    {
        int err = errno;
        if (err == ESRCH)
        {
            return; // the process has already finished.
        }
        if (err == EPERM)
        {
            // the process can not send the signal, no reason to bother.
            throw std::runtime_error("This process can not kill the given target");
        }
        // the EINVAL should never occur.
    }
    while (isProcessRunning(pid) && ++count < 10)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    if (count == 10)
    {
        LOGINFO("Terminate kill will be send to pid: " << pid << " as it did not shutdown on SIGTERM");
        ::kill(pid, SIGKILL);
    }
    count = 0;
    while (isProcessRunning(pid) && count++ < 10)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
}

void Proc::ensureNoExecWithThisCommIsRunning(
    const std::string& partOfComm,
    const std::string& requiresFullPathContainsPath)
{
    LOGSUPPORT("Checking currently running executables that contain comm: " << partOfComm);
    auto sortByPid = [](const Proc::ProcStat& lh, const Proc::ProcStat& rh) { return lh.pid < rh.pid; };

    int count = 0;
    while (count++ < 10)
    {
        std::vector<ProcStat> runningEntries = listProcessesByComm(partOfComm, requiresFullPathContainsPath);

        if (runningEntries.empty())
        {
            break;
        }
        // sort by pid in order to kill the processes in the 'likely' order that they were created.
        std::sort(runningEntries.begin(), runningEntries.end(), sortByPid);
        size_t countNotActive = 0;
        for (auto& entry : runningEntries)
        {
            LOGSUPPORT("Found entry in proc table: " << entry.pid << " " << entry.comm);
            if (entry.state == ProcStat::ProcState::Active)
            {
                LOGINFO("Stopping process " << entry.comm << " pid: " << entry.pid);
                Proc::killProcess(entry.pid);
            }
            else
            {
                countNotActive++;
                LOGSUPPORT("Process terminated by pid available in /proc table " << entry.pid);
            }
        }
        if (countNotActive == runningEntries.size())
        {
            LOGSUPPORT("All the entries in the proc table are not really running");
            break;
        }
        // give time to kernel to clear /proc table.
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

std::vector<Proc::ProcStat> Proc::listProcessesByComm(
    const std::string& partOfComm,
    const std::string& requiresFullPathContainsPath)
{
    Proc::CommMatcher commMatcher { partOfComm };

    auto fullPathContains = [&requiresFullPathContainsPath](const ProcStat& commEntry) {
      if (requiresFullPathContainsPath.empty())
      {
          return true;
      }
      std::optional<std::string> fullPath = ::readLink("/proc/" + std::to_string(commEntry.pid) + "/exe");
      if (fullPath.has_value() && fullPath.value().find(requiresFullPathContainsPath) == 0)
      {
          return true;
      }
      return false;
    };

    std::vector<Proc::ProcStat> commEntries = Proc::listAndFilterProcesses(commMatcher);
    if (!commEntries.empty())
    {
        std::stringstream information;
        for (auto& entry : commEntries)
        {
            information << entry.comm << " " << entry.pid << "; ";
        }
        LOGSUPPORT("Match comm: " << information.str());
    }

    std::vector<ProcStat> runningEntries = Proc::filterProcesses(commEntries, fullPathContains);
    if (commEntries.size() != runningEntries.size())
    {
        std::stringstream information;
        for (auto& entry : runningEntries)
        {
            information << entry.comm << " " << entry.pid << "; ";
        }
        LOGSUPPORT("After path filtered: " << information.str());
    }
    return runningEntries;
}

bool Proc::CommMatcher::operator()(const std::string& comm) const
{
    if (comm.size() < 2)
    {
        return false;
    }
    std::string_view entryToMatch { m_comm };
    if (entryToMatch.size() > 15)
    {
        entryToMatch = entryToMatch.substr(0, 15);
    }

    std::string_view procComm { comm };
    procComm = procComm.substr(1, procComm.size() - 2);

    if (procComm.size() == 15)
    {
        return procComm == entryToMatch;
    }

    auto pos = procComm.find(entryToMatch);
    if (pos == std::string::npos || pos != 0)
    {
        return false;
    }
    // entry to match is in the beginning of the proc entry
    std::string_view restOfString = procComm.substr(pos + entryToMatch.size());
    if (restOfString.empty())
    {
        // both strings match.
        return true;
    }
    if (restOfString.at(0) == '.')
    {
        // simplification, but assumes it is a link here
        // and return true
        return true;
    }

    return false;
}

int Proc::getUserIdFromStatus(const long pid)
{
    int uid = -1;

    std::string pathRoot("/proc/");

    std::string pathString = Common::FileSystem::join(pathRoot, std::to_string(pid), "status");

    std::optional<std::string> content = Common::FileSystem::fileSystem()->readProcFile(pid, pathString);

    std::stringstream contentStream;
    contentStream << content.value();  //Convert to stream so that we can call getline on the content.
    std::string line;

    while(std::getline(contentStream, line))
    {
        if(line.find("Uid") != std::string::npos)
        {
            int iterator = 0; //position 0 of the line
            int tempUid = -1;
            int count = 0; // no digit found yet
            while (uid == -1)
            {
                if (isdigit(line[iterator])) //we found the first digit of the uid
                {
                    count++;
                    if (tempUid == -1)
                        tempUid = 0;
                    tempUid = tempUid * 10 + (line[iterator] - '0');
                }
                if (line[iterator] == '\t' && count > 0) //we found the real uid, which is the first number and we also finished reading it
                {
                    uid=tempUid;
                    break;
                }
                iterator++;
            }
        }
    }
    return uid; // will return -1 if nothing found
}

std::vector<int> Proc::listProcWithUserName(std::string& userName)
{
    std::vector<int> procList;
    std::vector<std::string> entries = Common::FileSystem::fileSystem()->listFilesAndDirectories("/proc");
    auto filePermissions = Common::FileSystem::filePermissions();
    int userId = filePermissions->getUserId(userName);

    for (const auto& entry : entries)
    {
        auto basename = Common::FileSystem::basename(entry);
        std::optional<long> pid = convertTolong(basename);
        if (pid.has_value())
        {
            // if uid = given uid we add it to vector
            if (Proc::getUserIdFromStatus(pid.value())==userId)
            {
                //renamed method now gets as argument status content aswell
                procList.emplace_back(pid.value());
            }
        }
    }
    return procList;
}

void Proc::killAllProcessesInProcList(std::vector<int>& procList)
{
    for (const auto& procPid : procList)
    {
        try
        {
            Proc::killProcess(procPid);

        }
        catch (std::runtime_error& ex)
        {
            LOGINFO( ex.what() );
        }

    }
}