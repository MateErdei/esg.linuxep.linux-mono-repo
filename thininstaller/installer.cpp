// Copyright 2018-2023 Sophos Limited. All rights reserved.
#include <curl.h>
#include <nlohmann/json.hpp>

#include "CentralRegistration/Main.h"
#include "Common/CurlWrapper/CurlWrapper.h"
#include "Common/CurlWrapper/ICurlWrapper.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/HttpRequestsImpl/HttpRequesterImpl.h"
#include "Common/Main/Main.h"
#include "Common/Policy/ALCPolicy.h"
#include "Common/Policy/MCSPolicy.h"
#include "Common/Policy/SerialiseUpdateSettings.h"
#include "Common/UtilityImpl/StringUtils.h"
#include "cmcsrouter/Config.h"
#include "cmcsrouter/ConfigOptions.h"
#include "cmcsrouter/MCSApiCalls.h"
#include <log4cplus/configurator.h>
#include <log4cplus/initializer.h>
#include <log4cplus/logger.h>

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <unistd.h>
#include <utility>
#include <vector>

static bool g_DebugMode = false;


// Used to check if an env proxy is in use.
const char* g_httpsProxy = getenv("https_proxy");

#define BIGBUF 81920

static char g_buf[BIGBUF];
static size_t g_bufptr = 0;
static std::string g_mcs_url;

class ServerAddress
{
private:
    std::string m_address;
    int m_priority;

public:
    [[nodiscard]] std::string getAddress() const { return m_address; }

    [[nodiscard]] int getPriority() const { return m_priority; }

    ServerAddress(std::string address, int priority);
};

ServerAddress::ServerAddress(std::string address, int priority) : m_address(std::move(address)), m_priority(priority) {}

/// Print out a log line
/// \param log The message to print
static void log(const std::string& log)
{
    std::cout << log << std::endl;
}

/// Print out an error log line
/// \param log The error message to print
static void logError(const std::string& log)
{
    std::cout << "ERROR: " << log << std::endl;
}

/// Print out log line if the product is running in debug mode
/// \param log The debug message to print
static void logDebug(const std::string& log)
{
    if (g_DebugMode)
    {
        std::cout << "DEBUG: " << log << std::endl;
    }
}

/// We only use this function as we need something to pass to libcurl to capture any data we may receive back
/// from the HTTPS GET to Sophos Central, we don't actually care what comes back.
/// This callback function gets called by libcurl as soon as there is data received that needs to be saved.
/// For most transfers, this callback gets called many times and each invoke delivers another chunk of data.
/// \param ptr Points to the delivered data
/// \param size Size is always 1
/// \param nmemb The size of that data
/// \return Returns the number of bytes dealt with. If that amount differs from the amount passed to this callback
/// function, libcurl will signal an error condition. This will cause the transfer to get aborted.
static size_t function_pt(char* ptr, size_t size, size_t nmemb, void*)
{
    size_t sizeOfData = size * nmemb;
    if (g_bufptr + sizeOfData >= BIGBUF)
    {
        logError("Response from CURL is larger than buffer");
        g_buf[g_bufptr] = 0;
        return 0;
    }
    memcpy(&g_buf[g_bufptr], ptr, sizeOfData);
    g_bufptr += sizeOfData;
    g_buf[g_bufptr] = 0;
    return sizeOfData;
}

class CurlSession
{
public:
    CurlSession()
    {
        if (curl_global_init(CURL_GLOBAL_DEFAULT) != 0)
        {
            throw std::runtime_error("Failed to init curl");
        }
    }

    ~CurlSession() { curl_global_cleanup(); }
};

