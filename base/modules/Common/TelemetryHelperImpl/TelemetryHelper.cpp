/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Logger.h"
#include "TelemetryHelper.h"

#include "TelemetrySerialiser.h"

#include <Common/UtilityImpl/StringUtils.h>

#include <functional>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/ApplicationConfigurationImpl/ApplicationPathManager.h>
#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/FileSystem/IFilePermissions.h>
#include <sys/stat.h>

namespace Common::Telemetry
{
    const unsigned int DEFAULT_MAX_JSON_SIZE = 1000000; // 1MB
    const char* ROOTKEY = "rootkey";
    const char* STATSKEY = "statskey";

    void TelemetryHelper::set(const std::string& key, long value) { setInternal(key, value, false); }
    void TelemetryHelper::set(const std::string& key, unsigned long value) { setInternal(key, value, false); }

    void TelemetryHelper::set(const std::string& key, double value) { setInternal(key, value, false); }

    void TelemetryHelper::set(const std::string& key, const std::string& value) { setInternal(key, value, false); }
    void TelemetryHelper::set(const std::string& key, const char* value) { setInternal(key, value, false); }

    void TelemetryHelper::set(const std::string& key, bool value) { setInternal(key, value, false); }

    void TelemetryHelper::increment(const std::string& key, long value) { incrementInternal(key, value); }

    void TelemetryHelper::increment(const std::string& key, unsigned long value) { incrementInternal(key, value); }

    void TelemetryHelper::appendValue(const std::string& arrayKey, long value) { appendValueInternal(arrayKey, value); }

    void TelemetryHelper::appendValue(const std::string& arrayKey, unsigned long value)
    {
        appendValueInternal(arrayKey, value);
    }

    void TelemetryHelper::appendValue(const std::string& arrayKey, double value)
    {
        appendValueInternal(arrayKey, value);
    }

    void TelemetryHelper::appendValue(const std::string& arrayKey, const std::string& value)
    {
        appendValueInternal(arrayKey, value);
    }

    void TelemetryHelper::appendValue(const std::string& arrayKey, const char* value)
    {
        appendValueInternal(arrayKey, value);
    }

    void TelemetryHelper::appendValue(const std::string& arrayKey, bool value) { appendValueInternal(arrayKey, value); }

    void TelemetryHelper::appendObject(const std::string& arrayKey, const std::string& key, long value)
    {
        appendObjectInternal(arrayKey, key, value);
    }

    void TelemetryHelper::appendObject(const std::string& arrayKey, const TelemetryObject& newObject)
    {
        std::lock_guard<std::mutex> lock(m_dataLock);
        TelemetryObject& telemetryObject = getTelemetryObjectByKey(arrayKey);

        // Force telemetry object to be an array, they are defaulted to object type.
        if (telemetryObject.getType() != TelemetryObject::Type::array)
        {
            telemetryObject.set(std::list<TelemetryObject>());
        }

        std::list<TelemetryObject>& list = telemetryObject.getArray();
        list.emplace_back(newObject);
    }

    void TelemetryHelper::appendObject(const std::string& arrayKey, const std::string& key, unsigned long value)
    {
        appendObjectInternal(arrayKey, key, value);
    }

    void TelemetryHelper::appendObject(const std::string& arrayKey, const std::string& key, double value)
    {
        appendObjectInternal(arrayKey, key, value);
    }

    void TelemetryHelper::appendObject(const std::string& arrayKey, const std::string& key, const std::string& value)
    {
        appendObjectInternal(arrayKey, key, value);
    }

    void TelemetryHelper::appendObject(const std::string& arrayKey, const std::string& key, const char* value)
    {
        appendObjectInternal(arrayKey, key, value);
    }

    void TelemetryHelper::appendObject(const std::string& arrayKey, const std::string& key, bool value)
    {
        appendObjectInternal(arrayKey, key, value);
    }

    TelemetryObject& TelemetryHelper::getTelemetryObjectByKey(const std::string& keyPath)
    {
        return getTelemetryObjectByKey(keyPath, m_root);
    }

    TelemetryObject& TelemetryHelper::getTelemetryObjectByKey(const std::string& keyPath, TelemetryObject& root)
    {
        std::reference_wrapper<TelemetryObject> currentTelemObj = root;
        for (const auto& key : Common::UtilityImpl::StringUtils::splitString(keyPath, "."))
        {
            if (!currentTelemObj.get().keyExists(key))
            {
                currentTelemObj.get().set(key, TelemetryObject());
            }
            currentTelemObj = currentTelemObj.get().getObject(key);
        }
        return currentTelemObj;
    }


