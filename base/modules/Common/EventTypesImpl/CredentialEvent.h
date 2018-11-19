/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/EventTypes/ICredentialEvent.h>

namespace Common
{
    namespace EventTypes
    {
        class CredentialEvent : public ICredentialEvent
        {
        public:
            CredentialEvent(std::string& sessionType, std::string& subjectUserSID, std::string& targetUserSID, std::string& timestamp, std::string& logonID, std::string& remoteNetworkAccess);
            ~CredentialEvent() = default;

            const std::string getSessionType() const override;
            const std::string getSubjectUserSID() const override;
            const std::string getTargetUserSID() const override;
            const std::string getTimestamp() const override;
            const std::string getLogonID() const override;
            const std::string getRemoteNetworkAccess() const override;
            void setSessionType(std::string & sessionType) override;
            void setSubjectUserSID(std::string& subjectUserSID) override;
            void setTargetUserSID(std::string& targetUserSID) override;
            void setTimestamp(std::string& timestamp) override;
            void setLogonID(std::string& logonID) override;
            void setRemoteNetworkAccess(std::string& remoteNetworkAccess) override;
            std::string toString() override;
            void fromString(std::string& objectAsString, ICredentialEvent* credentialEvent) override;

        private:
            std::string m_sessionType;
            std::string m_subjectUserID;
            std::string m_targetUserSID;
            std::string m_timestamp;
            std::string m_logonID;
            std::string m_remoteNetworkAccess;

        };
    }

}
