/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "CredentialEvent.h"

namespace Common
{
    namespace EventTypes
    {
        CredentialEvent::CredentialEvent(std::string& sessionType, std::string& subjectUserSID, std::string& targetUserSID, std::string& timestamp, std::string& logonID, std::string& remoteNetworkAccess)
                : m_sessionType(sessionType)
                  , m_subjectUserID(subjectUserSID)
                  , m_targetUserSID(targetUserSID)
                  , m_timestamp(timestamp)
                  , m_logonID(logonID)
                  , m_remoteNetworkAccess(remoteNetworkAccess)
        {
        }

        const std::string CredentialEvent::getSessionType() const
        {
            return m_sessionType;
        }

        const std::string CredentialEvent::getSubjectUserSID() const
        {
            return m_subjectUserID;
        }

        const std::string CredentialEvent::getTargetUserSID() const
        {
            return m_targetUserSID;
        }

        const std::string CredentialEvent::getTimestamp() const
        {
            return m_timestamp;
        }

        const std::string CredentialEvent::getLogonID() const
        {
            return m_logonID;
        }

        const std::string CredentialEvent::getRemoteNetworkAccess() const
        {
            return m_remoteNetworkAccess;
        }

        void CredentialEvent::setSessionType(std::string& sessionType)
        {
            m_sessionType = sessionType;
        }

        void CredentialEvent::setSubjectUserSID(std::string& subjectUserSID)
        {
            m_subjectUserID = subjectUserSID;
        }

        void CredentialEvent::setTargetUserSID(std::string& targetUserSID)
        {
            m_targetUserSID = targetUserSID;
        }

        void CredentialEvent::setTimestamp(std::string& timestamp)
        {
            m_timestamp = timestamp;
        }

        void CredentialEvent::setLogonID(std::string& logonID)
        {
            m_logonID = logonID;
        }

        void CredentialEvent::setRemoteNetworkAccess(std::string& remoteNetworkAccess)
        {
            m_remoteNetworkAccess = remoteNetworkAccess;
        }

        std::string CredentialEvent::toString()
        {
            return "";
        }

        void CredentialEvent::fromString(std::string& objectAsString, ICredentialEvent* credentialEvent)
        {
        }
    }
}