/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ScanReport.h"

Example::ScanReport::ScanReport(time_t startTime)
: m_startTime(startTime != 0 ? startTime : now()), m_finishTime(), m_totalMemory(0), m_noFilesScanned(0)
{

}

void Example::ScanReport::scanFinished(time_t endTime)
{
    m_finishTime = endTime != 0 ? endTime : now();
}

void Example::ScanReport::reportInfection(const std::string &filePath) {
    m_infections.emplace_back(filePath);
}

void Example::ScanReport::reportNewFileScanned(double fileSize) {
    ++m_noFilesScanned;
    m_totalMemory += fileSize;
}

time_t Example::ScanReport::getStartTime() const {
    return m_startTime;
}

time_t Example::ScanReport::getFinishTime() const {
    return m_finishTime;
}

int Example::ScanReport::getNoFilesScanned() const {
    return m_noFilesScanned;
}

double Example::ScanReport::getTotalMemoryMB() const {
    return m_totalMemory;
}

const std::vector<std::string> &Example::ScanReport::getInfections() const {
    return m_infections;
}

time_t Example::ScanReport::now() const
{
    return std::time(nullptr);
}