    std::string TelemetryHelper::serialise()
    {
        std::lock_guard<std::mutex> lock(m_dataLock);
        return TelemetrySerialiser::serialise(m_root);
    }

    void TelemetryHelper::registerResetCallback(std::string cookie, std::function<void(TelemetryHelper&)> function)
    {
        std::lock_guard<std::mutex> lock(m_callbackLock);
        if (m_callbacks.find(cookie) != m_callbacks.end())
        {
            throw std::logic_error("Callback already registered with cookie: " + cookie);
        }
        m_callbacks[cookie] = function;
    }

    void TelemetryHelper::unregisterResetCallback(std::string cookie)
    {
        std::lock_guard<std::mutex> lock(m_callbackLock);
        m_callbacks.erase(cookie);
    }

    void TelemetryHelper::reset()
    {
        std::scoped_lock scopedLock( m_callbackLock, m_dataLock);
        locked_reset();
    }
    void TelemetryHelper::locked_reset()
    {
        m_statsCollection = {};
        TelemetryHelper another;
        another.m_root = m_resetToThis;
        for (const auto& callback_entry : m_callbacks)
        {
            if (callback_entry.second)
            {
                callback_entry.second(another);
            }
        }
        m_root = another.m_root;

    }

    void TelemetryHelper::clearData()
    {
        std::lock_guard<std::mutex> dataLock(m_dataLock);
        m_root = m_resetToThis;
    }

    void TelemetryHelper::mergeJsonIn(const std::string& key, const std::string& json)
    {
        std::lock_guard<std::mutex> dataLock(m_dataLock);
        TelemetryObject telemetryObject = TelemetrySerialiser::deserialise(json);
        getTelemetryObjectByKey(key) = telemetryObject;
    }

    std::string TelemetryHelper::serialiseAndReset()
    {
        std::scoped_lock scopedLock( m_callbackLock, m_dataLock);

        // Serialise
        std::string serialised = TelemetrySerialiser::serialise(m_root);
        locked_reset();
        return serialised;
    }


    void TelemetryHelper::set(const std::string& key, const TelemetryObject& object, bool stick)
    {
        std::lock_guard<std::mutex> lock(m_dataLock);
        TelemetryObject& telemetryObject = getTelemetryObjectByKey(key);
        telemetryObject = object;
        if ( stick)
        {
            TelemetryObject& telemetryObjectStick = getTelemetryObjectByKey(key, m_resetToThis);
            telemetryObjectStick = object;
        }
    }

    void TelemetryHelper::appendStat(const std::string& statsKey, double value)
    {
        std::lock_guard<std::mutex> statsLock(m_dataLock);
        lockedAppendStat(statsKey, value);
    }

    void TelemetryHelper::lockedAppendStat(const std::string& statsKey, double value)
    {
        m_statsCollection[statsKey].push_back(value);
    }

    double TelemetryHelper::getStatAverage(const std::string& statsKey)
    {
        if (m_statsCollection[statsKey].empty())
        {
            return 0;
        }

        double sumation = 0;
        for (auto& stat : m_statsCollection[statsKey])
        {
            sumation += stat;
        }

        return sumation / m_statsCollection[statsKey].size();
    }

    double TelemetryHelper::getStatMin(const std::string& statsKey)
    {
        return *std::min_element(m_statsCollection[statsKey].begin(), m_statsCollection[statsKey].end());
    }

    double TelemetryHelper::getStatMax(const std::string& statsKey)
    {
        return *std::max_element(m_statsCollection[statsKey].begin(), m_statsCollection[statsKey].end());
    }

    void TelemetryHelper::updateTelemetryWithStats()
    {
        updateTelemetryWithAllAverageStats();
        updateTelemetryWithAllMinStats();
        updateTelemetryWithAllMaxStats();
    }

    void TelemetryHelper::updateTelemetryWithAllAverageStats()
    {
        std::map<std::string, double> averages;
        for (const auto& keyValuePair : m_statsCollection)
        {
            set(keyValuePair.first + "-avg", getStatAverage(keyValuePair.first));
        }
    }

    void TelemetryHelper::updateTelemetryWithAllMinStats()
    {
        std::map<std::string, double> minValues;
        for (const auto& keyValuePair : m_statsCollection)
        {
            set(keyValuePair.first + "-min", getStatMin(keyValuePair.first));
        }
    }