/// Checks if we can connect to Sophos Central
/// \param proxy The full proxy string to use, if left empty then proxies will be explicitly turned off for this attempt
/// \return Returns true if we can connect to Sophos Central
static bool canConnectToCloud(const std::string& proxy = "")
{
    if (proxy.empty())
    {
        logDebug("Checking we can connect to Sophos Central (at " + g_mcs_url + ")");
    }
    else
    {
        logDebug("Checking we can connect to Sophos Central (at " + g_mcs_url + " via " + proxy + ")");
    }

    CURLcode res;
    bool ret = false;
    g_buf[0] = 0;

    CURL* curl = curl_easy_init();
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, g_mcs_url.c_str());
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 120);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, function_pt);
        curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);

        if (proxy.empty())
        {
            curl_easy_setopt(curl, CURLOPT_NOPROXY, "*");
            logDebug("Set CURLOPT_NOPROXY to *");
        }
        else
        {
            char* proxycreds = getenv("PROXY_CREDENTIALS");
            if (proxycreds != nullptr)
            {
                logDebug("PROXY_CREDENTIALS=" + std::string(proxycreds));
                curl_easy_setopt(curl, CURLOPT_PROXYUSERPWD, proxycreds);
                logDebug("Set CURLOPT_PROXYUSERPWD");
            }

            curl_easy_setopt(curl, CURLOPT_PROXYAUTH, CURLAUTH_ANY);
            logDebug("Set CURLOPT_PROXYAUTH to CURLAUTH_ANY");

            curl_easy_setopt(curl, CURLOPT_PROXY, proxy.c_str());
            logDebug("Set CURLOPT_PROXY to: " + proxy);
        }

        // Perform the request, res will get the return code
        res = curl_easy_perform(curl);

        // Check for errors
        if (res != CURLE_OK)
        {
            std::stringstream errorMessage;
            errorMessage << "Failed to connect to Sophos Central at " << g_mcs_url << " (cURL error is ["
                         << curl_easy_strerror(res) << "]). Please check your firewall rules";
            logDebug(errorMessage.str());
        }
        else
        {
            ret = true;
            logDebug("Successfully got [" + std::string(curl_easy_strerror(res)) + "] from Sophos Central");
        }

        // Always cleanup
        curl_easy_cleanup(curl);
    }
    return ret;
}

/// Check if we can contact Central via any of the following: Message Relays, env proxy, directly.
/// Note that we only log success here, failure message will be handled by the bash script.
/// \param proxies Vector of Message Relays which will be used first to test connectivity.
/// \return Return true if Central is contactable, false otherwise.
static bool canConnectToCloudDirectOrProxies(const std::vector<MCS::MessageRelay>& proxies)
{
    CurlSession init;
    log("Attempting to connect to Sophos Central");
    bool connected = false;

    // Try message relays
    for (auto& proxy : proxies)
    {
        std::string fullAddress = proxy.address+":"+proxy.port;
        if (canConnectToCloud(fullAddress))
        {
            connected = true;
            log("Successfully verified connection to Sophos Central via Message Relay: " + fullAddress);
            break;
        }
    }

    // Try via system proxy
    if (!connected && g_httpsProxy)
    {
        connected = canConnectToCloud(g_httpsProxy);
        if (connected)
        {
            log("Successfully verified connection to Sophos Central via proxy: " + std::string(g_httpsProxy));
        }
    }

    // Try direct
    if (!connected)
    {
        connected = canConnectToCloud();
        if (connected)
        {
            if (g_httpsProxy)
            {
                log("WARN: Could not connect using proxy");
            }
            log("Successfully verified connection to Sophos Central");
        }
    }

    return connected;
}

/// Splits a string by a deliminator and returns a vector of the split strings.
/// \param string_to_split
/// \param delim Deliminator used to split the string
/// \return
static std::vector<std::string> splitString(std::string string_to_split, const std::string& delim)
{
    std::vector<std::string> splitStrings = std::vector<std::string>();

    while (!string_to_split.empty())
    {
        std::size_t delimPos = string_to_split.find(delim);
        if (delimPos != std::string::npos)
        {
            std::string stripped_section = string_to_split.substr(0, delimPos);
            string_to_split = string_to_split.substr(delimPos + 1);
            splitStrings.push_back(stripped_section);
        }
        else
        {
            splitStrings.push_back(string_to_split);
            break;
        }
    }

    return splitStrings;
}

static std::vector<MCS::MessageRelay> extractRelays(const std::string& deliminated_addresses)
{
    std::vector<MCS::MessageRelay> relays;
    std::vector<std::string> proxyStrings = splitString(deliminated_addresses, ";");
    for (auto& proxyString : proxyStrings)
    {
        std::vector<std::string> addressAndPriority = splitString(proxyString, ",");
        if (addressAndPriority.size() != 3)
        {
            logError("Malformed proxy,priority,id: " + proxyString);
            continue;
        }


        std::vector<std::string> addressAndPort = splitString(addressAndPriority[0], ":");
        if (addressAndPort.size() != 2)
        {
            logError("Malformed address:port: " + addressAndPriority[0]);
            continue;
        }
        std::string address = addressAndPort[0];
        std::string port = addressAndPort[1];
        std::string priorityString = addressAndPriority[1];
        std::string id = addressAndPriority[2];
        int priority = 1;
        try
        {
            priority = std::stoi(priorityString);
        }
        catch (...)
        {
            logError("Malformed priority (" + priorityString + ") for proxy defaulting to 1");
            continue;
        }
        MCS::MessageRelay relay;
        relay.address = address;
        relay.priority = priority;
        relay.port = port;
        relay.id = id;
        relays.push_back(relay);
    }
    std::stable_sort(relays.begin(), relays.end(), [](const MCS::MessageRelay& p1, const MCS::MessageRelay& p2) {
                         return p1.priority < p2.priority;
                     });
    return relays;
}

