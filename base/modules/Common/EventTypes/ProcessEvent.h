/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "CommonEventData.h"
#include "IEventType.h"
#include "SophosString.h"

namespace Common
{
    namespace EventTypes
    {
        class ProcessEvent : public virtual Common::EventTypes::IEventType
        {
        public:
            using windows_timestamp_t = Common::EventTypes::windows_timestamp_t;

            enum EventType
            {
                start = 0,
                end = 1,
            };
            ProcessEvent();

            /**
             * return the event type of this process event, i.e. if it's a start or end event.
             * @return
             */
            EventType getEventType() const;

            /**
             * Gets the SophosPid of this process which is the PID concatenated with timestamp of when the process
             * started.
             * @return
             */
            Common::EventTypes::SophosPid getSophosPid() const;

            /**
             * This is the parent process SophosPID as explained above
             * @return
             */
            Common::EventTypes::SophosPid getParentSophosPid() const;

            /**
             * Gets the sophosPid which is the TID (thread ID) concatenated with timestamp of when the process started.
             * @return
             */
            Common::EventTypes::SophosTid getParentSophosTid() const;

            /**
             * return the ID of the event type.
             * @return
             */
            std::string getEventTypeId() const override;

            /**
             * Return the time the process ended, this may not be present if it's a start event.
             * @return
             */
            windows_timestamp_t getEndTime() const;

            /**
             * Return an optional filesize of the binary
             * @return
             */
            Common::EventTypes::OptionalUInt64 getFileSize() const;

            /**
             * Process flags, we don't know what these will be on Linux, will likely just be 0.
             * @return
             */
            std::uint32_t getFlags() const;

            /**
             * On linux this is the process namespace.
             * @return
             */
            std::uint32_t getSessionId() const;

            /**
             * On linux this is the UID, on windows SID (Security Identifier) has a specific meaning.
             * @return
             */
            std::string getSid() const;

            /**
             * Return a struct that holds information about the user that owns a process
             * @return
             */
            Common::EventTypes::UserSid getOwnerUserSid() const;

            /**
             * Return a struct that holds information about the path of the binary file.
             * @return
             */
            Common::EventTypes::Pathname getPathname() const;

            /**
             * Command line of running process - e.g. what you see when you run a ps on Linux.
             * @return
             */
            std::string getCmdLine() const;

            /**
             * Return the SHA256 of the executable.
             * @return
             */
            std::string getSha256() const;

            /**
             * Return the SHA1 of the executable.
             * @return
             */
            std::string getSha1() const;

            /*
             * Return the procTitle of the this process event.
             * This is the cmd line string of the originating top level process.
             * When processes are nested this will be the same for all child processes that are spawned from the
             * top level process, which will have a matching cmdline as this value.
             */
            std::string getProcTitle() const;

            /**
             * Sets the type of process event, either it's a process starting or ending.
             * @param eventType
             */
            void setEventType(EventType eventType);

            /**
             * Sets sophosPid which is the Sophos Process ID of this process.
             * A Sophos Process ID is the concatenation of the PID and timestamp of the start of the process.
             * @param sophosPid
             */
            void setSophosPid(const Common::EventTypes::SophosPid& sophosPid);

            /**
             * Sets parentSophosPid which is the Sophos Process ID of this process' parent.
             * @param parentSophosPid
             */
            void setParentSophosPid(const Common::EventTypes::SophosPid& parentSophosPid);

            /**
             * Sets parentSophosTid which is the Sophos Thread ID of this process' parent.
             * @param parentSophosTid
             */
            void setParentSophosTid(const Common::EventTypes::SophosTid& parentSophosTid);

            /**
             * Sets the process end time property of this ProcessEvent object
             * @param endTime
             */
            void setEndTime(windows_timestamp_t endTime);

            /**
             * Sets the filesize, in bytes, property of this ProcessEvent object
             * @param fileSize
             */
            void setFileSize(Common::EventTypes::OptionalUInt64 fileSize);

            /**
             * Sets the flags property of this ProcessEvent object
             * @param flags
             */
            void setFlags(std::uint32_t flags);

            /**
             * Sets the session ID property of this ProcessEvent object
             * @param sessionId
             */
            void setSessionId(std::uint32_t sessionId);

            /**
             * Sets the sid property of this ProcessEvent object
             * @param sid
             */
            void setSid(const SophosString& sid);

            /**
             * Sets the ownerUserSid property of this ProcessEvent object
             * @param ownerUserSid
             */
            void setOwnerUserSid(const Common::EventTypes::UserSid& ownerUserSid);

            /**
             * Sets the pathname property of this ProcessEvent object
             * @param pathname
             */
            void setPathname(const Common::EventTypes::Pathname& pathname);

            /**
             * Constructs the pathname property of this ProcessEvent object
             * from the given string
             * @param pathnameString
             */
            void setPathname(const SophosString& pathnameString);

            /**
             * Sets the command line property of this ProcessEvent object
             * @param cmdLine
             */
            void setCmdLine(const SophosString& cmdLine);

            /**
             * Sets the SHA256 property of this ProcessEvent object
             * @param sha256
             */
            void setSha256(const SophosString& sha256);

            /**
             * Sets the SHA1 property of this ProcessEvent object
             * @param sha1
             */
            void setSha1(const SophosString& sha1);

            /**
             * Sets the procTitle property of this ProcessEvent object.
             * This is the cmd line string of the originating top level process.
             * When processes are nested this will be the same for all child processes that are spawned from the
             * top level process, which will have a matching cmdline as this value.
             * @param sha1
             */
            void setProcTitle(const SophosString& procTitle);

            /**
             * Turns this ProcessEvent object into a capn byte string.
             * @return a capn byte string of the ProcessEvent.
             */
            std::string toString() const override;

            /**
             * Takes in a event object as a capn byte string.
             * @param an objectAsString
             * @returns ProcessEvent object created from the capn byte string.
             */
            void fromString(const std::string& objectAsString);

        private:
            Common::EventTypes::ProcessEvent::EventType m_eventType;

            // A SophosPid is the PID concatenated with timestamp of when the process started.
            Common::EventTypes::SophosPid m_sophosPid;

            // A SophosPid is the PID concatenated with timestamp of when the process started.
            // This is the parent process SophosPID
            Common::EventTypes::SophosPid m_parentSophosPid;

            // A sophosTid is the TID (thread ID) concatenated with timestamp of when the process started.
            Common::EventTypes::SophosTid m_parentSophosTid;
            windows_timestamp_t m_endTime;
            Common::EventTypes::OptionalUInt64 m_fileSize;

            // Process flags, we don't know what these will be on Linux, will likely just be 0.
            std::uint32_t m_flags;

            // On linux this is the process namespace.
            std::uint32_t m_sessionId;

            // a struct that holds information about the path of the binary file
            Common::EventTypes::Pathname m_pathname;

            // On linux this is the UID, on windows SID (Security Identifier) has a specific meaning.
            std::string m_sid;

            // A Struct that holds information about the user that owns a process
            Common::EventTypes::UserSid m_ownerUserSid;

            // Command line of running process - e.g. what you get when you run a ps on Linux.
            std::string m_cmdLine;
            std::string m_sha256;
            std::string m_sha1;

            // m_procTitle is the command of the originating process,
            // i.e.this will be the same as cmdLine for the top level process in a series of nested process calls.
            std::string m_procTitle;
        };

        Common::EventTypes::ProcessEvent createProcessEvent(Common::EventTypes::ProcessEvent::EventType eventType);
    } // namespace EventTypes

} // namespace Common