    void TelemetryHelper::updateTelemetryWithAllMaxStats()
    {
        std::map<std::string, double> maxValues;
        for (const auto& keyValuePair : m_statsCollection)
        {
            set(keyValuePair.first + "-max",getStatMax(keyValuePair.first));
        }
    }

    void TelemetryHelper::lockedRestoreRoot(const TelemetryObject& savedTelemetryRoot)
    {
        m_root = savedTelemetryRoot;
    }

    void TelemetryHelper::save()
    {
        try
        {
            std::lock_guard<std::mutex> lock(m_dataLock);

            auto fs = Common::FileSystem::fileSystem();
            if(fs->isDirectory(Common::FileSystem::dirName(m_saveTelemetryPath)))
            {
                TelemetryObject restoreTelemetryObj;
                restoreTelemetryObj.set(ROOTKEY, m_root);
                restoreTelemetryObj.set(STATSKEY, lockedStatsCollectionToTelemetryObject());

                auto output = TelemetrySerialiser::serialise(restoreTelemetryObj);

                auto tempDir = Common::ApplicationConfigurationImpl::ApplicationPathManager().getTempPath();
                fs->writeFileAtomically(m_saveTelemetryPath, output, tempDir);
                Common::FileSystem::filePermissions()->chmod(m_saveTelemetryPath, S_IRUSR | S_IWUSR);
            }
            else
            {
                LOGINFO("Restore directory " << Common::FileSystem::dirName(m_saveTelemetryPath) << " does not exists");
            }
        }
        catch(std::exception& ex)
        {
            LOGINFO("Unable to save telemetry reason: " << ex.what());
        }
    }

    void TelemetryHelper::restore(const std::string &pluginName)
    {
        try
        {
            auto restoreDir = Common::ApplicationConfiguration::applicationConfiguration().getData(
                    Common::ApplicationConfiguration::TELEMETRY_RESTORE_DIR);
            m_saveTelemetryPath = Common::FileSystem::join(restoreDir, pluginName + "-telemetry.json");
        }
        catch(std::out_of_range& outOfRange)
        {
            LOGERROR("Telemetry restore directory path is not defined");
            return;
        }

        auto fs = Common::FileSystem::fileSystem();
        try
        {
            if (fs->isFile(m_saveTelemetryPath))
            {
                auto input = fs->readFile(m_saveTelemetryPath, DEFAULT_MAX_JSON_SIZE);
                auto savedTelemetryObject = TelemetrySerialiser::deserialise(input);

                std::lock_guard<std::mutex> dataLock(m_dataLock);
                lockedRestoreRoot(savedTelemetryObject.getObject(ROOTKEY));
                lockedUpdateStatsCollection(savedTelemetryObject.getObject(STATSKEY));
                fs->removeFile(m_saveTelemetryPath);
            }
            else
            {
                LOGINFO("There is no saved telemetry at: " << m_saveTelemetryPath);
            }
        }
        catch(std::exception& ex)
        {
            LOGINFO("Restore Telemetry unsuccessful reason: " << ex.what());
            if( fs->isFile(m_saveTelemetryPath) )
            {
                try {
                    fs->removeFile(m_saveTelemetryPath);
                }
                catch(std::exception& ex)
                {
                    LOGINFO("Failed to remove cached telemetry file, reason: " << ex.what());
                }
            }
        }
    }

    TelemetryObject TelemetryHelper::lockedStatsCollectionToTelemetryObject()
    {
        TelemetryObject statsTelemetryObj;
        for(const auto& values : m_statsCollection)
        {
            std::list<TelemetryObject> valuesTelemetryObject;
            for(const auto& value : values.second)
            {
                TelemetryObject valueTObj;
                valueTObj.set(TelemetryValue(value));
                valuesTelemetryObject.emplace_back(valueTObj);
            }
            statsTelemetryObj.set(values.first, valuesTelemetryObject);
        }
        return statsTelemetryObj;
    }

    void TelemetryHelper::lockedUpdateStatsCollection(const TelemetryObject& statsObject)
    {
       auto statsCollection = statsObject.getChildObjects();
       for( auto stat : statsCollection)
       {
           for(const auto& value : stat.second.getArray())
           {
               lockedAppendStat(stat.first, value.getValue().getDouble());
           }
       }
    }
} // namespace Common::Telemetry