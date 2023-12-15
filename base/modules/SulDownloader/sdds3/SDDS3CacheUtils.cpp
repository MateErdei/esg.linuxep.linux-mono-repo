// Copyright 2023 Sophos Limited. All rights reserved.

#include "SDDS3CacheUtils.h"

#include "Logger.h"
#include "Sdds3Wrapper.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "base/modules/Common/Policy/UpdateSettings.h"

namespace
{
    std::string getRawXMLFromDatFile(const std::string& suiteDatFilePath, Common::FileSystem::IFileSystem* fs)
    {
        std::string data = fs->readFile(suiteDatFilePath);
        return SulDownloader::sdds3Wrapper()->getUnverifiedSignedBlob(data);
    }

    Common::XmlUtilities::AttributesMap getXMLFromDatFileWithFS(const std::string& suiteDatFilePath, Common::FileSystem::IFileSystem* fs)
    {
        auto xml = getRawXMLFromDatFile(suiteDatFilePath, fs);
        Common::XmlUtilities::AttributesMap attributesMap{ {}, {} };
        try
        {
            attributesMap = Common::XmlUtilities::parseXml(xml);
        }
        catch (const Common::XmlUtilities::XmlUtilitiesException& ex)
        {
            LOGERROR("Failed to parse xml: " << ex.what());
        }

        return attributesMap;
    }
}

namespace SulDownloader
{
    Common::XmlUtilities::AttributesMap SDDS3CacheUtils::getXMLFromDatFile(const std::string& suiteDatFilePath)
    {
        return getXMLFromDatFileWithFS(suiteDatFilePath, Common::FileSystem::fileSystem());
    }

    std::vector<std::pair<std::string, std::string>> SDDS3CacheUtils::getSupplementRefsFromBaseSuite(
        const std::string& baseSuitePath)
    {
        Common::XmlUtilities::AttributesMap suite = getXMLFromDatFile(baseSuitePath);
        auto packages = suite.entitiesThatContainPath("suite/package-ref", false);

        std::vector<std::string> filteredPackages;
        std::vector<std::pair<std::string, std::string>> filteredSupplements;
        std::string arch = Common::Policy::machineArchitecture_;

        for (const auto& package : packages)
        {
            std::vector<std::string> platformElements = suite.entitiesThatContainPath(package + "/platforms/platform");
            for (const auto& element : platformElements)
            {
                if (suite.lookup(element).value("name") == arch)
                {
                    filteredPackages.emplace_back(package);
                    break;
                }
            }
        }

        // get supplement name and tag out of  filtered packages
        for (const auto& package : filteredPackages)
        {
            std::vector<std::string> supplements = suite.entitiesThatContainPath(package + "/supplement-ref");
            for (const auto& element : supplements)
            {
                auto supplement = suite.lookup(element);
                filteredSupplements.emplace_back(supplement.value("src"), supplement.value("tag"));
            }
        }
        return filteredSupplements;
    }

    bool SDDS3CacheUtils::isCacheUpToDate(const std::string& baseSuitePath, const std::set<std::string>& releaseGroups)
    {
        auto fs = Common::FileSystem::fileSystem();
        std::vector<std::pair<std::string, std::string>> supplements;
        if (fs->isFile(baseSuitePath))
        {
            supplements = getSupplementRefsFromBaseSuite(baseSuitePath);
        }
        else
        {
            LOGWARN("Missing suite data file that should be cached: " << baseSuitePath);
            return false;
        }

        // for each supplement open up the dat file and check if there is a matching tag with
        // matching release group if release groups are set
        for (const auto& [src, releaseTag] : supplements)
        {
            bool foundTag = false;
            std::string supplementDatFilePath = Common::FileSystem::join(
                Common::ApplicationConfiguration::applicationPathManager().getLocalSdds3Repository(),
                "supplement",
                "sdds3." + src + ".dat");

            if (fs->isFile(supplementDatFilePath))
            {
                Common::XmlUtilities::AttributesMap supplementData = getXMLFromDatFile(supplementDatFilePath);
                auto supplementPackages = supplementData.entitiesThatContainPath("supplement/package-ref", false);

                for (const auto& package : supplementPackages)
                {
                    auto tags = supplementData.entitiesThatContainPath(package + "/tags/tag", false);
                    for (const auto& tag : tags)
                    {
                        auto tagAttributes = supplementData.lookup(tag);

                        if (tagAttributes.value("name") == releaseTag)
                        {
                            auto groups = supplementData.entitiesThatContainPath(tag + "/release-group");
                            if (!groups.empty())
                            {
                                for (const auto& group : groups)
                                {
                                    std::string groupName = supplementData.lookup(group).value("name");
                                    if (releaseGroups.find(groupName) != releaseGroups.end())
                                    {
                                        foundTag = true;
                                        break;
                                    }
                                }
                            }
                            else
                            {
                                // No tags means applies to all releaseGroups
                                foundTag = true;
                                break;
                            }
                        }
                    }
                    if (foundTag)
                    {
                        break;
                    }
                }
                if (!foundTag)
                {
                    LOGWARN("Cannot find entry matching tag: " << releaseTag << " for supplement: " << src);
                    auto* fs = Common::FileSystem::fileSystem();
                    auto xml = getRawXMLFromDatFile(supplementDatFilePath, fs);
                    LOGINFO("Failed to find matching tag in XML from " << supplementDatFilePath << ": " << xml);
                    return false;
                }
            }
            else
            {
                // if there is no dat file for the supplement then cache is not up to date
                LOGWARN("Missing supplement data file that should be cached: " << supplementDatFilePath);
                return false;
            }
        }
        return true;
    }

    bool SDDS3CacheUtils::checkCache(const std::set<std::string>& suites, const std::set<std::string>& releaseGroups)
    {
        std::string baseSuite;
        for (const auto& suite : suites)
        {
            if (suite.find("ServerProtectionLinux-Base") != suite.npos)
            {
                baseSuite = suite;
            }
        }

        if (baseSuite.empty())
        {
            clearEntireCache();
            return false;
        }
        else
        {
            std::string baseSuitePath = Common::FileSystem::join(
                Common::ApplicationConfiguration::applicationPathManager().getLocalSdds3Repository(),
                "suite",
                baseSuite);
            if (!isCacheUpToDate(baseSuitePath, releaseGroups))
            {
                clearEntireCache();
                return false;
            }
        }
        return true;
    }

    void SDDS3CacheUtils::clearEntireCache()
    {
        LOGWARN("The sophos update cache was out of sync with config so it is being cleared");
        auto fs = Common::FileSystem::fileSystem();
        try
        {
            fs->removeFile(Common::ApplicationConfiguration::applicationPathManager().getSdds3PackageConfigPath());
            fs->recursivelyDeleteContentsOfDirectory(
                Common::ApplicationConfiguration::applicationPathManager().getLocalSdds3Repository());
            fs->recursivelyDeleteContentsOfDirectory(
                Common::ApplicationConfiguration::applicationPathManager().getLocalSdds3DistributionRepository());
            LOGWARN("Cache cleared");
        }
        catch (const Common::FileSystem::IFileSystemException& ex)
        {
            LOGWARN("Failed to clear cache with error: " << ex.what());
        }
    }
} // namespace SulDownloader