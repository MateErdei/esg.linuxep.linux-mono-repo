// Copyright 2021-2023 Sophos Limited. All rights reserved.

#include "EventJournalReaderWrapper.h"
#include "EventJournalTimeUtils.h"
#include "Logger.h"

#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/UtilityImpl/FileUtils.h"
#include "Common/UtilityImpl/TimeUtils.h"

#include <capnp/message.h>
#include <capnp/serialize.h>


#ifdef SPL_BAZEL
#include "journallib/Journal.h"
#include "cpp_lib/Event.capnp.h"
#else
#include "Event.capnp.h"
#include "Journal.h"
#endif


#include <cstring>
#include <fstream>

namespace Common
{
    namespace EventJournalWrapper
    {
        static std::string getSubjectName(Subject subject)
        {
            switch (subject)
            {
                case Subject::Detections: return "Detections";
                default: return "";
            }
        }

        Reader::Reader()
        {
            m_helper = Sophos::Journal::GetHelper();
        }

        Reader::Reader(std::shared_ptr<Sophos::Journal::HelperInterface> myHelper)
        {
            m_helper = myHelper;
        }

        Reader::Reader(const std::string& location) : m_location(location)
        {
            std::optional<Sophos::Journal::ViewSettings> settings = Sophos::Journal::ViewSettings();
            settings.value()[Sophos::Journal::Settings::FlushDelayKey] = "false";
            settings.value()[Sophos::Journal::Settings::FlushDelayIntervalKey] = "0";
            settings.value()[Sophos::Journal::Settings::JournalLocationKey] = m_location;
            m_helper = Sophos::Journal::GetHelper(settings);
        }

        std::string Reader::getCurrentJRLForId(const std::string& idFilePath)
        {
            std::string jrl("");
            auto filesystem = Common::FileSystem::fileSystem();
            try
            {
                if (filesystem->isFile(idFilePath))
                {
                    jrl = filesystem->readFile(idFilePath);
                }
            }
            catch (const Common::FileSystem::IFileSystemException& ex)
            {
                LOGWARN("Failed to read jrl file " << idFilePath << " with error: " << ex.what());
            }
            return jrl;
        }

        void Reader::updateJrl(const std::string& idFilePath, const std::string& jrl)
        {
            if (idFilePath.empty())
            {
                return;
            }
            auto filesystem = Common::FileSystem::fileSystem();
            try
            {
                if (filesystem->isFile(idFilePath))
                {
                    filesystem->removeFile(idFilePath);
                }

                filesystem->writeFile(idFilePath, jrl);
            }
            catch (const Common::FileSystem::IFileSystemException& ex)
            {
                LOGWARN("Failed to update jrl file "<< idFilePath << " with error: " << ex.what());
            }
        }

        u_int32_t Reader::getCurrentJRLAttemptsForId(const std::string& trackerFilePath)
        {
            u_int32_t attempts = 0;
            auto filesystem = Common::FileSystem::fileSystem();
            try
            {
                if (filesystem->isFile(trackerFilePath))
                {
                    attempts = std::stoul(filesystem->readFile(trackerFilePath));
                }
            }
            catch (const Common::FileSystem::IFileSystemException& ex)
            {
                LOGWARN("Failed to read jrl tracker file " << trackerFilePath << " with error: " << ex.what());
            }
            catch (const std::exception& ex)
            {
                LOGWARN("Failed to convert contents of tracker file " << trackerFilePath << " to int with error: " << ex.what());
            }
            return attempts;
        }

        void Reader::updateJRLAttempts(const std::string& trackerFilePath, const u_int32_t attempts)
        {
            auto filesystem = Common::FileSystem::fileSystem();
            try
            {
                if (filesystem->isFile(trackerFilePath))
                {
                    filesystem->removeFile(trackerFilePath);
                }

                filesystem->writeFile(trackerFilePath, std::to_string(attempts));
            }
            catch (const Common::FileSystem::IFileSystemException& ex)
            {
                LOGWARN("Failed to update jrl tracker file " << trackerFilePath << " with error: " << ex.what());
            }
        }

        void Reader::clearJRLFile(const std::string& filePath)
        {
            auto fs = Common::FileSystem::fileSystem();
            try
            {
                if (fs->isFile(filePath))
                {
                    fs->removeFile(filePath);
                }
            }
            catch (const Common::FileSystem::IFileSystemException& ex)
            {
                LOGWARN("Failed to remove file " << filePath << " with error: " << ex.what());
            }
        }