static int inner_main(int argc, char** argv)
{
    g_DebugMode = static_cast<bool>(getenv("DEBUG_THIN_INSTALLER"));

    log4cplus::Initializer initializer;
    log4cplus::BasicConfigurator config;
    config.configure();
    if (g_DebugMode)
    {
        log4cplus::Logger::getRoot().setLogLevel(log4cplus::DEBUG_LOG_LEVEL);
        std::stringstream initMessage;
        initMessage << "Logger configured for level: DEBUG";
        log4cplus::Logger::getRoot().log(log4cplus::DEBUG_LOG_LEVEL, initMessage.str());
    }
    else
    {
        log4cplus::Logger::getRoot().setLogLevel(log4cplus::WARN_LOG_LEVEL);
    }


    if (argc < 2)
    {
        logError("Expecting a filename as an argument but none supplied");
        return 40;
    }
    std::string arg_filename = argv[1];
    logDebug("Filename = [" + arg_filename + "]");

    FILE* f = fopen(arg_filename.c_str(), "r");
    if (!f)
    {
        logError("Cannot open credentials file");
        return 41;
    }

    std::vector<std::string> registerArgValues;
    for (int i = 2; i < argc; ++i)
    {
        registerArgValues.emplace_back(argv[i]);
    }

    std::vector<MCS::MessageRelay> relays;
    std::vector<MCS::MessageRelay> update_caches;

    while (!feof(f))
    {
        char buf[4096];
        if (fgets(buf, 4096, f) == nullptr)
        {
            break;
        }

        std::string val = buf;
        std::size_t equals_pos = val.find('=');
        if (equals_pos != std::string::npos)
        {
            std::string argname = val.substr(0, equals_pos);
            std::string argvalue = val.substr(equals_pos + 1, std::string::npos);
            std::size_t newline_pos = argvalue.find('\n');
            if (newline_pos != std::string::npos)
            {
                argvalue = argvalue.substr(0, newline_pos);
            }

            if (argname == "URL")
            {
                g_mcs_url = argvalue;
            }
            else if (argname == "MESSAGE_RELAYS")
            {
                relays = extractRelays(argvalue);
            }
            else if (argname == "UPDATE_CACHES")
            {
                update_caches = extractRelays(argvalue);
            }
        }
    }
    fclose(f);

    if (g_mcs_url.empty())
    {
        logError("Didn't find mcs url in credentials file");
        return 42;
    }

    char* override_mcs_url = getenv("OVERRIDE_CLOUD_URL");
    if (override_mcs_url)
    {
        g_mcs_url = std::string(override_mcs_url);
    }

    try
    {

        if (!canConnectToCloudDirectOrProxies(relays))
        {
            // Exit 44 means cannot connect to Cloud - must correspond to handling in installer_header.sh
            return 44;
        }
    }
    catch(std::runtime_error& ex)
    {
        logError(ex.what());
        // Exit 44 means cannot connect to Cloud - must correspond to handling in installer_header.sh
        return 44;
    }
    auto fs = Common::FileSystem::fileSystem();
    bool forceLegacyInstall = static_cast<bool>(getenv("FORCE_LEGACY_INSTALL"));
    if (!forceLegacyInstall)
    {
        std::string mcsRootCert;
        auto thisBinary = fs->readlink("/proc/self/exe");
        if (thisBinary.has_value())
        {
            auto thisDir = Common::FileSystem::dirName(thisBinary.value());
            mcsRootCert = Common::FileSystem::join(thisDir, "..", "mcs_rootca.crt");
            if (!fs->isFile(mcsRootCert))
            {
                logError("Shipped MCS cert cannot be found here: " + mcsRootCert);
                return 55;
            }
            logDebug("Using shipped MCS cert: " + mcsRootCert);
        }
        else
        {
            return 56;
        }

        MCS::ConfigOptions rootConfigOptions = CentralRegistration::innerCentralRegistration(registerArgValues, mcsRootCert);

        if (rootConfigOptions.config[MCS::MCS_ID].empty())
        {
            return 51;
        }

        std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper =
            std::make_shared<Common::CurlWrapper::CurlWrapper>();
        std::shared_ptr<Common::HttpRequests::IHttpRequester> client =
            std::make_shared<Common::HttpRequestsImpl::HttpRequesterImpl>(curlWrapper);
        std::shared_ptr<MCS::MCSHttpClient> httpClient = std::make_shared<MCS::MCSHttpClient>(
            rootConfigOptions.config[MCS::MCS_URL],
            rootConfigOptions.config[MCS::MCS_CUSTOMER_TOKEN],
            std::move(client));

        if (!rootConfigOptions.config[MCS::MCS_CA_OVERRIDE].empty())
        {
            httpClient->setCertPath(rootConfigOptions.config[MCS::MCS_CA_OVERRIDE]);
        }
        else
        {
            httpClient->setCertPath(mcsRootCert);
        }
        httpClient->setID(rootConfigOptions.config[MCS::MCS_ID]);
        httpClient->setPassword(rootConfigOptions.config[MCS::MCS_PASSWORD]);
        if (!rootConfigOptions.config[MCS::MCS_CONNECTED_PROXY].empty())
        {
            httpClient->setProxyInfo(
                rootConfigOptions.config[MCS::MCS_CONNECTED_PROXY],
                rootConfigOptions.config[MCS::MCS_PROXY_USERNAME],
                rootConfigOptions.config[MCS::MCS_PROXY_PASSWORD]);
        }

        MCS::MCSApiCalls mcsCalls;
        std::map<std::string,std::string> jwt = mcsCalls.getAuthenticationInfo(httpClient);
        if (jwt[MCS::TENANT_ID].empty() || jwt[MCS::DEVICE_ID].empty() || jwt[MCS::JWT_TOKEN].empty())
        {
            log("Failed to get JWT from Sophos Central");
            return 52;
        }

        MCS::ConfigOptions policyOptions;
        policyOptions.config[MCS::MCS_ID] = rootConfigOptions.config[MCS::MCS_ID];
        policyOptions.config[MCS::MCS_PASSWORD] = rootConfigOptions.config[MCS::MCS_PASSWORD];
        policyOptions.config[MCS::TENANT_ID] = jwt[MCS::TENANT_ID];
        policyOptions.config[MCS::DEVICE_ID] = jwt[MCS::DEVICE_ID];
        policyOptions.config["jwt_token"] = jwt[MCS::JWT_TOKEN];

        std::optional<std::string> alcPolicyXml;
        std::optional<std::string> mcsPolicyXml;
        try
        {
            alcPolicyXml = mcsCalls.getPolicy(httpClient, "ALC", 1, std::chrono::seconds{900}, std::chrono::seconds{20});
        }
        catch (...)
        {
            logError("Failed to get ALC policy");
            return 53;
        }
        try
        {
            mcsPolicyXml = mcsCalls.getPolicy(httpClient, "MCS", 25, std::chrono::seconds{900}, std::chrono::seconds{20});
        }
        catch(...)
        {
            logError("Failed to get MCS policy");
            return 53;
        }

        Common::Policy::UpdateSettings updateSettings;

        if (alcPolicyXml.has_value())
        {
            try
            {
                logDebug("ALC policy: " + alcPolicyXml.value());
                auto alcPolicy = std::make_shared<Common::Policy::ALCPolicy>(alcPolicyXml.value());

                updateSettings = alcPolicy->getUpdateSettings();
                updateSettings.setJWToken(jwt[MCS::JWT_TOKEN]);
                updateSettings.setDeviceId(jwt[MCS::DEVICE_ID]);
                updateSettings.setTenantId(jwt[MCS::TENANT_ID]);
                updateSettings.setVersigPath(Common::FileSystem::join( fs->currentWorkingDirectory(),"bin/versig"));

                if (!rootConfigOptions.config[MCS::MCS_CONNECTED_PROXY].empty())
                {
                    logDebug("Proxy used for registration was: " + rootConfigOptions.config[MCS::MCS_CONNECTED_PROXY]);
                    updateSettings.setPolicyProxy(Common::Policy::Proxy(
                        rootConfigOptions.config[MCS::MCS_CONNECTED_PROXY],
                        Common::Policy::ProxyCredentials(
                            rootConfigOptions.config[MCS::MCS_PROXY_USERNAME],
                            rootConfigOptions.config[MCS::MCS_PROXY_USERNAME])));
                }

                // the policy certificate will always be the most up-to-date version of the uc certs
                // so we should use them in the case where the baked in certs and policy certs are different
                std::string updateCacheCertPath = Common::FileSystem::join(fs->currentWorkingDirectory(),"installer/uc_certs.crt");
                std::string updateCacheCertFolderPath = Common::FileSystem::join(fs->currentWorkingDirectory(),"installer");
                std::string updateCachePolicyCert = alcPolicy->getUpdateCertificatesContent();
                if (!updateCachePolicyCert.empty())
                {
                    fs->removeFile(updateCacheCertPath, true);
                    if(!fs->isDirectory(updateCacheCertFolderPath))
                    {
                        fs->makedirs(updateCacheCertFolderPath);
                    }
                    fs->writeFile(updateCacheCertPath,updateCachePolicyCert);
                }

                // we only use this field when using updateCaches so its fine to set all the time
                updateSettings.setUpdateCacheCertPath(updateCacheCertPath);

                std::string updateCacheOverride;
                auto val = std::getenv("UPDATE_CACHES_OVERRIDE");
                if (val != nullptr)
                {
                    updateCacheOverride = std::string(val);
                }

                if (updateCacheOverride == "none")
                {
                    updateSettings.setLocalUpdateCacheHosts({});
                }
                else
                {
                    if (!update_caches.empty())
                    {
                        // override policy update caches with the ones from the thinstaller
                        std::vector<std::string> updateCaches;
                        for (const auto& cache: update_caches)
                        {
                            std::string fullAddress = cache.address+":"+cache.port;
                            updateCaches.push_back(fullAddress);
                        }
                        updateSettings.setLocalUpdateCacheHosts(updateCaches);
                    }
                }

            }
            catch (const std::exception& exception)
            {
                std::stringstream errMessage;
                errMessage << "Failed to parse ALC policy with error: " << exception.what();
                logError(errMessage.str());
                return 53;
            }
            catch (...)
            {
                logError("Failure: Unknown exception parsing ALC policy");
                return 53;
            }
        }
        else
        {
            logError("Failure: ALC policy is empty");
            return 53;
        }

        if (mcsPolicyXml.has_value())
        {
            try
            {
                logDebug("MCS policy: " + mcsPolicyXml.value());
                auto mcsPolicy = std::make_shared<Common::Policy::MCSPolicy>(mcsPolicyXml.value());

                updateSettings.setLocalMessageRelayHosts(mcsPolicy->getMessageRelaysHostNames());

                std::string messageRelayOverride;
                auto val = std::getenv("MESSAGE_RELAY_OVERRIDE");
                if (val != nullptr)
                {
                    messageRelayOverride = std::string(val);
                }

                if (messageRelayOverride == "none")
                {
                    updateSettings.setLocalMessageRelayHosts({});
                }
                else
                {
                    if (!relays.empty())
                    {
                        // override policy message relays with the ones from the thinstaller
                        std::vector<std::string> messageRelays;
                        for (const auto& relay: relays)
                        {
                            std::string fullAddress = relay.address+":"+relay.port;
                            messageRelays.push_back(fullAddress);
                        }
                        updateSettings.setLocalMessageRelayHosts(messageRelays);
                    }
                }


            }
            catch (const std::exception& exception)
            {
                std::stringstream errMessage;
                errMessage << "Failed to parse MCS policy with error: " << exception.what();
                logError(errMessage.str());
                return 53;
            }
            catch (...)
            {
                logError("Failure: Unknown exception parsing MCS policy");
                return 53;
            }
        }

        // writing to update config
        try
        {
            auto updateConfigJson = Common::Policy::SerialiseUpdateSettings::toJsonSettings(updateSettings);
            logDebug("Writing to update config: " + updateConfigJson);
            fs->writeFile("./update_config.json", updateConfigJson);
        }
        catch(...)
        {
            logError("Failed to write to update config from translated ALC/MCS Policy");
            return 53;
        }

        int i = 1;
        MCS::ConfigOptions MsgConfig;
        for (auto const& messagerelay: relays)
        {
            std::string key = std::to_string(i);
            ++i;
            MsgConfig.config["message_relay_priority"+key] = messagerelay.priority;
            MsgConfig.config["message_relay_port"+key] = messagerelay.port;
            MsgConfig.config["message_relay_address"+key] = messagerelay.address;
            MsgConfig.config["message_relay_id"+key] = messagerelay.id;
        }

        rootConfigOptions.config[MCS::MCS_ID] = "";
        rootConfigOptions.config[MCS::MCS_PASSWORD] = "";
        rootConfigOptions.writeToDisk("./mcs.config");
        policyOptions.writeToDisk("./mcsPolicy.config");
        MsgConfig.writeToDisk("./mcs_policy.config");
    }
    return 0;
}
MAIN(inner_main(argc, argv))
