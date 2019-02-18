/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ProcessEvent.h"

#include "EventStrings.h"

#include <Common/EventTypes/CommonEventData.h>
#include <Common/EventTypes/IEventException.h>
#include <capnp/message.h>
#include <capnp/serialize-packed.h>

#include <Process.capnp.h>
#include <iostream>
#include <sstream>

namespace
{
    Sophos::Journal::ProcessEvent::EventType convertToCapnEventType(
        Common::EventTypes::ProcessEvent::EventType eventType)
    {
        switch (eventType)
        {
            case Common::EventTypes::ProcessEvent::EventType::start:
                return Sophos::Journal::ProcessEvent::EventType::START;
            case Common::EventTypes::ProcessEvent::EventType::end:
                return Sophos::Journal::ProcessEvent::EventType::END;
            default:
                throw Common::EventTypes::IEventException("Common::EventTypes::EventType, contained unknown type");
        }
    }

    Common::EventTypes::ProcessEvent::EventType convertFromCapnEventType(
        Sophos::Journal::ProcessEvent::EventType eventType)
    {
        switch (eventType)
        {
            case Sophos::Journal::ProcessEvent::EventType::START:
                return Common::EventTypes::ProcessEvent::EventType::start;
            case Sophos::Journal::ProcessEvent::EventType::END:
                return Common::EventTypes::ProcessEvent::EventType::end;
            default:
                throw Common::EventTypes::IEventException(
                    "Sophos::Journal::ProcessEvent::EventType, contained unknown type");
        }
    }
} // namespace

Common::EventTypes::ProcessEvent Common::EventTypes::createProcessEvent(
    Common::EventTypes::ProcessEvent::EventType eventType)
{
    Common::EventTypes::ProcessEvent event = ProcessEvent();
    event.setEventType(eventType);

    return event;
}

namespace Common
{
    namespace EventTypes
    {
        ProcessEvent::ProcessEvent() : m_eventType(start), m_endTime(0), m_flags(0), m_sessionId(0)
        {
            m_sophosPid.pid = 0;
            m_sophosPid.timestamp = 0;
            m_parentSophosPid.pid = 0;
            m_parentSophosPid.timestamp = 0;
            m_parentSophosTid.tid = 0;
            m_parentSophosTid.timestamp = 0;
            m_fileSize.value = 0;
        }