        std::shared_ptr<Sophos::Journal::ViewInterface> Reader::getJournalView(std::vector<Subject> subjectFilter, const std::string& jrl, uint64_t startTime, uint64_t endTime)
        {
            std::vector<std::string> subjects;
            for (auto s : subjectFilter)
            {
                subjects.push_back(getSubjectName(s));
            }
            std::shared_ptr<Sophos::Journal::ViewInterface> view;

            if (!jrl.empty())
            {
                view = m_helper->GetJournalView(subjects, jrl);
            }
            else
            {
                if (endTime == 0)
                {
                    view = m_helper->GetJournalView(
                        subjects,
                        EventJournalWrapper::getFileTimeFromWindowsTime(
                            UtilityImpl::TimeUtils::EpochToWindowsFileTime(startTime)));
                }
                else
                {
                    view = m_helper->GetJournalView(
                        subjects,
                        EventJournalWrapper::getFileTimeFromWindowsTime(
                            UtilityImpl::TimeUtils::EpochToWindowsFileTime(startTime)),
                        EventJournalWrapper::getFileTimeFromWindowsTime(
                            UtilityImpl::TimeUtils::EpochToWindowsFileTime(endTime)));
                }
            }

            if (!view)
            {
                throw std::runtime_error("");
            }

            return view;
        }

        std::vector<Entry> Reader::getEntries(std::vector<Subject> subjectFilter, uint64_t startTime, uint64_t endTime, uint32_t maxMemoryThreshold, bool& moreAvailable)
        {
            return getEntries(subjectFilter, "", startTime, endTime, maxMemoryThreshold, moreAvailable);
        }

        std::vector<Entry> Reader::getEntries(std::vector<Subject> subjectFilter, const std::string& jrl, uint32_t maxMemoryThreshold, bool& moreAvailable)
        {
            return getEntries(subjectFilter, jrl, 0, 0, maxMemoryThreshold, moreAvailable);
        }

        std::vector<Entry> Reader::getEntries(std::vector<Subject> subjectFilter, const std::string& jrl, uint64_t startTime, uint64_t endTime, uint32_t maxMemoryThreshold, bool& moreAvailable)
        {
            std::vector<Entry> entries;
            auto view = getJournalView(std::move(subjectFilter), jrl, startTime, endTime);
            size_t entrySize = getEntrySize(view);
            if (entrySize == 0)
            {
                LOGDEBUG("No entries found");
                return entries;
            }
            uint64_t sizeRead = 0;
            uint64_t count = 0;
            bool more = false;
            for (auto it = view->cbegin(); it != view->cend(); ++it)
            {
                auto entry = *it;
                if (!entry)
                {
                    continue;
                }

                sizeRead += entry->GetDataSize();
                if (sizeRead > maxMemoryThreshold)
                {
                    LOGDEBUG("Reached journal read limit of " << maxMemoryThreshold/1000 << "kB after " << count << " records");
                    more = true;
                    break;
                }

                Entry e;
                e.producerUniqueID = entry->GetProducerUniqueId();
                e.timestamp = UtilityImpl::TimeUtils::WindowsFileTimeToEpoch(getWindowsFileTime(entry->GetTimestamp()));
                e.jrl = it.GetJournalResourceLocator();
                e.data.resize(entry->GetDataSize());
                memcpy(e.data.data(), entry->GetData(), entry->GetDataSize());
                entries.push_back(e);

                count++;
            }
            moreAvailable = more;
            return entries;
        }

        size_t Reader::getEntrySize(std::shared_ptr<Sophos::Journal::ViewInterface> journalView)
        {
            size_t size = 0;
            auto firstEntry = *(journalView->cbegin());
            if (firstEntry)
            {
                size = firstEntry->GetDataSize();
            }
            return size;
        }

        std::pair<bool, Detection> Reader::decode(const std::vector<uint8_t>& data)
        {
            Detection detection;
            auto array = kj::ArrayPtr<const capnp::word>(reinterpret_cast<const capnp::word*>(data.data()), data.size() / sizeof(capnp::word));
            capnp::FlatArrayMessageReader reader(array);
            auto event = reader.getRoot<Sophos::Journal::Event>();

            if (!event.isJson())
            {
                LOGWARN("unexpected detection event " << event.which());
                return std::make_pair(false, detection);
            }

            auto json = event.getJson();
            detection.subType = json.getSubType();
            detection.data = json.getData();

            return std::make_pair(true, detection);
        }

    } // namespace EventJournalWrapper
} // namespace EdrCommon
