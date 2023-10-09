/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "TelemetryHelper.h"

#include "Logger.h"
#include "TelemetrySerialiser.h"

#include "modules/Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "modules/Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "modules/Common/FileSystemImpl/FileSystemImpl.h"
#include "modules/Common/UtilityImpl/StringUtils.h"

#include <cmath>
#include <functional>

namespace Common::Telemetry
{
    const unsigned int DEFAULT_MAX_JSON_SIZE = 1000000; // 1MB
    const char* ROOTKEY = "rootkey";
    const char* STATSKEY = "statskey";

    TelemetryHelper::TelemetryHelper() :
        m_fileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{ new Common::FileSystem::FileSystemImpl() })
    {
    }

    void TelemetryHelper::replaceFS(std::unique_ptr<Common::FileSystem::IFileSystem> fs)
    {
        m_fileSystem = std::move(fs);
    }

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
        return getTelemetryObjectByKey(keyPath, std::ref<TelemetryObject>(m_root));
    }

    TelemetryObject& TelemetryHelper::getTelemetryObjectByKey(
        const std::string& keyPath,
        std::reference_wrapper<TelemetryObject> root)
    {
        for (const auto& key : Common::UtilityImpl::StringUtils::splitString(keyPath, "."))
        {
            if (!root.get().keyExists(key))
            {
                root.get().set(key, TelemetryObject());
            }
            root = root.get().getObject(key);
        }
        // This is ok because root is passed in with a reference wrapper.
        // cppcheck-suppress returnReference
        return root;
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
        std::scoped_lock scopedLock(m_callbackLock, m_dataLock);
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
        std::scoped_lock scopedLock(m_callbackLock, m_dataLock);

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
        if (stick)
        {
            TelemetryObject& telemetryObjectStick =
                getTelemetryObjectByKey(key, std::ref<TelemetryObject>(m_resetToThis));
            telemetryObjectStick = object;
        }
    }

    void TelemetryHelper::appendStat(const std::string& statsKey, double value)
    {
        std::lock_guard<std::mutex> statsLock(m_dataLock);
        noLockAppendStat(statsKey, value);
    }

    void TelemetryHelper::noLockAppendStat(const std::string& statsKey, double value)
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

    double TelemetryHelper::getStatStdDeviation(const std::string& statsKey)
    {
        double statMean = TelemetryHelper::getStatAverage(statsKey);
        double sumation = 0;
        for (auto& stat : m_statsCollection[statsKey])
        {
            double term = stat - statMean;
            term = term * term;
            sumation += term;
        }
        double statStdDeviationSquared = sumation / m_statsCollection[statsKey].size();

        return sqrt(statStdDeviationSquared);
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
            set(keyValuePair.first + "-max", getStatMax(keyValuePair.first));
        }
    }

    void TelemetryHelper::updateTelemetryWithAllStdDeviationStats()
    {
        std::map<std::string, double> maxValues;
        for (const auto& keyValuePair : m_statsCollection)
        {
            set(keyValuePair.first + "-std-deviation", getStatStdDeviation(keyValuePair.first));
        }
    }

    void TelemetryHelper::noLockRestoreRoot(const TelemetryObject& savedTelemetryRoot) { m_root = savedTelemetryRoot; }

    void TelemetryHelper::save()
    {
        try
        {
            std::lock_guard<std::mutex> lock(m_dataLock);

            if (m_fileSystem->isDirectory(Common::FileSystem::dirName(m_saveTelemetryPath)))
            {
                TelemetryObject restoreTelemetryObj;
                restoreTelemetryObj.set(ROOTKEY, m_root);
                restoreTelemetryObj.set(STATSKEY, noLockStatsCollectionToTelemetryObject());

                auto output = TelemetrySerialiser::serialise(restoreTelemetryObj);

                auto tempDir = Common::ApplicationConfiguration::applicationPathManager().getTempPath();
                m_fileSystem->writeFileAtomically(m_saveTelemetryPath, output, tempDir);
            }
            else
            {
                LOGINFO("Restore directory " << Common::FileSystem::dirName(m_saveTelemetryPath) << " does not exists");
            }
        }
        catch (std::exception& ex)
        {
            LOGINFO("Unable to save telemetry reason: " << ex.what());
        }
    }

    void TelemetryHelper::restore(const std::string& pluginName)
    {
        try
        {
            auto restoreDir = Common::ApplicationConfiguration::applicationConfiguration().getData(
                Common::ApplicationConfiguration::TELEMETRY_RESTORE_DIR);
            m_saveTelemetryPath = Common::FileSystem::join(restoreDir, pluginName + "-telemetry.json");
        }
        catch (std::out_of_range& outOfRange)
        {
            LOGERROR("Telemetry restore directory path is not defined");
            return;
        }

        try
        {
            if (m_fileSystem->isFile(m_saveTelemetryPath))
            {
                auto input = m_fileSystem->readFile(m_saveTelemetryPath, DEFAULT_MAX_JSON_SIZE);
                auto savedTelemetryObject = TelemetrySerialiser::deserialise(input);

                std::lock_guard<std::mutex> dataLock(m_dataLock);
                noLockRestoreRoot(savedTelemetryObject.getObject(ROOTKEY));
                noLockUpdateStatsCollection(savedTelemetryObject.getObject(STATSKEY));
                m_fileSystem->removeFile(m_saveTelemetryPath);
            }
            else
            {
                LOGINFO("There is no saved telemetry at: " << m_saveTelemetryPath);
            }
        }
        catch (std::exception& ex)
        {
            LOGINFO("Restore Telemetry unsuccessful reason: " << ex.what());
            if (m_fileSystem->isFile(m_saveTelemetryPath))
            {
                try
                {
                    m_fileSystem->removeFile(m_saveTelemetryPath);
                }
                catch (std::exception& ex)
                {
                    LOGINFO("Failed to remove cached telemetry file, reason: " << ex.what());
                }
            }
        }
    }

    TelemetryObject TelemetryHelper::noLockStatsCollectionToTelemetryObject()
    {
        TelemetryObject statsTelemetryObj;
        for (const auto& values : m_statsCollection)
        {
            std::list<TelemetryObject> valuesTelemetryObject;
            for (const auto& value : values.second)
            {
                TelemetryObject valueTObj;
                valueTObj.set(TelemetryValue(value));
                valuesTelemetryObject.emplace_back(valueTObj);
            }
            statsTelemetryObj.set(values.first, valuesTelemetryObject);
        }
        return statsTelemetryObj;
    }

    void TelemetryHelper::noLockUpdateStatsCollection(const TelemetryObject& statsObject)
    {
        auto statsCollection = statsObject.getChildObjects();
        for (auto stat : statsCollection)
        {
            for (const auto& value : stat.second.getArray())
            {
                noLockAppendStat(stat.first, value.getValue().getDouble());
            }
        }
    }

    void TelemetryHelper::addValueToSet(const std::string& setKey, long value)
    {
        addValueToSetInternal(setKey, value);
    }

    void TelemetryHelper::addValueToSet(const std::string& setKey, unsigned long value)
    {
        addValueToSetInternal(setKey, value);
    }

    void TelemetryHelper::addValueToSet(const std::string& setKey, double value)
    {
        addValueToSetInternal(setKey, value);
    }

    void TelemetryHelper::addValueToSet(const std::string& setKey, const std::string& value)
    {
        addValueToSetInternal(setKey, value);
    }

    void TelemetryHelper::addValueToSet(const std::string& setKey, const char* value)
    {
        addValueToSetInternal(setKey, value);
    }

    void TelemetryHelper::addValueToSet(const std::string& setKey, bool value)
    {
        addValueToSetInternal(setKey, value);
    }

} // namespace Common::Telemetry