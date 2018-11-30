/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "CredentialEvent.h"

#include <Common/EventTypes/IEventException.h>
#include <Common/EventTypes/ICredentialEvent.h>
#include <Common/EventTypes/CommonEventData.h>

#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include <Credentials.capnp.h>

#include <iostream>
#include <sstream>

namespace
{

    Sophos::Journal::CredentialsEvent::SessionType convertToCapnSessionType(Common::EventTypes::SessionType sessionType)
    {
        switch(sessionType)
        {
            case Common::EventTypes::SessionType::interactive:
                return Sophos::Journal::CredentialsEvent::SessionType::INTERACTIVE;
            case Common::EventTypes::SessionType::network:
                return Sophos::Journal::CredentialsEvent::SessionType::NETWORK;
            case Common::EventTypes::SessionType::networkInteractive:
                return Sophos::Journal::CredentialsEvent::SessionType::NETWORK_INTERACTIVE;
            default:
                throw Common::EventTypes::IEventException("Common::EventTypes::SessionType, contained unknown type");
        }
    }

    Sophos::Journal::CredentialsEvent::EventType convertToCapnEventType(Common::EventTypes::EventType eventType)
    {
        switch(eventType)
        {
            case Common::EventTypes::EventType::authFailure:
                return Sophos::Journal::CredentialsEvent::EventType::AUTH_FAILURE;
            case Common::EventTypes::EventType::authSuccess:
                return Sophos::Journal::CredentialsEvent::EventType::AUTH_SUCCESS;
            case Common::EventTypes::EventType::created:
                return Sophos::Journal::CredentialsEvent::EventType::CREATED;
            case Common::EventTypes::EventType::deleted:
                return Sophos::Journal::CredentialsEvent::EventType::DELETED;
            case Common::EventTypes::EventType::passwordChange:
                return Sophos::Journal::CredentialsEvent::EventType::PASSWORD_CHANGE;
            case Common::EventTypes::EventType::membershipChange:
                return Sophos::Journal::CredentialsEvent::EventType::MEMBERSHIP_CHANGE;
            default:
                throw Common::EventTypes::IEventException("Common::EventTypes::EventType, contained unknown type");
        }
    }

    Common::EventTypes::SessionType convertFromCapnSessionType(Sophos::Journal::CredentialsEvent::SessionType sessionType)
    {
        switch(sessionType)
        {
            case Sophos::Journal::CredentialsEvent::SessionType::INTERACTIVE:
                return Common::EventTypes::SessionType::interactive;
            case Sophos::Journal::CredentialsEvent::SessionType::NETWORK:
                return Common::EventTypes::SessionType::network;
            case Sophos::Journal::CredentialsEvent::SessionType::NETWORK_INTERACTIVE:
                return Common::EventTypes::SessionType::networkInteractive;
            default:
                throw Common::EventTypes::IEventException("Sophos::Journal::CredentialsEvent::SessionType, contained unknown type");

        }
    }

    Common::EventTypes::EventType convertFromCapnEventType(Sophos::Journal::CredentialsEvent::EventType eventType)
    {
        switch(eventType)
        {
            case Sophos::Journal::CredentialsEvent::EventType::AUTH_FAILURE:
                return Common::EventTypes::EventType::authFailure;
            case Sophos::Journal::CredentialsEvent::EventType::AUTH_SUCCESS:
                return Common::EventTypes::EventType::authSuccess;
            case Sophos::Journal::CredentialsEvent::EventType::CREATED:
                return Common::EventTypes::EventType::created;
            case Sophos::Journal::CredentialsEvent::EventType::DELETED:
                return Common::EventTypes::EventType::deleted;
            case Sophos::Journal::CredentialsEvent::EventType::PASSWORD_CHANGE:
                return Common::EventTypes::EventType::passwordChange;
            case Sophos::Journal::CredentialsEvent::EventType::MEMBERSHIP_CHANGE:
                return Common::EventTypes::EventType::membershipChange;
            default:
                throw Common::EventTypes::IEventException("Sophos::Journal::CredentialsEvent::EventType, contained unknown type");
        }
    }
}


namespace Common
{
    namespace EventTypesImpl
    {
        const std::string CredentialEvent::getEventTypeId() const
        {
            return "Detector.Credentials";
        }

        const Common::EventTypes::SessionType CredentialEvent::getSessionType() const
        {
            return m_sessionType;
        }

        const Common::EventTypes::EventType CredentialEvent::getEventType() const
        {
            return m_eventType;
        }

        const Common::EventTypes::UserSid CredentialEvent::getSubjectUserSid() const
        {
            return m_subjectUserSid;
        }

        const Common::EventTypes::UserSid CredentialEvent::getTargetUserSid() const
        {
            return m_targetUserSid;
        }

        const unsigned long long CredentialEvent::getTimestamp() const
        {
            return m_timestamp;
        }

        const unsigned long CredentialEvent::getLogonId() const
        {
            return m_logonId;
        }

        const Common::EventTypes::NetworkAddress CredentialEvent::getRemoteNetworkAccess() const
        {
            return m_remoteNetworkAccess;
        }

