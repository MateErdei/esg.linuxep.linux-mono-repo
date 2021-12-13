/******************************************************************************************************

Copyright 2020-2021, Sophos Limited.  All rights reserved.

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
        Detection detectionContainer;
        detectionContainer.name = detection.getThreatName();
        detectionContainer.path = detection.getFilePath();
        detectionContainer.sha256 = detection.getSha256();
        m_detections.emplace_back(detectionContainer);
    }
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
        detections[i].setFilePath(m_detections[i].path);
        detections[i].setThreatName(m_detections[i].name);
        detections[i].setSha256(m_detections[i].sha256);
    }
    responseBuilder.setErrorMsg(m_errorMsg);

    kj::Array<capnp::word> dataArray = capnp::messageToFlatArray(message);
    kj::ArrayPtr<kj::byte> bytes = dataArray.asBytes();
    std::string dataAsString(bytes.begin(), bytes.end());
    return dataAsString;
}

void ScanResponse::addDetection(const std::string& filePath, const std::string& threatName, const std::string& sha256)
{
    Detection detection;
    detection.path = filePath;
    detection.name = threatName;
    detection.sha256 = sha256;
    m_detections.emplace_back(detection);
}

std::vector<Detection> ScanResponse::getDetections()
{
    return m_detections;
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
        if (!detection.name.empty())
        {
            allClean = false;
        }
    }
    return allClean;
}