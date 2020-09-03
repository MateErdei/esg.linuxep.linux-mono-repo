/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Logger.h"
#include "ScanResponse.h"

#include <capnp/message.h>
#include <capnp/serialize.h>

using namespace scan_messages;

ScanResponse::ScanResponse()
{
}

ScanResponse::ScanResponse(Sophos::ssplav::FileScanResponse::Reader reader)
{
    for (Sophos::ssplav::FileScanResponse::Detection::Reader detection : reader.getDetections())
    {
        m_detections.emplace_back(std::make_pair(detection.getFilePath(), detection.getThreatName()));
    }
    m_fullScanResult = reader.getFullScanResult();
    m_errorMsg = reader.getErrorMsg();
}

std::string ScanResponse::serialise() const
{
    ::capnp::MallocMessageBuilder message;
    Sophos::ssplav::FileScanResponse::Builder responseBuilder =
            message.initRoot<Sophos::ssplav::FileScanResponse>();

    ::capnp::List<Sophos::ssplav::FileScanResponse::Detection>::Builder detections = responseBuilder.initDetections(m_detections.size());
    for (unsigned int i=0; i < m_detections.size(); i++)
    {
        detections[i].setFilePath(m_detections[i].first);
        detections[i].setThreatName(m_detections[i].second);
    }
    responseBuilder.setFullScanResult(m_fullScanResult);
    responseBuilder.setErrorMsg(m_errorMsg);

    kj::Array<capnp::word> dataArray = capnp::messageToFlatArray(message);
    kj::ArrayPtr<kj::byte> bytes = dataArray.asBytes();
    std::string dataAsString(bytes.begin(), bytes.end());
    return dataAsString;
}

void ScanResponse::addDetection(std::string filePath, std::string threatName)
{
    m_detections.emplace_back(std::make_pair(filePath, threatName));
}

std::vector<std::pair<std::string, std::string>> ScanResponse::getDetections()
{
    return m_detections;
}

void ScanResponse::setFullScanResult(std::string threatName)
{
    m_fullScanResult = std::move(threatName);
}

void ScanResponse::setErrorMsg(std::string errorMsg)
{
    m_errorMsg = std::move(errorMsg);
}

std::string ScanResponse::getErrorMsg()
{
    return m_errorMsg;
}

bool ScanResponse::allClean()
{
    bool allClean = true;
    for (const auto& detection: m_detections)
    {
        if (!detection.second.empty())
        {
            allClean = false;
        }
    }
    return allClean;
}