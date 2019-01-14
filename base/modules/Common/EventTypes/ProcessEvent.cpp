/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ProcessEvent.h"
#include "EventStrings.h"

#include <Common/EventTypes/IEventException.h>
#include <Common/EventTypes/CommonEventData.h>

#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include <Process.capnp.h>

#include <iostream>
#include <sstream>

namespace
{

    Sophos::Journal::ProcessEvent::EventType convertToCapnEventType(Common::EventTypes::ProcessEvent::EventType eventType)
    {
        switch(eventType)
        {
            case Common::EventTypes::ProcessEvent::EventType::start:
                return Sophos::Journal::ProcessEvent::EventType::START;
            case Common::EventTypes::ProcessEvent::EventType::end:
                return Sophos::Journal::ProcessEvent::EventType::END;
            default:
                throw Common::EventTypes::IEventException("Common::EventTypes::EventType, contained unknown type");
        }
    }

    Common::EventTypes::ProcessEvent::EventType convertFromCapnEventType(Sophos::Journal::ProcessEvent::EventType eventType)
    {
        switch(eventType)
        {
            case Sophos::Journal::ProcessEvent::EventType::START:
                return Common::EventTypes::ProcessEvent::EventType::start;
            case Sophos::Journal::ProcessEvent::EventType::END:
                return Common::EventTypes::ProcessEvent::EventType::end;
            default:
                throw Common::EventTypes::IEventException("Sophos::Journal::ProcessEvent::EventType, contained unknown type");
        }
    }
}

Common::EventTypes::ProcessEvent Common::EventTypes::createProcessEvent(Common::EventTypes::ProcessEvent::EventType eventType)
{
    Common::EventTypes::ProcessEvent event = ProcessEvent();
    event.setEventType(eventType);

    return event;
}

namespace Common
{
    namespace EventTypes
    {

        std::string ProcessEvent::toString() const
        {

            ::capnp::MallocMessageBuilder message;
            Sophos::Journal::ProcessEvent::Builder processEvent = message.initRoot<Sophos::Journal::ProcessEvent>();

            processEvent.setEventType(convertToCapnEventType(m_eventType));

            processEvent.setEndTime(m_endTime);
            processEvent.setFlags(m_flags);
            processEvent.setSessionId(m_sessionId);
            processEvent.setCmdLine(m_cmdLine);




            //processEvent.getFileSize().setValue(m_fileSize);

//            processEvent.get// getSubjectUserSID().setUsername(m_subjectUserSid.username);
//            processEvent.getSubjectUserSID().setDomain(m_subjectUserSid.domain);
//
//            processEvent.getTargetUserSID().setUsername(m_targetUserSid.username);
//            processEvent.getTargetUserSID().setDomain(m_targetUserSid.domain);
//
//            processEvent.getRemoteNetworkAddress().setAddress(m_remoteNetworkAccess.address);
//
//            processEvent.setLogonID(m_logonId);
//
//            processEvent.setUserGroupID(m_groupId);
//            processEvent.setUserGroupName(m_groupName);

            // Convert to byte string
            kj::Array<capnp::word> dataArray = capnp::messageToFlatArray(message);
            kj::ArrayPtr<kj::byte> bytes = dataArray.asBytes();
            std::string dataAsString(bytes.begin(), bytes.end());

            return dataAsString;
        }

        void ProcessEvent::fromString(const std::string& objectAsString)
        {

//            if(objectAsString.empty())
//            {
//                throw Common::EventTypes::IEventException("Invalid capn byte string, string is empty");
//            }
//
//            try
//            {
//                const kj::ArrayPtr<const capnp::word> view(
//                        reinterpret_cast<const capnp::word*>(&(*std::begin(objectAsString))),
//                        reinterpret_cast<const capnp::word*>(&(*std::end(objectAsString))));
//
//
//                capnp::FlatArrayMessageReader message(view);
//                Sophos::Journal::CredentialsEvent::Reader credentialsEvent = message.getRoot<Sophos::Journal::CredentialsEvent>();
//
//                setSessionType(convertFromCapnSessionType(credentialsEvent.getSessionType()));
//                setEventType(convertFromCapnEventType(credentialsEvent.getEventType()));
//
//                std::string groupName(credentialsEvent.getUserGroupName());
//                setGroupName(groupName);
//
//                setGroupId(credentialsEvent.getUserGroupID());
//                setTimestamp(credentialsEvent.getTimestamp());
//                setLogonId(credentialsEvent.getLogonID());
//
//                Common::EventTypes::UserSid subjectUserId;
//                subjectUserId.username = credentialsEvent.getSubjectUserSID().getUsername();;
//                subjectUserId.domain = credentialsEvent.getSubjectUserSID().getDomain();
//                setSubjectUserSid(subjectUserId);
//
//                Common::EventTypes::UserSid targetUserId;
//                targetUserId.username = credentialsEvent.getTargetUserSID().getUsername();
//                targetUserId.domain = credentialsEvent.getTargetUserSID().getDomain();
//                setTargetUserSid(targetUserId);
//
//                Common::EventTypes::NetworkAddress networkAddress;
//                networkAddress.address = credentialsEvent.getRemoteNetworkAddress().getAddress();
//                setRemoteNetworkAccess(networkAddress);
//            }
//            catch(std::exception& ex)
//            {
//                std::stringstream errorMessage;
//                errorMessage << "Error: failed to process capn CredentialEvent string, " << ex.what();
//                throw Common::EventTypes::IEventException(errorMessage.str());
//            }
        }

        const Common::EventTypes::ProcessEvent::EventType EventTypes::ProcessEvent::getEventType() const
        {
            return m_eventType;
        }

        const std::string ProcessEvent::getEventTypeId() const
        {
            return Common::EventTypes::ProcessEventName;
        }

        const unsigned long long ProcessEvent::getEndTime() const
        {
            return m_endTime;
        }

        const Common::EventTypes::OptionalUInt64 ProcessEvent::getFileSize() const
        {
            return m_fileSize;
        }

        const std::uint32_t ProcessEvent::getFlags() const
        {
            return m_flags;
        }

        const std::uint32_t ProcessEvent::getSessionId() const
        {
            return m_sessionId;
        }

//        const std::string ProcessEvent::getSId() const
//        {
//            return m_sid;
//        }

        const Common::EventTypes::PathName ProcessEvent::getPathName() const
        {
            return m_pathName;
        }

        const std::string ProcessEvent::getcmdLine() const
        {
            return m_cmdLine;
        }

        void ProcessEvent::setEventType(const ProcessEvent::EventType eventType)
        {

        }

        void ProcessEvent::setEndTime(const unsigned long long endTime)
        {

        }

        void ProcessEvent::setFileSize(const Common::EventTypes::OptionalUInt64 fileSize)
        {

        }

        void ProcessEvent::setFlags(const std::uint32_t flags)
        {

        }

        void ProcessEvent::setSessionId(const std::uint32_t sessionId)
        {

        }

        void ProcessEvent::setPathName(const Common::EventTypes::PathName pathName)
        {

        }

        void ProcessEvent::setCmdLine(const std::string cmdLine)
        {

        }
    }
}