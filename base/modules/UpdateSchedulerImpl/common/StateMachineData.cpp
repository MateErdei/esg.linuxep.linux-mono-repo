// Copyright 2021-2023 Sophos Limited. All rights reserved.

#include "StateMachineData.h"

#include "Logger.h"
#include "StateMachineException.h"

#include "Common/ProtobufUtil/MessageUtility.h"
#include "Common/UtilityImpl/StringUtils.h"
#include "UpdateSchedulerImpl/common/StateMachineData.pb.h"

#include <google/protobuf/util/json_util.h>

#include <sstream>

namespace UpdateSchedulerImpl::StateData
{
    StateMachineData::StateMachineData() :
        m_downloadFailedSinceTime("0"),
        m_downloadState("1"),
        m_downloadStateCredit("0"),
        m_eventStateLastError("0"),
        m_eventStateLastTime("0"),
        m_installFailedSinceTime("0"),
        m_installState("1"),
        m_installStateCredit("0"),
        m_lastGoodInstallTime("0"),
        m_canSendEvent(true)
    {
    }
    const std::string& StateMachineData::getDownloadFailedSinceTime() const
    {
        return m_downloadFailedSinceTime;
    }

    const std::string& StateMachineData::getDownloadState() const
    {
        return m_downloadState;
    }

    const std::string& StateMachineData::getDownloadStateCredit() const
    {
        return m_downloadStateCredit;
    }

    const std::string& StateMachineData::getEventStateLastError() const
    {
        return m_eventStateLastError;
    }

    const std::string& StateMachineData::getEventStateLastTime() const
    {
        return m_eventStateLastTime;
    }

    const std::string& StateMachineData::getInstallFailedSinceTime() const
    {
        return m_installFailedSinceTime;
    }

    const std::string& StateMachineData::getInstallState() const
    {
        return m_installState;
    }

    const std::string& StateMachineData::getInstallStateCredit() const
    {
        return m_installStateCredit;
    }

    const std::string& StateMachineData::getLastGoodInstallTime() const
    {
        return m_lastGoodInstallTime;
    }

    void StateMachineData::setDownloadFailedSinceTime(const std::string& downloadFailedSinceTime)
    {
        m_downloadFailedSinceTime = downloadFailedSinceTime;
    }

    void StateMachineData::setDownloadState(const std::string& downloadState)
    {
        m_downloadState = downloadState;
    }

    void StateMachineData::setDownloadStateCredit(const std::string& downloadStateCredit)
    {
        m_downloadStateCredit = downloadStateCredit;
    }

    void StateMachineData::setEventStateLastError(const std::string& eventStateLastError)
    {
        m_eventStateLastError = eventStateLastError;
    }

    void StateMachineData::setEventStateLastTime(const std::string& eventStateLastTime)
    {
        m_eventStateLastTime = eventStateLastTime;
    }

    void StateMachineData::setInstallFailedSinceTime(const std::string& installFailedSinceTime)
    {
        m_installFailedSinceTime = installFailedSinceTime;
    }

    void StateMachineData::setInstallState(const std::string& installState)
    {
        m_installState = installState;
    }

    void StateMachineData::setInstallStateCredit(const std::string& installStateCredit)
    {
        m_installStateCredit = installStateCredit;
    }

    void StateMachineData::setLastGoodInstallTime(const std::string& lastGoodInstallTime)
    {
        m_lastGoodInstallTime = lastGoodInstallTime;
    }
    void StateMachineData::setCanSendEvent(bool canSendEvent)
    {
        m_canSendEvent = canSendEvent;
    }

    bool StateMachineData::canSendEvent() const
    {
        return m_canSendEvent;
    }

    // add one return the json content directly.
    std::string StateMachineData::toJsonStateMachineData(const StateMachineData& stateMachineData)
    {
        StateMachineRawDataProto::StateMachineRawData protoStateMachine;
        protoStateMachine.set_downloadfailedsincetime(stateMachineData.getDownloadFailedSinceTime());
        protoStateMachine.set_downloadstate(stateMachineData.getDownloadState());
        protoStateMachine.set_downloadstatecredit(stateMachineData.getDownloadStateCredit());
        protoStateMachine.set_eventstatelasterror(stateMachineData.getEventStateLastError());
        protoStateMachine.set_eventstatelasttime(stateMachineData.getEventStateLastTime());
        protoStateMachine.set_installfailedsincetime(stateMachineData.getInstallFailedSinceTime());
        protoStateMachine.set_installstate(stateMachineData.getInstallState());
        protoStateMachine.set_installstatecredit(stateMachineData.getInstallStateCredit());
        protoStateMachine.set_lastgoodinstalltime(stateMachineData.getLastGoodInstallTime());
        protoStateMachine.set_cansendevent(stateMachineData.canSendEvent());

        return Common::ProtobufUtil::MessageUtility::protoBuf2Json(protoStateMachine);
    }

    StateMachineData StateMachineData::fromJsonStateMachineData(const std::string& serializedVersion)
    {
        using namespace google::protobuf::util;
        using StateMachineRawDataProto::StateMachineRawData;

        StateMachineRawDataProto::StateMachineRawData protoStateMachine;
        JsonParseOptions jsonParseOptions;
        jsonParseOptions.ignore_unknown_fields = true;

        auto status = JsonStringToMessage(serializedVersion, &protoStateMachine, jsonParseOptions);
        if (!status.ok())
        {
            LOGWARN("Failed to load state machine state serialized string");
            LOGSUPPORT(status.ToString());
            throw StateMachineException("Failed to process json message");
        }

        UpdateSchedulerImpl::StateData::StateMachineData stateMachineData;

        // When reading values, make sure numerical based values can be converted.  This is to prevent
        // storing in-valid data in the first place.

        stateMachineData.m_downloadFailedSinceTime = std::to_string(
            Common::UtilityImpl::StringUtils::stringToLong(protoStateMachine.downloadfailedsincetime()).first);
        stateMachineData.m_downloadState =
            std::to_string(Common::UtilityImpl::StringUtils::stringToInt(protoStateMachine.downloadstate()).first);
        stateMachineData.m_downloadStateCredit = std::to_string(
            Common::UtilityImpl::StringUtils::stringToInt(protoStateMachine.downloadstatecredit()).first);
        stateMachineData.m_eventStateLastError = protoStateMachine.eventstatelasterror();
        stateMachineData.m_eventStateLastTime = std::to_string(
            Common::UtilityImpl::StringUtils::stringToLong(protoStateMachine.eventstatelasttime()).first);
        stateMachineData.m_installFailedSinceTime = std::to_string(
            Common::UtilityImpl::StringUtils::stringToLong(protoStateMachine.installfailedsincetime()).first);
        stateMachineData.m_installState =
            std::to_string(Common::UtilityImpl::StringUtils::stringToInt(protoStateMachine.installstate()).first);
        stateMachineData.m_installStateCredit =
            std::to_string(Common::UtilityImpl::StringUtils::stringToInt(protoStateMachine.installstatecredit()).first);
        stateMachineData.m_lastGoodInstallTime = std::to_string(
            Common::UtilityImpl::StringUtils::stringToLong(protoStateMachine.lastgoodinstalltime()).first);
        stateMachineData.m_canSendEvent = protoStateMachine.cansendevent();

        return stateMachineData;
    }
} // namespace UpdateSchedulerImpl::StateData