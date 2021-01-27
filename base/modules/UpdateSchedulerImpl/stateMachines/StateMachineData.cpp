/******************************************************************************************************

Copyright 2018-2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

//#include "DownloadedProduct.h"
//#include "IWarehouseRepository.h"
#include "StateMachineData.h"

#include "Logger.h"
#include "StateMachineException.h"
//#include "TimeTracker.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/ProtobufUtil/MessageUtility.h>
#include <Common/UtilityImpl/StringUtils.h>
#include <Common/UtilityImpl/TimeUtils.h>
#include <google/protobuf/util/json_util.h>
#include <StateMachineData.pb.h>
#include <sstream>

namespace UpdateScheduler
{
    namespace StateData
    {
        using namespace Common::UtilityImpl;



        const std::string& StateMachineData::getDownloadFailedSinceTime() const
        {
            return m_downloadFailedSinceTime;
        }

        const std::string& StateMachineData::getDownloadState() const { return m_downloadState; }

        const std::string& StateMachineData::getDownloadStateCredit() const { return m_downloadStateCredit; }

        const std::string& StateMachineData::getEventStateLastError() const { return m_eventStateLastError; }

        const std::string& StateMachineData::getEventStateLastTime() const { return m_eventStateLastTime; }

        const std::string& StateMachineData::getInstallFailedSinceTime() const { return m_installFailedSinceTime; }

        const std::string& StateMachineData::getInstallState() const { return m_installState; }

        const std::string& StateMachineData::getInstallStateCredit() const { return m_installStateCredit; }

        const std::string& StateMachineData::getLastGoodInstallTime() const { return m_lastGoodInstallTime; }

        void StateMachineData::setDownloadFailedSinceTime( const std::string& downloadFailedSinceTime)
        {
            m_downloadFailedSinceTime = downloadFailedSinceTime;
        }

        void StateMachineData::setDownloadState( const std::string& downloadState)
        {
            m_downloadState = downloadState;
        }

        void StateMachineData::setDownloadStateCredit( const std::string& downloadStateCredit)
        {
            m_downloadStateCredit = downloadStateCredit;
        }

        void StateMachineData::setEventStateLastError( const std::string& eventStateLastError)
        {
            m_eventStateLastError = eventStateLastError;
        }

        void StateMachineData::setEventStateLastTime( const std::string& eventStateLastTime)
        {
            m_eventStateLastTime = eventStateLastTime;
        }

        void StateMachineData::setInstallFailedSinceTime( const std::string& installFailedSinceTime)
        {
            m_installFailedSinceTime = installFailedSinceTime;
        }

        void StateMachineData::setInstallState( const std::string& installState)
        {
            m_installState = installState;
        }

        void StateMachineData::setInstallStateCredit( const std::string& installStateCredit)
        {
            m_installStateCredit = installStateCredit;
        }

        void StateMachineData::setLastGoodInstallTime( const std::string& lastGoodInstallTime)
        {
            m_lastGoodInstallTime = lastGoodInstallTime;
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
                    LOGERROR("Failed to load state machine state serialized string");
                    LOGSUPPORT(status.ToString());
                    throw StateMachineException("Failed to process json message");
                }

                UpdateScheduler::StateData::StateMachineData stateMachineData;
                stateMachineData.m_downloadFailedSinceTime = protoStateMachine.downloadfailedsincetime();
                stateMachineData.m_downloadState = protoStateMachine.downloadstate();
                stateMachineData.m_downloadStateCredit = protoStateMachine.downloadstatecredit();
                stateMachineData.m_eventStateLastError = protoStateMachine.eventstatelasterror();
                stateMachineData.m_eventStateLastTime = protoStateMachine.eventstatelasttime();
                stateMachineData.m_installFailedSinceTime = protoStateMachine.installfailedsincetime();
                stateMachineData.m_installState = protoStateMachine.installstate();
                stateMachineData.m_installStateCredit = protoStateMachine.installstatecredit();
                stateMachineData.m_lastGoodInstallTime = protoStateMachine.lastgoodinstalltime();

                return stateMachineData;
            }

        //    std::tuple<int, std::string> DownloadReport::CodeAndSerialize(const DownloadReport& report)
        //    {
        //        std::string json = DownloadReport::fromReport(report);
        //        return std::tuple<int, std::string>(report.getExitCode(), json);
        //    }


    } // namespace StateData
} // namespace SulDownloader