        const unsigned long CredentialEvent::getGroupId() const
        {
            return m_groupId;
        }

        const std::string CredentialEvent::getGroupName() const
        {
            return m_groupName;
        }

        void CredentialEvent::setSessionType(const Common::EventTypes::SessionType sessionType)
        {
            m_sessionType = sessionType;
        }

        void CredentialEvent::setEventType(const Common::EventTypes::EventType eventType)
        {
            m_eventType = eventType;
        }
        void CredentialEvent::setSubjectUserSid(const Common::EventTypes::UserSid subjectUserSid)
        {
            m_subjectUserSid = subjectUserSid;
        }

        void CredentialEvent::setTargetUserSid(const Common::EventTypes::UserSid targetUserSid)
        {
            m_targetUserSid = targetUserSid;
        }

        void CredentialEvent::setTimestamp(const unsigned long long timestamp)
        {
            m_timestamp = timestamp;
        }

        void CredentialEvent::setLogonId(const unsigned long logonId)
        {
            m_logonId = logonId;
        }

        void CredentialEvent::setRemoteNetworkAccess(const Common::EventTypes::NetworkAddress remoteNetworkAccess)
        {
            m_remoteNetworkAccess = remoteNetworkAccess;
        }

        void CredentialEvent::setGroupId(const unsigned long groupId)
        {
            m_groupId = groupId;
        }

        void CredentialEvent::setGroupName(const std::string& groupName)
        {
            m_groupName = groupName;
        }


        const std::string CredentialEvent::toString() const
        {

            ::capnp::MallocMessageBuilder message;
            Sophos::Journal::CredentialsEvent::Builder credentialsEvent = message.initRoot<Sophos::Journal::CredentialsEvent>();

            credentialsEvent.setSessionType(convertToCapnSessionType(m_sessionType));
            credentialsEvent.setEventType(convertToCapnEventType(m_eventType));

            credentialsEvent.getSubjectUserSID().setUsername(m_subjectUserSid.username);
            credentialsEvent.getSubjectUserSID().setDomain(m_subjectUserSid.domain);

            credentialsEvent.getTargetUserSID().setUsername(m_targetUserSid.username);
            credentialsEvent.getTargetUserSID().setDomain(m_targetUserSid.domain);

            credentialsEvent.getRemoteNetworkAddress().setAddress(m_remoteNetworkAccess.address);

            credentialsEvent.setTimestamp(m_timestamp);
            credentialsEvent.setLogonID(m_logonId);

            credentialsEvent.setUserGroupID(m_groupId);
            credentialsEvent.setUserGroupName(m_groupName);

            // Convert to byte string
            kj::Array<capnp::word> dataArray = capnp::messageToFlatArray(message);
            kj::ArrayPtr<kj::byte> bytes = dataArray.asBytes();
            std::string dataAsString(bytes.begin(), bytes.end());

            return dataAsString;
        }

        CredentialEvent CredentialEvent::fromString(const std::string& objectAsString)
        {

            if(objectAsString.empty())
            {
                throw Common::EventTypes::IEventException("Invalid capn byte string, string is empty");
            }

            try
            {
                const kj::ArrayPtr<const capnp::word> view(
                        reinterpret_cast<const capnp::word*>(&(*std::begin(objectAsString))),
                        reinterpret_cast<const capnp::word*>(&(*std::end(objectAsString))));


                capnp::FlatArrayMessageReader message(view);
                Sophos::Journal::CredentialsEvent::Reader credentialsEvent = message.getRoot<Sophos::Journal::CredentialsEvent>();

                Common::EventTypesImpl::CredentialEvent event;

                event.setSessionType(convertFromCapnSessionType(credentialsEvent.getSessionType()));
                event.setEventType(convertFromCapnEventType(credentialsEvent.getEventType()));

                std::string groupName(credentialsEvent.getUserGroupName());
                event.setGroupName(groupName);

                event.setGroupId(credentialsEvent.getUserGroupID());
                event.setTimestamp(credentialsEvent.getTimestamp());
                event.setLogonId(credentialsEvent.getLogonID());

                Common::EventTypes::UserSid subjectUserId;
                subjectUserId.username = credentialsEvent.getSubjectUserSID().getUsername();;
                subjectUserId.domain = credentialsEvent.getSubjectUserSID().getDomain();
                event.setSubjectUserSid(subjectUserId);

                Common::EventTypes::UserSid targetUserId;
                targetUserId.username = credentialsEvent.getTargetUserSID().getUsername();
                targetUserId.domain = credentialsEvent.getTargetUserSID().getDomain();
                event.setTargetUserSid(targetUserId);

                Common::EventTypes::NetworkAddress networkAddress;
                networkAddress.address = credentialsEvent.getRemoteNetworkAddress().getAddress();
                event.setRemoteNetworkAccess(networkAddress);
                return event;
            }
            catch(std::exception& ex)
            {
                std::stringstream errorMessage;
                errorMessage << "Error: failed to process capn CredentialEvent string, " << ex.what();
                throw Common::EventTypes::IEventException(errorMessage.str());
            }
        }
    }
}