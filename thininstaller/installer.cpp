#include "curl.h"
#include "json.hpp"

#include <CentralRegistration/Main.h>
#include <cmcsrouter/Config.h>
#include <cmcsrouter/ConfigOptions.h>
#include <cmcsrouter/MCSApiCalls.h>

#include <Common/FileSystem/IFileSystem.h>
#include <Common/UtilityImpl/StringUtils.h>

#include <log4cplus/logger.h>
#include <log4cplus/configurator.h>
#include <log4cplus/initializer.h>

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
static int g_bufptr = 0;
static std::string g_mcs_url;

class ServerAddress
{
private:
    std::string m_address;
    int m_priority;

public:
    std::string getAddress() const { return m_address; }

    int getPriority() const { return m_priority; }

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

            curl_easy_setopt(curl, CURLOPT_PROXYAUTH, CURLAUTH_ANY); // NOLINT
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
static bool canConnectToCloudDirectOrProxies(const std::vector<ServerAddress>& proxies)
{
    CurlSession init;
    log("Attempting to connect to Sophos Central");
    bool connected = false;

    // Try message relays
    for (auto& proxy : proxies)
    {
        if (canConnectToCloud(proxy.getAddress()))
        {
            connected = true;
            log("Successfully verified connection to Sophos Central via Message Relay: " + proxy.getAddress());
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

/// Returns a vector of server addresses which can represent Update Caches or Message Relays
/// \param deliminated_addresses should be in the format: hostname1:port1,priority1,id1;hostname2:port2,priority2,id2...
/// \return
static std::vector<ServerAddress> extractPrioritisedAddresses(const std::string& deliminated_addresses)
{
    std::vector<ServerAddress> proxies;
    std::vector<std::string> proxyStrings = splitString(deliminated_addresses, ";");
    for (auto& proxyString : proxyStrings)
    {
        std::vector<std::string> addressAndPriority = splitString(proxyString, ",");
        if (addressAndPriority.size() < 2)
        {
            logError("Malformed proxy,priority,id: " + proxyString);
            continue;
        }

        // Note, we ignore the MR or UC ID
        std::string address = addressAndPriority[0];
        std::string priorityString = addressAndPriority[1];
        int priority;
        try
        {
            priority = std::stoi(priorityString);
        }
        catch (...)
        {
            logError("Malformed priority (" + priorityString + ") for proxy defaulting to 1");
            continue;
        }
        ServerAddress proxy = ServerAddress(address, priority);
        proxies.push_back(proxy);
    }
    std::stable_sort(proxies.begin(), proxies.end(), [](const ServerAddress& p1, const ServerAddress& p2) {
        return p1.getPriority() < p2.getPriority();
    });
    return proxies;
}

int main(int argc, char** argv)
{
    g_DebugMode = static_cast<bool>(getenv("DEBUG_THIN_INSTALLER"));

    log4cplus::Initializer initializer;
    log4cplus::BasicConfigurator config;
    config.configure();
    if (g_DebugMode)
    {
        log4cplus::Logger::getRoot().setLogLevel(log4cplus::DEBUG_LOG_LEVEL);
        std::stringstream initMessage;
        initMessage << "Logger configured for level: DEBUG" << std::endl;
        log4cplus::Logger::getRoot().log(log4cplus::DEBUG_LOG_LEVEL, initMessage.str());
    }
    else
    {
        log4cplus::Logger::getRoot().setLogLevel(log4cplus::INFO_LOG_LEVEL);
        std::stringstream initMessage;
        initMessage << "Logger configured for level: INFO" << std::endl;
        log4cplus::Logger::getRoot().log(log4cplus::INFO_LOG_LEVEL, initMessage.str());
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

    std::vector<ServerAddress> relays;
    std::vector<ServerAddress> update_caches;

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
                relays = extractPrioritisedAddresses(argvalue);
            }
            else if (argname == "UPDATE_CACHES")
            {
                update_caches = extractPrioritisedAddresses(argvalue);
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

    bool forceLegacyInstall = static_cast<bool>(getenv("FORCE_LEGACY_INSTALL"));
    if (!forceLegacyInstall)
    {
        MCS::ConfigOptions rootConfigOptions = CentralRegistration::innerCentralRegistration(registerArgValues);

        if (rootConfigOptions.config[MCS::MCS_ID].empty())
        {
            return 51;
        }
        log("Successfully registered with Sophos Central");

        std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper =
            std::make_shared<Common::CurlWrapper::CurlWrapper>();
        std::shared_ptr<Common::HttpRequests::IHttpRequester> client =
            std::make_shared<Common::HttpRequestsImpl::HttpRequesterImpl>(curlWrapper);
        MCS::MCSHttpClient httpClient(
            rootConfigOptions.config[MCS::MCS_URL],
            rootConfigOptions.config[MCS::MCS_CUSTOMER_TOKEN],
            std::move(client));

        if (!rootConfigOptions.config[MCS::MCS_CA_OVERRIDE].empty())
        {
            httpClient.setCertPath(rootConfigOptions.config[MCS::MCS_CA_OVERRIDE]);
        }
        else
        {
            httpClient.setCertPath("./mcs_rootca.crt");
        }
        httpClient.setID(rootConfigOptions.config[MCS::MCS_ID]);
        httpClient.setPassword(rootConfigOptions.config[MCS::MCS_PASSWORD]);
        if (!rootConfigOptions.config[MCS::MCS_CONNECTED_PROXY].empty())
        {
            httpClient.setProxyInfo(
                rootConfigOptions.config[MCS::MCS_CONNECTED_PROXY],
                rootConfigOptions.config[MCS::MCS_PROXY_USERNAME],
                rootConfigOptions.config[MCS::MCS_PROXY_PASSWORD]);
        }
        std::map<std::string,std::string> jwt = MCS::MCSApiCalls().getAuthenticationInfo(httpClient);
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

        rootConfigOptions.config[MCS::MCS_ID] = "";
        rootConfigOptions.config[MCS::MCS_PASSWORD] = "";
        rootConfigOptions.writeToDisk("./mcs.config");
        policyOptions.writeToDisk("./mcsPolicy.config");
        auto fs = Common::FileSystem::fileSystem();
        std::string versigPath = Common::FileSystem::join( fs->currentWorkingDirectory(),"installer/bin/versig");
        nlohmann::json j;
        //Dummy values for now
        j["credential"]["username"] = "jwt[MCS::TENANT_ID]";
        j["credential"]["password"] = "jwt[MCS::TENANT_ID]";

        if (!rootConfigOptions.config[MCS::MCS_CONNECTED_PROXY].empty())
        {
            logDebug("Proxy used for registration was: " + rootConfigOptions.config[MCS::MCS_CONNECTED_PROXY]);
            j["proxy"]["url"] = rootConfigOptions.config[MCS::MCS_CONNECTED_PROXY];
            j["proxy"]["credential"]["username"] = rootConfigOptions.config[MCS::MCS_PROXY_USERNAME];
            j["proxy"]["credential"]["password"] = rootConfigOptions.config[MCS::MCS_PROXY_PASSWORD];
        }

        j["versigPath"] = versigPath;
        j["useSDDS3"] = true;

        j["primarySubscription"]["rigidName"] = "ServerProtectionLinux-Base";
        j["primarySubscription"]["tag"] = "RECOMMENDED";
        j["features"] = {"CORE"};
        j["tenantId"] = jwt[MCS::TENANT_ID];
        j["deviceId"] = jwt[MCS::DEVICE_ID];
        j["JWToken"] = jwt[MCS::JWT_TOKEN];

        if (!update_caches.empty())
        {
            j["updateCache"] = {};
            std::string updateCachePath = Common::FileSystem::join( fs->currentWorkingDirectory(),"installer/uc_certs.crt");
            j["updateCacheCertPath"] = updateCachePath;
            for (const auto& cache: update_caches)
            {
                j["updateCache"].emplace_back(cache.getAddress());
            }

        }
        fs->writeFile("./update_config.json",j.dump());
    }
    return 0;
}
