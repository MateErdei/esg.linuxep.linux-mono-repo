#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <utility>
#include <vector>
#include <algorithm>
#include <fcntl.h>
#include <unistd.h>
#include <cassert>

#include "SUL.h"

#include <curl/curl.h> // Because #include "curl.h" does not work.

static SU_PHandle g_Product = nullptr;
static bool g_DebugMode = false;

// TODO Add GUID for base warehouse files
static const char * g_Guid = "SSPL-RIGIDNAME";

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
        std::string getAddress() const
        {
            return m_address;
        }

        int getPriority() const
        {
            return m_priority;
        }

    ServerAddress (std::string address, int priority);
};

ServerAddress::ServerAddress(std::string address, int priority) :
        m_address(std::move(address)),
        m_priority(priority)
{}

static size_t function_pt(char *ptr, size_t size, size_t nmemb, void *stream)
{
    size_t sizeOfData = size * nmemb;
    if (g_bufptr + sizeOfData >= BIGBUF)
    {
         fprintf(stderr, "ERROR: Response from CURL is larger than buffer\n");
         g_buf[g_bufptr] = 0;
         return 0;
    }
    memcpy(&g_buf[g_bufptr], ptr, sizeOfData);
    g_bufptr += sizeOfData;
    g_buf[g_bufptr] = 0;
    return sizeOfData;
}

static bool canConnectToCloud(const std::string& proxy = "")
{
    if (proxy.empty())
    {
        printf("Checking we can connect to Sophos Central (at %s)...\n", g_mcs_url.c_str());
    }
    else
    {
        printf("Checking we can connect to Sophos Central (at %s via %s)...\n", g_mcs_url.c_str(), proxy.c_str());
    }

    CURLcode res;
    bool ret = false;
    g_buf[0] = 0;

    curl_global_init(CURL_GLOBAL_DEFAULT);

    CURL *curl = curl_easy_init();
    if(curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, g_mcs_url.c_str());
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, function_pt);

        curl_easy_setopt(curl, CURLOPT_PROXYAUTH, CURLAUTH_ANY);
        char* proxycreds = getenv("PROXY_CREDENTIALS");
        if (proxycreds != nullptr)
        {
            curl_easy_setopt(curl, CURLOPT_PROXYUSERPWD, proxycreds);
        }

        if (!proxy.empty())
        {
            curl_easy_setopt(curl, CURLOPT_PROXY, proxy.c_str());
        }

        /* Perform the request, res will get the return code */
        res = curl_easy_perform(curl);

        /* Check for errors */
        if(res != CURLE_OK) {
            fprintf(stderr, "Failed to connect to Sophos Central at %s (cURL error is [%s]). Please check your firewall rules.\n",
                g_mcs_url.c_str(), curl_easy_strerror(res));
        } else {
            if (g_DebugMode)
            {
                printf("Successfully got [%s] from Sophos Central\n", curl_easy_strerror(res));
            }
            ret = true;
        }

        /* always cleanup */
        curl_easy_cleanup(curl);
    }

  curl_global_cleanup();

  return ret;
}

static bool canConnectToCloudDirectOrProxies(const std::vector<ServerAddress>& proxies)
{
    for (auto & proxy : proxies)
    {
        if (canConnectToCloud(proxy.getAddress()))
        {
            return true;
        }
    }
    return canConnectToCloud();
}

static void queryProductMetadata(SU_PHandle product, const char *the_thing)
{
    SU_String thing = SU_queryProductMetadata(product, the_thing, 0);
    printf("%s: [%s]\n", the_thing, thing ? thing : "<null>");
}

static void writeSignedFile(const char* cred)
{
    std::string path="./warehouse/catalogue/signed-";
    path += cred;
    int fd = ::open(path.c_str(), O_CREAT|O_WRONLY|O_TRUNC, 0600);
    assert (fd >= 0);

    ssize_t retSize = ::write(fd, "true", 4);
    assert (retSize > 0);

    int ret = ::close(fd);
    assert (ret == 0);
}

static void debugLog(const char * what, int ret)
{
    if (g_DebugMode)
    {
        printf("%s: %d\n", what, ret);
    }
}

static bool isSULError(SU_Result ret)
{
    return ret != SU_Result_OK && ret != SU_Result_nullSuccess;
}

static bool errorLog(const char * what, SU_Result ret, SU_Handle session)
{
    if (isSULError(ret))
    {
        fprintf(stderr, "FAILURE: %s: %s (%d)\n", what, SU_getErrorDetails(session), ret);
        return true;
    }
    else
    {
        debugLog(what, ret);
    }
    return false;
}

#define RETURN_IF_ERROR(what, sulerror) \
    if (errorLog(what, sulerror, session)) \
    { \
        return sulerror; \
    }