        std::string ProcessEvent::toString() const
        {
            ::capnp::MallocMessageBuilder message;
            Sophos::Journal::ProcessEvent::Builder processEvent = message.initRoot<Sophos::Journal::ProcessEvent>();

            processEvent.setEventType(convertToCapnEventType(m_eventType));

            processEvent.getSophosPID().setOsPID(m_sophosPid.pid);
            processEvent.getSophosPID().setCreateTime(m_sophosPid.timestamp);

            processEvent.getParentSophosPID().setOsPID(m_parentSophosPid.pid);
            processEvent.getParentSophosPID().setCreateTime(m_parentSophosPid.timestamp);

            processEvent.getParentSophosTID().setOsTID(m_parentSophosTid.tid);
            processEvent.getParentSophosTID().setCreateTime(m_parentSophosTid.timestamp);

            processEvent.setEndTime(m_endTime);
            processEvent.setFlags(m_flags);
            processEvent.setSessionId(m_sessionId);

            ::capnp::Data::Reader sidReader =
                ::capnp::Data::Reader(reinterpret_cast<const ::capnp::byte*>(m_sid.data()), m_sid.size());
            processEvent.setSid(sidReader);

            processEvent.getOwnerUserSID().setUsername(m_ownerUserSid.username);
            processEvent.getOwnerUserSID().setSid(sidReader);
            processEvent.getOwnerUserSID().setDomain(m_ownerUserSid.domain);

            processEvent.getFileSize().setValue(m_fileSize.value);
            processEvent.getPathname().setFlags(m_pathname.flags);
            processEvent.getPathname().setFileSystemType(m_pathname.fileSystemType);
            processEvent.getPathname().setDriveLetter(m_pathname.driveLetter);
            processEvent.getPathname().setPathname(m_pathname.pathname);
            processEvent.getPathname().getOpenName().setLength(m_pathname.openName.length);
            processEvent.getPathname().getOpenName().setOffset(m_pathname.openName.offset);
            processEvent.getPathname().getVolumeName().setLength(m_pathname.volumeName.length);
            processEvent.getPathname().getVolumeName().setOffset(m_pathname.volumeName.offset);
            processEvent.getPathname().getShareName().setLength(m_pathname.shareName.length);
            processEvent.getPathname().getShareName().setOffset(m_pathname.shareName.offset);
            processEvent.getPathname().getExtensionName().setLength(m_pathname.extensionName.length);
            processEvent.getPathname().getExtensionName().setOffset(m_pathname.extensionName.offset);
            processEvent.getPathname().getStreamName().setLength(m_pathname.streamName.length);
            processEvent.getPathname().getStreamName().setOffset(m_pathname.streamName.offset);
            processEvent.getPathname().getFinalComponentName().setLength(m_pathname.finalComponentName.length);
            processEvent.getPathname().getFinalComponentName().setOffset(m_pathname.finalComponentName.offset);
            processEvent.getPathname().getParentDirName().setLength(m_pathname.parentDirName.length);
            processEvent.getPathname().getParentDirName().setOffset(m_pathname.parentDirName.offset);

            processEvent.setCmdLine(m_cmdLine);

            ::capnp::Data::Reader sha256Reader =
                ::capnp::Data::Reader(reinterpret_cast<const ::capnp::byte*>(m_sha256.data()), m_sha256.size());
            processEvent.setSha256(sha256Reader);

            ::capnp::Data::Reader sha1Reader =
                ::capnp::Data::Reader(reinterpret_cast<const ::capnp::byte*>(m_sha1.data()), m_sha1.size());
            processEvent.setSha1(sha1Reader);

            processEvent.setProcTitle(m_procTitle);

            // Convert to byte string
            kj::Array<capnp::word> dataArray = capnp::messageToFlatArray(message);
            kj::ArrayPtr<kj::byte> bytes = dataArray.asBytes();
            std::string dataAsString(bytes.begin(), bytes.end());

            return dataAsString;
        }

