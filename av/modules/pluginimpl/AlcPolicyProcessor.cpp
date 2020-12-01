/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "AlcPolicyProcessor.h"

#include "Logger.h"

#include <pluginimpl/ObfuscationImpl/Obfuscate.h>

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>
#include <common/StringUtils.h>

static std::string getPluginInstall()
{
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    return appConfig.getData("PLUGIN_INSTALL");
}

bool Plugin::AlcPolicyProcessor::processAlcPolicy(const Common::XmlUtilities::AttributesMap& policy)
{
    auto oldCustomerId = m_customerId;
    m_customerId = getCustomerId(policy);
    if (m_customerId.empty())
    {
        LOGWARN("Failed to find customer ID from ALC policy");
    }
    else
    {
        // write customer ID to a file
        auto* fs = Common::FileSystem::fileSystem();
        auto pluginInstall = getPluginInstall();
        auto dest = pluginInstall + "/var/customer_id.txt";
        fs->writeFile(dest, m_customerId);
        auto chroot = pluginInstall + "/chroot";
        dest = chroot + dest;
        fs->writeFile(dest, m_customerId);
    }
    return (m_customerId != oldCustomerId && !oldCustomerId.empty());
}

std::string Plugin::AlcPolicyProcessor::getCustomerId(const Common::XmlUtilities::AttributesMap& policy)
{
    auto primaryLocation = policy.lookup("AUConfigurations/AUConfig/primary_location/server");
    if (primaryLocation.empty())
    {
        return "";
    }
    std::string username{ primaryLocation.value("UserName") };
    std::string password{ primaryLocation.value("UserPassword") };
    std::string algorithm{ primaryLocation.value("Algorithm", "Clear") };

    // we check that username and password are not empty mainly for fuzzing purposes as in
    // product we never expect central to send us a policy with empty credentials
    if (password.empty())
    {
        throw std::invalid_argument("Invalid policy: Password is empty ");
    }
    if (username.empty())
    {
        throw std::invalid_argument("Invalid policy: Username is empty ");
    }

    bool unhashed = true;
    std::string cred;

    if (algorithm == "AES256")
    {
        password = Common::ObfuscationImpl::SECDeobfuscate(password);
    }
    else if (algorithm != "Clear")
    {
        throw std::invalid_argument("Invalid policy: Unknown obfuscation algorithm");
    }

    if (username.size() == 32 && username == password)
    {
        unhashed = false;
        cred = username;
    }

    if (unhashed)
    {
        cred = common::md5_hash(username + ':' + password);
    }
    return common::md5_hash(cred); // always do the second hash
}