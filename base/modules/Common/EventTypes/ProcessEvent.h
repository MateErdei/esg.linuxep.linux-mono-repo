/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <capnp/blob.h>
#include "CommonEventData.h"
#include "IEventType.h"
#include "PortScanningEvent.h"

namespace Common
{
    namespace EventTypes
    {
    class ProcessEvent : public Common::EventTypes::IEventType
        {
        public:

        enum EventType
        {
            start = 0,
            end = 1,
        };
            ProcessEvent() = default;

            ~ProcessEvent() = default;

            const EventType getEventType() const ;
            const std::string getEventTypeId() const override;
            const unsigned long long getEndTime() const ;
            const Common::EventTypes::OptionalUInt64 getFileSize() const ;
            const std::uint32_t getFlags() const ;
            const std::uint32_t getSessionId() const ;
//            const std::string getSId() const ; //We think this is windows only
            const Common::EventTypes::PathName getPathName() const ;
            const std::string getcmdLine() const ;
//            const Common::EventType::Data getSha256() const ;
//            const Common::EventType::Data getSha1() const ;
//            const Common::EventType::Data getPeSha256() const ;
//            const Common::EventType::Data getPeSha1() const ;

            void setEventType(const EventType eventType) ;
            void setEndTime(const unsigned long long endTime) ;
            void setFileSize(const Common::EventTypes::OptionalUInt64 fileSize) ;
            void setFlags(const std::uint32_t flags) ;
            void setSessionId(const std::uint32_t sessionId) ;
//            void setSid(const Common::EventTypes::Data sid) ;
            void setPathName(const Common::EventTypes::PathName pathName) ;
            void setCmdLine(const std::string cmdLine) ;
//            void setSha256(const Common::EventTypes::Data sha256) ;
//            void setSha1(const Common::EventTypes::Data sha1) ;
//            void setPeSha256(const Common::EventTypes::Data pesha256) ;
//            void setPeSha1(const Common::EventTypes::Data pesha1) ;


            std::string toString() const override;

            /**
            * Takes in a event object as a capn byte string.
            * @param a objectAsString
            * @returns CredentialEvent object created from the capn byte string..
            */
            void fromString(const std::string& objectAsString);

        private:
            Common::EventTypes::ProcessEvent::EventType m_eventType;
            unsigned long long m_endTime;
            Common::EventTypes::OptionalUInt64 m_fileSize;
            std::uint32_t m_flags;
            std::uint32_t m_sessionId;
            std::string m_sid;
            Common::EventTypes::PathName m_pathName;
            std::string m_cmdLine;
//            Common::EventTypes::Data m_sha256;
//            Common::EventTypes::Data m_sha1;
//            Common::EventTypes::Data m_pesha256;
//            Common::EventTypes::Data m_pesha1;
        };

        Common::EventTypes::ProcessEvent createProcessEvent(Common::EventTypes::ProcessEvent::EventType eventType);
    }

}