        void ProcessEvent::fromString(const std::string& objectAsString)
        {
            if (objectAsString.empty())
            {
                throw Common::EventTypes::IEventException("Invalid capn byte string, string is empty");
            }

            try
            {
                const kj::ArrayPtr<const capnp::word> view(
                    reinterpret_cast<const capnp::word*>(&(*std::begin(objectAsString))),
                    reinterpret_cast<const capnp::word*>(&(*std::end(objectAsString))));

                capnp::FlatArrayMessageReader message(view);
                Sophos::Journal::ProcessEvent::Reader processEvent = message.getRoot<Sophos::Journal::ProcessEvent>();

                setEventType(convertFromCapnEventType(processEvent.getEventType()));

                Common::EventTypes::SophosPid sophosPid{ 0, 0 };
                sophosPid.pid = processEvent.getSophosPID().getOsPID();
                sophosPid.timestamp = processEvent.getSophosPID().getCreateTime();
                setSophosPid(sophosPid);

                Common::EventTypes::SophosPid parentSophosPid{ 0, 0 };
                parentSophosPid.pid = processEvent.getParentSophosPID().getOsPID();
                parentSophosPid.timestamp = processEvent.getParentSophosPID().getCreateTime();
                setParentSophosPid(parentSophosPid);

                Common::EventTypes::SophosTid parentSophosTid{ 0, 0 };
                parentSophosTid.tid = processEvent.getParentSophosTID().getOsTID();
                parentSophosTid.timestamp = processEvent.getParentSophosTID().getCreateTime();
                setParentSophosTid(parentSophosTid);

                setEndTime(processEvent.getEndTime());

                Common::EventTypes::OptionalUInt64 fileSize{ processEvent.getFileSize().getValue() };
                setFileSize(fileSize);

                setFlags(processEvent.getFlags());
                setSessionId(processEvent.getSessionId());

                ::capnp::Data::Reader sidReader = processEvent.getSid();
                std::string sidString(sidReader.begin(), sidReader.end());
                setSid(sidString);

                Common::EventTypes::UserSid processOwner;
                processOwner.username = processEvent.getOwnerUserSID().getUsername();
                processOwner.sid = sidString;
                processOwner.domain = processEvent.getOwnerUserSID().getDomain();
                setOwnerUserSid(processOwner);

                Common::EventTypes::Pathname pathname;
                pathname.flags = processEvent.getPathname().getFlags();
                pathname.fileSystemType = processEvent.getPathname().getFileSystemType();
                pathname.driveLetter = processEvent.getPathname().getDriveLetter();
                pathname.pathname = processEvent.getPathname().getPathname();
                pathname.openName.length = processEvent.getPathname().getOpenName().getLength();
                pathname.openName.offset = processEvent.getPathname().getOpenName().getOffset();
                pathname.volumeName.length = processEvent.getPathname().getVolumeName().getLength();
                pathname.volumeName.offset = processEvent.getPathname().getVolumeName().getOffset();
                pathname.shareName.length = processEvent.getPathname().getShareName().getLength();
                pathname.shareName.offset = processEvent.getPathname().getShareName().getOffset();
                pathname.extensionName.length = processEvent.getPathname().getExtensionName().getLength();
                pathname.extensionName.offset = processEvent.getPathname().getExtensionName().getOffset();
                pathname.streamName.length = processEvent.getPathname().getStreamName().getLength();
                pathname.streamName.offset = processEvent.getPathname().getStreamName().getOffset();
                pathname.finalComponentName.length = processEvent.getPathname().getFinalComponentName().getLength();
                pathname.finalComponentName.offset = processEvent.getPathname().getFinalComponentName().getOffset();
                pathname.parentDirName.length = processEvent.getPathname().getParentDirName().getLength();
                pathname.parentDirName.offset = processEvent.getPathname().getParentDirName().getOffset();
                setPathname(pathname);

                setCmdLine(processEvent.getCmdLine());

                ::capnp::Data::Reader sha256Reader = processEvent.getSha256();
                std::string sha256String(sha256Reader.begin(), sha256Reader.end());
                setSha256(sha256String);

                ::capnp::Data::Reader sha1Reader = processEvent.getSha1();
                std::string sha1String(sha1Reader.begin(), sha1Reader.end());
                setSha1(sha1String);

                setProcTitle(processEvent.getProcTitle());
            }
            catch (std::exception& ex)
            {
                std::stringstream errorMessage;
                errorMessage << "Error: failed to process capn ProcessEvent string, " << ex.what();
                throw Common::EventTypes::IEventException(errorMessage.str());
            }
        }

        // Getters
        Common::EventTypes::ProcessEvent::EventType EventTypes::ProcessEvent::getEventType() const
        {
            return m_eventType;
        }

        Common::EventTypes::SophosPid ProcessEvent::getSophosPid() const { return m_sophosPid; }

        Common::EventTypes::SophosPid ProcessEvent::getParentSophosPid() const { return m_parentSophosPid; }

        Common::EventTypes::SophosTid ProcessEvent::getParentSophosTid() const { return m_parentSophosTid; }

        std::string ProcessEvent::getEventTypeId() const { return Common::EventTypes::ProcessEventName; }

        windows_timestamp_t ProcessEvent::getEndTime() const { return m_endTime; }

        Common::EventTypes::OptionalUInt64 ProcessEvent::getFileSize() const { return m_fileSize; }

        std::uint32_t ProcessEvent::getFlags() const { return m_flags; }

        std::uint32_t ProcessEvent::getSessionId() const { return m_sessionId; }

        std::string ProcessEvent::getSid() const { return m_sid; }

        Common::EventTypes::UserSid ProcessEvent::getOwnerUserSid() const { return m_ownerUserSid; }