static std::vector<std::string> splitString(std::string string_to_split, const std::string &delim)
{
    std::vector<std::string> splitStrings = std::vector<std::string>();

    while (!string_to_split.empty())
    {
        std::size_t delimPos = string_to_split.find(delim);
        if (delimPos != std::string::npos)
        {
            std::string stripped_section = string_to_split.substr(0, delimPos);
            string_to_split = string_to_split.substr(delimPos+1);
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

// The argument deliminated_addresses should be in the format hostname1:port1,priority1,id1;hostname2:port2,priority2,id2...
static std::vector<ServerAddress> extractPrioritisedAddresses(const std::string &deliminated_addresses)
{
    std::vector<ServerAddress> proxies;
    std::vector<std::string> proxyStrings = splitString(deliminated_addresses, ";");
    for (auto & proxyString : proxyStrings)
    {
        std::vector<std::string> addressAndPriority = splitString(proxyString, ",");
        if (addressAndPriority.size() < 2)
        {
            fprintf(stderr, "Malformed proxy,priority,id: %s\n", proxyString.c_str());
            continue;
        }

        // Note, we ignore the Message Relay ID
        std::string address = addressAndPriority[0];
        std::string priorityString = addressAndPriority[1];
        int priority;
        try
        {
            priority = std::stoi(priorityString);
        }
        catch (...)
        {
            fprintf(stderr, "Malformed priority (%s) for proxy defaulting to 1\n", priorityString.c_str());
            continue;
        }
        ServerAddress proxy = ServerAddress(address, priority);
        proxies.push_back(proxy);
    }
    std::stable_sort(proxies.begin(), proxies.end(), [] (ServerAddress p1, ServerAddress p2)
    {
        return p1.getPriority() < p2.getPriority();
    });
    return proxies;
}

static int downloadInstaller(std::string location, bool https, bool updateCache)
{
    //Todo tidy this up to make it only https
    SU_init();
    SU_Result ret;
    SU_Handle session = SU_beginSession();

    if (session == nullptr)
    {
        fprintf(stderr, "Failed to init SUL session\n");
        return 45;
    }

    if (updateCache)
    {
        std::string redirectToLocation = location + "/sophos/warehouse";
        const char * redirectFromAddresses[] = {
            "d1.sophosupd.com/update",
            "d1.sophosupd.net/update",
            "d2.sophosupd.com/update",
            "d2.sophosupd.net/update",
            "d3.sophosupd.com/update",
            "d3.sophosupd.net/update",
        };
        for (auto & redirectFromAddress : redirectFromAddresses)
        {
            ret = SU_addRedirect(session, redirectFromAddress, redirectToLocation.c_str());
            RETURN_IF_ERROR("addredirect", ret);
        }

        location = "https://" + location + "/sophos/customer";
        https = true;
    }

    ret = SU_setLoggingLevel(session, SU_LoggingLevel_verbose);
    RETURN_IF_ERROR("SU_setLoggingLevel", ret)

    ret = SU_addCache(session, "./cache");
    RETURN_IF_ERROR("addCache", ret);

    ret = SU_setLocalRepository(session, "./warehouse");
    RETURN_IF_ERROR("setlocal", ret);

    ret = SU_addSophosLocation(session, location.c_str());
    RETURN_IF_ERROR("addsophoslocation", ret);

    ret = SU_setUseHttps(session, https);
    RETURN_IF_ERROR("setUseHttps", ret);

    if (https)
    {
        auto * certsToUse = const_cast<char *>("installer/sdds_https_rootca.crt");
        if (updateCache)
        {
            certsToUse = const_cast<char *>("installer/uc_certs.crt");
        }
        char * sslCerts = getenv("OVERRIDE_SSL_SOPHOS_CERTS");
        sslCerts = sslCerts ? sslCerts : certsToUse;

        ret = SU_setSslCertificatePath(session, sslCerts);
        RETURN_IF_ERROR("setSslCertificatePath", ret);

        ret = SU_setUseSophosCertStore(session, true);
        RETURN_IF_ERROR("setUseSophosCertStore", ret);
    }

    char * certsDir = getenv("OVERRIDE_SOPHOS_CERTS");
    certsDir = certsDir ? certsDir : (char *)"installer";

    char * creds = getenv("OVERRIDE_SOPHOS_CREDS");
    creds = creds ? creds : const_cast<char *>(g_sul_credentials.c_str());

    // Write signed file
    writeSignedFile(creds);

    if (g_DebugMode)
    {
        printf("Creds is [%s]\n", creds);
    }

    ret = SU_addUpdateSource(session, "SOPHOS", creds, creds, 0,0,0);
    RETURN_IF_ERROR("addUpdateSource", ret);

    ret = SU_setCertificatePath(session, certsDir);
    RETURN_IF_ERROR("setCertificatePath", ret);

    if (g_DebugMode)
    {
        fprintf(stderr, "Listing warehouse with creds [%s] at [%s] with certs dir [%s] via [%s]\n",
                creds, location.c_str(), certsDir, https ? "HTTPS" : "HTTP");
    }

    ret = SU_readRemoteMetadata(session);
    debugLog("readRemoteMetadata", ret);

    if (isSULError(ret))
    {
        fprintf(stderr, "Failed to connect to warehouse at %s (SUL error is [%d-%s]). Please check your firewall rules and proxy configuration.\n", location.c_str(), ret, SU_getErrorDetails(session));
        return 46;
    }

    while (true)
    {
        if (g_DebugMode)
        {
            printf("Getting next product\n");
        }
        SU_PHandle product = SU_getProductRelease(session);
        if (product)
        {
            if (g_DebugMode)
            {
                queryProductMetadata(product,"Line");
                queryProductMetadata(product,"Major");
                queryProductMetadata(product,"Minor");
                queryProductMetadata(product,"Name");
                queryProductMetadata(product,"VersionId");
                queryProductMetadata(product,"DefaultHomeFolder");
                queryProductMetadata(product,"Platforms");
                queryProductMetadata(product,"ReleaseTagsTag");
                printf("\n");
            }
            if (!strcmp(SU_queryProductMetadata(product, "Line", 0), g_Guid))
            {
                g_Product = product;
            }
            else
            {
                ret = SU_removeProduct(product);
                RETURN_IF_ERROR("SU_removeProduct", ret);
            }
        }
        else
        {
            if (g_DebugMode)
            {
                printf("Out of products\n");
            }
            break;
        }
    }

    if (!g_Product)
    {
        fprintf(stderr, "Internal error - medium installer not found\n");
        return 47;
    }

    ret = SU_synchronise(session);
    debugLog("synchronise", ret);
    if (isSULError(ret))
    {
        fprintf(stderr,
            "Failed to synchronise warehouse at %s (SUL error is [%d-%s]). "
            "Please check your firewall rules and proxy configuration.\n",
            location.c_str(),
            ret,
            SU_getErrorDetails(session));
        return 48;
    }

    ret = SU_addDistribution(g_Product, "./distribute/", 0, "", "");
    RETURN_IF_ERROR("addDistribution", ret);
    ret = SU_distribute(session, 0);
    RETURN_IF_ERROR("distribute", ret);
    ret = SU_endSession(session);
    RETURN_IF_ERROR("SU_endSession", ret);

    return 0;
}


static int downloadInstallerDirectOrCaches(const std::vector<ServerAddress>& caches)
{
    int ret = 0;

    // Try override if set
    const char * sophosLocation = getenv("OVERRIDE_SOPHOS_LOCATION");

    if (!sophosLocation)
    {
        sophosLocation = "http://dci.sophosupd.com/update";

        // Use update caches
        for (auto & cache : caches)
        {
            ret = downloadInstaller(cache.getAddress(), true, true);
            if (ret == 0)
            {
                return ret;
            }
        }
    }

    // Go direct, always https
    return downloadInstaller(sophosLocation, true, false);
}

int main(int argc, char ** argv)
{
    g_DebugMode = static_cast<bool>(getenv("DEBUG_THIN_INSTALLER"));

    if (argc < 2) {
        fprintf(stderr, "Expecting a filename as an argument but none supplied\n");
        return 40;
    }
    std::string arg_filename=argv[1];
    if (g_DebugMode)
    {
        printf("Filename = [%s]\n",arg_filename.c_str());
    }


    FILE * f = fopen(arg_filename.c_str(), "r");
    if (!f)
    {
         fprintf(stderr, "Cannot open credentials file\n");
         return 41;
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
            std::string argvalue = val.substr(equals_pos+1, std::string::npos);
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
            fprintf(stderr, "Didn't find mcs url in credentials file\n");
            return 42;
    }

    if (g_sul_credentials.empty())
    {
            fprintf(stderr, "Didn't find update credentials in credentials file\n");
            return 43;
    }

    char * override_mcs_url = getenv("OVERRIDE_CLOUD_URL");
    if (override_mcs_url)
    {
        g_mcs_url = std::string(override_mcs_url);
    }

    if (!canConnectToCloudDirectOrProxies(relays))
    {
        // Exit 44 means cannot connect to Cloud - must correspond to handling in installer_header.sh
        return 44;
    }

    return downloadInstallerDirectOrCaches(update_caches);
}
