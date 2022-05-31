#include "SUL.h"
#include "curl.h"

#include <CentralRegistration/Main.h>
#include <cmcsrouter/Config.h>
#include <cmcsrouter/ConfigOptions.h>
#include <cmcsrouter/MCSApiCalls.h>

#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/Logging/LoggerConfig.h>

#include <log4cplus/loggingmacros.h>

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unistd.h>
#include <utility>
#include <vector>



static SU_PHandle g_Product = nullptr;
static bool g_DebugMode = false;

static const std::vector<std::string> g_GuidPreferences{"ServerProtectionLinux-Base-component", "ServerProtectionLinux-Base"};
static const char* g_ReleaseTag = "RECOMMENDED";

// Used to check if an env proxy is in use.
const char* g_httpsProxy = getenv("https_proxy");

#define BIGBUF 81920

static char g_buf[BIGBUF];
static int g_bufptr = 0;
static std::string g_mcs_url;
static std::string g_sul_credentials;

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

std::string strFromSul(SU_ConstString sulString, SU_Handle session)
{
    if ( sulString == nullptr)
    {
        return std::string{}; 
    }
    std::string copyStr{sulString}; 
    SU_freeString(session, sulString ); 
    return copyStr; 
}

std::string getProductMetadata(SU_PHandle product, const char * tag, SU_Int index)
{
    return strFromSul(SU_queryProductMetadata(product, tag, index ), SU_getSession(product)); 
}

/// Print metadata for a product using the SUL library, for debug purposes.
/// \param product SUL object which acts as a handle to a products warehouse info.
/// \param attribute The attribute of the product metadata that you want printed.
static void queryProductMetadata(SU_PHandle product, const char* attribute)
{
    static std::string nullString{"<null>"}; 
    std::string result = getProductMetadata(product, attribute, 0);     
    printf("%s: [%s]\n", attribute, !result.empty() ? result.c_str() : nullString.c_str());
}

static void writeSignedFile(const char* cred)
{
    std::string path = "./warehouse/catalogue/signed-";
    path += cred;
    int fd = ::open(path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0600); // NOLINT
    assert(fd >= 0);

    ssize_t retSize = ::write(fd, "true", 4);
    assert(retSize > 0);

    int ret = ::close(fd);
    assert(ret == 0);
}

static bool isSULError(SU_Result ret)
{
    return ret != SU_Result_OK && ret != SU_Result_nullSuccess;
}

/// Used to log SUL lib errors by the macro RETURN_IF_ERROR, logs debug if success.
/// \param what The error message
/// \param ret The return value of the SUL lib call
/// \param session SUL session handle used to get further error details
/// \return Returns true if the call to the SUL lib returns an error, false if the call was successful.
static bool logSulError(const char* what, SU_Result ret, SU_Handle session)
{
    if (isSULError(ret))
    {
        std::string error = strFromSul(SU_getErrorDetails(session), session); 
        fprintf(stderr, "FAILURE: %s: %s (%d)\n", what, error.c_str(), ret);
        return true;
    }
    else
    {
        logDebug(std::string(what) + ": " + std::to_string(ret));
    }
    return false;
}