        Common::EventTypes::Pathname ProcessEvent::getPathname() const { return m_pathname; }

        std::string ProcessEvent::getCmdLine() const { return m_cmdLine; }

        std::string ProcessEvent::getSha256() const { return m_sha256; }

        std::string ProcessEvent::getSha1() const { return m_sha1; }

        std::string ProcessEvent::getProcTitle() const { return m_procTitle; }

        // Setters
        void ProcessEvent::setEventType(const ProcessEvent::EventType eventType) { m_eventType = eventType; }

        void ProcessEvent::setSophosPid(const Common::EventTypes::SophosPid& sophosPid) { m_sophosPid = sophosPid; }

        void ProcessEvent::setParentSophosPid(const Common::EventTypes::SophosPid& parentSophosPid)
        {
            m_parentSophosPid = parentSophosPid;
        }

        void ProcessEvent::setParentSophosTid(const Common::EventTypes::SophosTid& parentSophosTid)
        {
            m_parentSophosTid = parentSophosTid;
        }

        void ProcessEvent::setEndTime(windows_timestamp_t endTime) { m_endTime = endTime; }

        void ProcessEvent::setFileSize(Common::EventTypes::OptionalUInt64 fileSize) { m_fileSize = fileSize; }

        void ProcessEvent::setFlags(std::uint32_t flags) { m_flags = flags; }

        void ProcessEvent::setSessionId(std::uint32_t sessionId) { m_sessionId = sessionId; }

        void ProcessEvent::setSid(const std::string& sid) { m_sid = sid; }

        void ProcessEvent::setOwnerUserSid(const Common::EventTypes::UserSid& ownerUserSid)
        {
            m_ownerUserSid = ownerUserSid;
        }

        void ProcessEvent::setPathname(const Common::EventTypes::Pathname& pathname) { m_pathname = pathname; }

        void ProcessEvent::setPathname(const std::string& pathnameString)
        {
            Common::EventTypes::Pathname pathname;
            pathname.pathname = pathnameString;

            // Find the last slash to identify the executable name and set the finalComponentName and parentDirName
            size_t lastSlashPosition = pathname.pathname.rfind('/');

            // If the path string contains a '/'
            if (lastSlashPosition != std::string::npos)
            {
                if (pathname.pathname.back() != '/')
                {
                    pathname.finalComponentName.offset = static_cast<uint32_t>(lastSlashPosition) + 1;
                    pathname.finalComponentName.length =
                        static_cast<uint32_t>(pathname.pathname.size() - pathname.finalComponentName.offset);
                }

                pathname.parentDirName.offset = 0;
                pathname.parentDirName.length = static_cast<uint32_t>(lastSlashPosition + 1);
            }
            pathname.openName = { static_cast<uint32_t>(pathname.pathname.size()), 0 };

            // Find the final period in the finalComponent name and set the extension if there is a final component name
            if (pathname.finalComponentName.length != 0)
            {
                std::string finalComponentName =
                    pathname.pathname.substr(pathname.finalComponentName.offset, pathname.finalComponentName.length);
                size_t periodPosition = finalComponentName.rfind('.');
                if (periodPosition != std::string::npos && periodPosition < pathname.pathname.size() - 1)
                {
                    pathname.extensionName.offset =
                        static_cast<uint32_t>(pathname.finalComponentName.offset + periodPosition + 1);
                    pathname.extensionName.length =
                        static_cast<uint32_t>(pathname.pathname.size() - pathname.extensionName.offset);
                }
            }
            m_pathname = pathname;
        }

        void ProcessEvent::setCmdLine(const std::string& cmdLine) { m_cmdLine = cmdLine; }

        void ProcessEvent::setSha256(const std::string& sha256) { m_sha256 = sha256; }

        void ProcessEvent::setSha1(const std::string& sha1) { m_sha1 = sha1; }

        void ProcessEvent::setProcTitle(const std::string& procTitle) { m_procTitle = procTitle; }
    } // namespace EventTypes
} // namespace Common