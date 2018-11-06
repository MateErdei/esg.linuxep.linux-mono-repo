/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ScannerSettings.h"
#include "StringReplace.h"
#include "Logger.h"
#include <regex>

namespace
{
    bool isPolicyType2(const std::string & policyXml)
    {
        return policyXml.find("policyType=\"2\"") != std::string::npos;
    }

    std::string returnFirstMatch(const std::string & pattern, const std::string & content)
    {
        std::regex regexPattern {pattern};
        std::smatch matches;

        if (std::regex_search(content, matches, regexPattern))
        {
            if (1 < matches.size() && matches[1].matched)
            {
                return matches[1];
            }
        }
        return std::string();
    }

    std::string extractRevId(const std::string & policyXml)
    {
        std::string RevIdPattern {R"sophos(csc:Comp xmlns:csc="com.sophos\\msys\\csc" RevID="([a-z0-9]+)" policyType="2")sophos"};
        return returnFirstMatch(RevIdPattern, policyXml);
    }

    bool extractOnAccessEnabled(const std::string & policyXml)
    {
        std::string OnAccessPattern{R"sophos(<onAccessScan>[^<]+<enabled>(true|false)</enabled>[^]*</onAccessScan>)sophos"};
        return returnFirstMatch(OnAccessPattern, policyXml) == "true";
    }

    std::vector<std::string> extractExclusions(const std::string & policyXml)
    {
        std::string exclusionsPattern {R"sophos(<linuxExclusions>[^]*<filePathSet>([^]*)</filePathSet>[^]*</linuxExclusions>)sophos"};
        std::regex filePathPattern {R"sophos(<filePath>(.*)</filePath>)sophos"};

        std::vector<std::string> extractExclusions = {};
        std::string filePaths = returnFirstMatch(exclusionsPattern, policyXml);

        std::sregex_token_iterator end{};
        for(std::sregex_token_iterator p{filePaths.begin(), filePaths.end(), filePathPattern, {1}}; p != end; ++p)
        {
            extractExclusions.emplace_back(*p);
        }

        return extractExclusions;
    }

    bool extractScheduledScanEnabled(const std::string & policyXml)
    {
        return policyXml.find("<scanSet/>") == std::string::npos;
    }
}

namespace Example
{
    void ScannerSettings::ProcessPolicy(const std::string &policyXml)
    {

        if (!isPolicyType2(policyXml))
        {
            LOGINFO("Ignoring invalid policy");
            return;
        }
        m_policy.onAccess = extractOnAccessEnabled(policyXml);
        m_policy.scheduled = extractScheduledScanEnabled(policyXml);
        m_policy.exclusions = extractExclusions(policyXml);
        m_policy.revId = extractRevId(policyXml);

        logSettings();
    }

    Common::PluginApi::StatusInfo ScannerSettings::getStatus() const
    {
        std::string statusTemplate{R"sophos(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<status xmlns="http://www.sophos.com/EE/EESavStatus">
    <csc:CompRes xmlns:csc="com.sophos\msys\csc" Res="RESKEY" RevID="REVIDKEY" policyType="2" />
<upToDateState>1</upToDateState>
<vdl-info>
<virus-engine-version>3.66</virus-engine-version>
<virus-data-version>5.40</virus-data-version>
<idelist></idelist>
<ideChecksum></ideChecksum>
</vdl-info>
<on-access>ONACCESSKEY</on-access>
<entity><productId>SAV</productId><product-version></product-version><entityInfo>SAV</entityInfo></entity>
</status>)sophos"};

        KeyValueCollection keyvalues{
                {"RESKEY", m_policy.scheduled ? "Diff" : "Same"},
                {"REVIDKEY", m_policy.revId},
                {"ONACCESSKEY", m_policy.onAccess ? "true" : "false"}
        };
        std::string statusString = orderedStringReplace(statusTemplate, keyvalues);
        return Common::PluginApi::StatusInfo{statusString, statusString, "SAV"};
    }

    bool ScannerSettings::isEnabled() const
    {
        return m_policy.onAccess;
    }

    std::vector<std::string> ScannerSettings::getExclusions() const
    {
        return m_policy.exclusions;
    }

    void ScannerSettings::logSettings() {
        std::stringstream logMessage;
        logMessage << "Current Settings\n";
        logMessage << "\tActive: " << m_policy.onAccess << "\n";
        logMessage << "\tCompliant with RevID " << m_policy.revId << ": " << !m_policy.scheduled;
        if (!m_policy.exclusions.empty())
        {
            for (auto &exclusion : m_policy.exclusions) {
                logMessage << "\n\tExclusion: " << exclusion;
            }
        }
        else
        {
            logMessage << "\n\tNo Exclusions";
        }
        LOGINFO(logMessage.str());
    }
}