#define RETURN_IF_ERROR(what, sulerror)                                                                            \
if (logSulError(what, sulerror, session->m_session))                                                               \
{                                                                                                                  \
    return sulerror;                                                                                               \
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

class SULSession
{
public:
    SULSession()
    {
        m_session = SU_beginSession();
        if (m_session == nullptr)
        {
            throw std::runtime_error("Failed to start SUL session");
        }
    }
    SULSession(const SULSession&) = delete;
    SULSession& operator=(const SULSession&) = delete;

    ~SULSession()
    {
        if (m_session != nullptr)
        {
            SU_endSession(m_session);
        }
    }

    SU_Handle m_session = nullptr;
};

class SULInit
{
public:
    SULInit(const SULInit&) = delete;
    SULInit& operator=(const SULInit&) = delete;

    SULInit() { SU_init(); }

    ~SULInit() { SU_deinit(); }
};

struct ProductInfo
{
    SU_PHandle pHandle; 
    bool hasTargetReleaseTag; 
    std::string rigidName;
    std::vector<std::string> releaseTags;
    static ProductInfo from(SU_PHandle product)
    {
        ProductInfo productInfo;
        if (g_DebugMode)
        {
            queryProductMetadata(product, "Line");
            queryProductMetadata(product, "Major");
            queryProductMetadata(product, "Minor");
            queryProductMetadata(product, "Name");
            queryProductMetadata(product, "VersionId");
            queryProductMetadata(product, "DefaultHomeFolder");
            queryProductMetadata(product, "Platforms");
            queryProductMetadata(product, "ReleaseTagsTag");
            printf("\n");
        }

        // First get the list of available Release Tags
        int index = 0;
        while (true)
        {
            std::string tag = getProductMetadata(product, "ReleaseTagsTag", index);
            if (tag.empty())
            {
                break;
            }
            productInfo.releaseTags.push_back(tag);
            index++;
        }
        // Now that we have the available release tags check to see if one of them matches the target release tag.
        productInfo.hasTargetReleaseTag = false;
        for(auto& tag : productInfo.releaseTags)
        {
            if(tag == g_ReleaseTag)
            {
                productInfo.hasTargetReleaseTag = true;
                break;
            }
        }

        productInfo.rigidName =  getProductMetadata(product, "Line", 0); 
        productInfo.pHandle = product; 
        return productInfo;
    }
};

struct MatchProduct
{
    std::string m_rigidLine; 
    MatchProduct( std::string rigidLine): m_rigidLine(std::move(rigidLine)){}
    bool operator()(const ProductInfo& pInfo) const{
        return pInfo.rigidName == m_rigidLine && pInfo.hasTargetReleaseTag; 
    }
};

std::vector<ProductInfo> listProductsFromWarehouse(SU_Handle session){
    std::vector<ProductInfo> products; 
    while (true)
    {
        logDebug("Getting next product");
        SU_PHandle product = SU_getProductRelease(session);
        if (product)
        {
            products.emplace_back(ProductInfo::from(product));
        }
        else{
            logDebug("Out of products");
            break;
        }
    }
    return products; 
}

/// Downloads installer from sophos URL
/// \param location Sophos warehouse URL
/// \param updateCache Whether location is an Update Cache.
/// \param disableEnvProxy Whether we should stop SUL using any env proxies.
/// \return Error code or 0 for success.
static int downloadInstaller(std::string location, bool updateCache, bool disableEnvProxy)
{
    SULInit init;
    SU_Result ret;
    std::unique_ptr<SULSession> session;

    try
    {
        session.reset(new SULSession());
    }
    catch(std::runtime_error& ex)
    {
        logDebug(ex.what());
        return 49;
    }

    if (session->m_session == nullptr)
    {
        logError("Failed to init SUL session");
        return 45;
    }

    if (updateCache)
    {
        std::string redirectToLocation = location + "/sophos/warehouse";
        const char* redirectFromAddresses[] = {
            "d1.sophosupd.com/update", "d1.sophosupd.net/update", "d2.sophosupd.com/update",
            "d2.sophosupd.net/update", "d3.sophosupd.com/update", "d3.sophosupd.net/update",
        };
        for (auto& redirectFromAddress : redirectFromAddresses)
        {
            ret = SU_addRedirect(session->m_session, redirectFromAddress, redirectToLocation.c_str());
            RETURN_IF_ERROR("addredirect", ret);
        }

        location = "https://" + location + "/sophos/customer";
    }

    ret = SU_setLoggingLevel(session->m_session, SU_LoggingLevel_verbose);
    RETURN_IF_ERROR("SU_setLoggingLevel", ret)

    ret = SU_addCache(session->m_session, "./cache");
    RETURN_IF_ERROR("addCache", ret);

    ret = SU_setLocalRepository(session->m_session, "./warehouse");
    RETURN_IF_ERROR("setlocal", ret);

    ret = SU_addSophosLocation(session->m_session, location.c_str());
    RETURN_IF_ERROR("addsophoslocation", ret);

    ret = SU_setUseHttps(session->m_session, true);
    RETURN_IF_ERROR("setUseHttps", ret);

    if (updateCache)
    {
        auto certsToUse = const_cast<char*>("installer/uc_certs.crt");
        ret = SU_setSslCertificatePath(session->m_session, certsToUse);
        RETURN_IF_ERROR("setSslCertificatePath", ret);
    }

    ret = SU_setUseSophosCertStore(session->m_session, true);
    RETURN_IF_ERROR("setUseSophosCertStore", ret);

    char* certsDir = getenv("OVERRIDE_SOPHOS_CERTS");
    certsDir = certsDir ? certsDir : (char*)"installer";

    char* creds = getenv("OVERRIDE_SOPHOS_CREDS");
    creds = creds ? creds : const_cast<char*>(g_sul_credentials.c_str());

    // Write signed file
    writeSignedFile(creds);

    logDebug("Creds is [" + std::string(creds) + "]");

    const char* proxy = nullptr;
    if (updateCache || disableEnvProxy)
    {
        proxy = "noproxy:";
    }

    ret = SU_addUpdateSource(session->m_session, "SOPHOS", creds, creds, proxy, nullptr, nullptr);
    RETURN_IF_ERROR("addUpdateSource", ret);

    ret = SU_setCertificatePath(session->m_session, certsDir);
    RETURN_IF_ERROR("setCertificatePath", ret);

    std::stringstream listingWarehouseMessage;
    listingWarehouseMessage << "Listing warehouse with creds [" << creds << "] at [" << location << "] with certs dir [" << certsDir << "] via [HTTPS]";
    if (updateCache)
    {
        listingWarehouseMessage << " using UC";
    }
    if (!disableEnvProxy && g_httpsProxy)
    {
        listingWarehouseMessage << " env proxy [" << g_httpsProxy << "]";
    }
    logDebug(listingWarehouseMessage.str());

    ret = SU_readRemoteMetadata(session->m_session);
    logDebug("readRemoteMetadata: " + std::to_string(ret));

    if (isSULError(ret))
    {
        logDebug(
            "Failed to connect to warehouse at " + location + " (SUL error is [" + std::to_string(ret) + "-" +
            SU_getErrorDetails(session->m_session) + "]). Please check your firewall rules and proxy configuration");

        return 46;
    }

    std::vector<ProductInfo> products = listProductsFromWarehouse(session->m_session); 
    
    std::vector<ProductInfo>::iterator selectedProductIt; 
    for( auto & targetRigidName: g_GuidPreferences)
    {
        selectedProductIt = std::find_if(products.begin(), products.end(), MatchProduct(targetRigidName)); 
        if (selectedProductIt != products.end())
        {
            break; 
        }
    }

    if (selectedProductIt == products.end())
    {
        logError("Internal error - base installer not found");
        return 47;
    }

    // remove all the other products
    for( auto it=products.begin(); it != products.end(); ++it)
    {
        if ( it != selectedProductIt)
        {
                ret = SU_removeProduct(it->pHandle);
                RETURN_IF_ERROR("SU_removeProduct", ret);
        }
    }

    g_Product = selectedProductIt->pHandle; 

    ret = SU_synchronise(session->m_session);
    logDebug("synchronise: " + std::to_string(ret));
    if (isSULError(ret))
    {
        fprintf(
            stderr,
            "Failed to synchronise warehouse at %s (SUL error is [%d-%s]). "
            "Please check your firewall rules and proxy configuration\n",
            location.c_str(),
            ret,
            SU_getErrorDetails(session->m_session));
        return 48;
    }

    ret = SU_addDistribution(g_Product, "./distribute/", 0, "", "");
    RETURN_IF_ERROR("addDistribution", ret);
    ret = SU_distribute(session->m_session, 0);
    RETURN_IF_ERROR("distribute", ret);

    return 0;
}

/// Tries to download the base installer via the following methods:
/// 1) Update Caches 2) Allowing any env proxies 3) Explicitly disabling env proxies - allows fallback if env proxy is bad
/// \param caches Vector of Update Caches which should be tried to download from first.
/// \return Error code or 0 for success.
static int downloadInstallerDirectOrCaches(const std::vector<ServerAddress>& caches)
{
    int ret = 0;
    log("Downloading base installer (this may take some time)");

    // Try override if set
    const char* sophosLocation = getenv("OVERRIDE_SOPHOS_LOCATION");

    // If the override is set then don't try to use UCs
    if (!sophosLocation)
    {
        sophosLocation = "http://dci.sophosupd.com/update";

        // Try using update caches
        for (auto& cache : caches)
        {
            logDebug("Attempting to download installer from update cache address [" + cache.getAddress() + "]");
            ret = downloadInstaller(cache.getAddress(), true, true);
            if (ret == 0)
            {
                logDebug("Successfully downloaded installer from update cache address [" + cache.getAddress() + "]");
                return ret;
            }
        }
    }

    logDebug("Attempting to download installer from Sophos");

    // Download installer directly from sophos, *allowing* env proxies
    ret = downloadInstaller(sophosLocation, false, false);

    if (ret == 0)
    {
        logDebug("Successfully downloaded installer from Sophos");
        return 0;
    }

    // If we failed to download via UCs and directly (possibly via a proxy)
    // then here we check to see if proxy was set, if one is set and the download failed then we try without proxies.
    if (g_httpsProxy)
    {
        // Download installer directly from sophos, *disallowing* env proxies
        // as the previous download using it didn't work, so disable proxies.
        ret = downloadInstaller(sophosLocation, false, true);

        if (ret == 0)
        {
            logDebug("Successfully downloaded installer from Sophos");
            return 0;
        }
    }

    // If we got here then all download attempts have failed.
    log("Failed to download installer");
    return ret;
}

int main(int argc, char** argv)
{
    g_DebugMode = static_cast<bool>(getenv("DEBUG_THIN_INSTALLER"));
    if (g_DebugMode)
    {
        Common::Logging::ConsoleLoggingSetup m_loggingSetup;
    }
    else
    {
        Common::Logging::ConsoleLoggingSetup m_loggingSetup{Common::Logging::LOGOFFFORTEST()};
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
    for(int i = 2; i < argc; i++)
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
            else if (argname == "UPDATE_CREDENTIALS")
            {
                g_sul_credentials = argvalue;
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

    if (g_sul_credentials.empty())
    {
        logError("Didn't find update credentials in credentials file");
        return 43;
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

    MCS::ConfigOptions configOptions =
        CentralRegistration::innerCentralRegistration(registerArgValues);

    if (configOptions.config[MCS::MCS_ID].empty())
    {
        return 51;
    }
    log("Successfully registered with Sophos Central");
    configOptions.writeToDisk("./mcs.config");

    std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper =
        std::make_shared<Common::CurlWrapper::CurlWrapper>();
    std::shared_ptr<Common::HttpRequests::IHttpRequester> client = std::make_shared<Common::HttpRequestsImpl::HttpRequesterImpl>(curlWrapper);
    MCS::MCSHttpClient httpClient(configOptions.config[MCS::MCS_URL], configOptions.config[MCS::MCS_CUSTOMER_TOKEN], std::move(client));

    if (!configOptions.config[MCS::MCS_CA_OVERRIDE].empty())
    {
        httpClient.setCertPath(configOptions.config[MCS::MCS_CA_OVERRIDE]);
    }
    else
    {
        httpClient.setCertPath("./mcs.config");
    }
    httpClient.setID(configOptions.config[MCS::MCS_ID]);
    httpClient.setPassword(configOptions.config[MCS::MCS_PASSWORD]);

    std::string jwt = MCS::MCSApiCalls().getJwt(httpClient);
    if (jwt.empty())
    {
        return 52;
    }
    // TODO LINUXDAR-4273 stop logging this
    log("JWT: ");
    log(jwt);

    return downloadInstallerDirectOrCaches(update_caches);
}
