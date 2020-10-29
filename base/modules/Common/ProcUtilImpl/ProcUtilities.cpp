/******************************************************************************************************

Copyright 2018-2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "ProcUtilities.h"

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
        ::kill(pid, SIGKILL);
    }
    count = 0;
    while (isProcessRunning(pid) && count++ < 10)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
}

int Proc::getUserIdFromStatus(const long pid)
{
    int uid = -1;

    std::string pathRoot("/proc/");

    std::string pathString = Common::FileSystem::join(pathRoot, std::to_string(pid), "status");

    std::optional<std::string> content = Common::FileSystem::fileSystem()->readProcFile(pid, pathString);

    if(content == std::nullopt)
    {
        return -1;
    }


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
            // Continue
        }

    }
}