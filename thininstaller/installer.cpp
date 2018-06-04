#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <algorithm>
#include "SUL.h"
#include "curl.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>


//test commit...

static SU_PHandle g_Product = 0;
static bool g_DebugMode = false;

#ifdef LINUX32
static const char * g_Guid = "084077D4-01CA-4FBC-93A0-54FBB35B0D27";
#else
static const char * g_Guid = "8F2D7B2E-3715-4964-9428-DBDBB69C4800";
#endif

#define BIGBUF 81920

static char g_buf[BIGBUF];
static int g_bufptr = 0;
static std::string g_mcs_url = "";
static std::string g_sul_credentials = "";

class ServerAddress
{
    private:
        std::string address;
        int priority;
    public:
        std::string getAddress() const
        {
            return address;
        }

        int getPriority() const
        {
            return priority;
        }

    ServerAddress (std::string address, int priority):
        address(address),
        priority(priority)
        {}
};

static size_t function_pt(void *ptr, size_t size, size_t nmemb, void *stream)
{
        char * dptr = (char *)ptr;
        int siz = size*nmemb;
        if (g_bufptr+siz >= BIGBUF)
        {
             fprintf(stderr, "ERROR: Response from CURL is larger than buffer\n");
             g_buf[g_bufptr] = 0;
             return 0;
        }

        memcpy(&g_buf[g_bufptr], dptr, siz);
        g_bufptr += siz;
        g_buf[g_bufptr] = 0;
        return size*nmemb;
}

static bool canConnectToCloud(const std::string& proxy = "")
{
    if (proxy == "")
    {
        printf("Checking we can connect to Sophos Central (at %s)...\n", g_mcs_url.c_str());
    }
    else
    {
        printf("Checking we can connect to Sophos Central (at %s via %s)...\n", g_mcs_url.c_str(), proxy.c_str());
    }

    CURL *curl;
    CURLcode res;
    bool ret = false;

    g_buf[0] = 0;

    curl_global_init(CURL_GLOBAL_DEFAULT);

    curl = curl_easy_init();
    if(curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, g_mcs_url.c_str());
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, function_pt);

        curl_easy_setopt(curl, CURLOPT_PROXYAUTH, CURLAUTH_ANY);
        char* proxycreds = getenv("PROXY_CREDENTIALS");
        if (proxycreds != NULL)
        {
            curl_easy_setopt(curl, CURLOPT_PROXYUSERPWD, proxycreds);
        }

        if (proxy != "")
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
        std::string proxyAddress = proxy.getAddress();
        if (canConnectToCloud(proxyAddress))
        {
            return true;
        }
    }
    return canConnectToCloud();
}

static void query_a_thing(SU_PHandle product, const char * the_thing)
{
    SU_String thing = SU_queryProductMetadata(product, the_thing, 0);
    printf("%s: [%s]\n", the_thing, thing ? thing : "<null>");
}

static void writeSignedFile(const char* cred)
{
    std::string path="./warehouse/catalogue/signed-";
    path += cred;
    int fd = ::open(path.c_str(),O_CREAT|O_WRONLY|O_TRUNC,0600);
    assert (fd >= 0);
    int ret = ::write(fd,"true",4);
    assert (ret > 0);
    ret = ::close(fd);
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

static std::vector<std::string> splitString(std::string string_to_split, std::string delim)
{
    std::vector<std::string> split_strings = std::vector<std::string>();

    while (string_to_split != "")
    {
        int delim_pos = string_to_split.find(delim);
        if (delim_pos != std::string::npos)
        {
            std::string stripped_section = string_to_split.substr(0, delim_pos);
            string_to_split = string_to_split.substr(delim_pos+1);
            split_strings.push_back(stripped_section);
        }
        else
        {
            split_strings.push_back(string_to_split);
            break;
        }
    }

    return split_strings;
}

// The argument deliminated_addresses should be in the format hostname1:port1,priority1,id1;hostname2:port2,priority2,id2...
static std::vector<ServerAddress> extractPrioritisedAddresses(const std::string deliminated_addresses)
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
        int priority = 1;
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

static int downloadMediumInstaller(std::string location, bool https, bool updateCache)
{
    SU_init();
    SU_Result ret;
    SU_Handle session = SU_beginSession();
    if (session == NULL)
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
        char * certsToUse = (char *)"installer/sdds_https_rootca.crt";
        if (updateCache)
        {
            certsToUse = (char *)"installer/uc_certs.crt";
        }
        char * ssl_certs = getenv("OVERRIDE_SSL_SOPHOS_CERTS");
        ssl_certs = ssl_certs ? ssl_certs : certsToUse;

        ret = SU_setSslCertificatePath(session, ssl_certs);
        RETURN_IF_ERROR("setSslCertificatePath", ret);

        ret = SU_setUseSophosCertStore(session, true);
        RETURN_IF_ERROR("setUseSophosCertStore", ret);
    }

    char * certs_dir = getenv("OVERRIDE_SOPHOS_CERTS");
    certs_dir = certs_dir ? certs_dir : (char *)"installer";

    char * creds = getenv("OVERRIDE_SOPHOS_CREDS");
    creds = creds ? creds : (char *)g_sul_credentials.c_str();

    // write signed file
    writeSignedFile(creds);

    if (g_DebugMode)
    {
        printf("Creds is [%s]\n", creds);
    }

    ret = SU_addUpdateSource(session, "SOPHOS", creds, creds, 0,0,0);
    RETURN_IF_ERROR("addUpdateSource", ret);

    ret = SU_setCertificatePath(session, certs_dir);
    RETURN_IF_ERROR("setCertificatePath", ret);

    if (g_DebugMode)
    {
        fprintf(stderr, "Listing warehouse with creds [%s] at [%s] with certs dir [%s] via [%s]\n",
                creds, location.c_str(), certs_dir, https ? "HTTPS" : "HTTP");
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
                query_a_thing(product,"Line");
                query_a_thing(product,"Major");
                query_a_thing(product,"Minor");
                query_a_thing(product,"Name");
                query_a_thing(product,"VersionId");
                query_a_thing(product,"DefaultHomeFolder");
                query_a_thing(product,"Platforms");
                query_a_thing(product,"ReleaseTagsTag");
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


static int downloadMediumInstallerDirectOrCaches(const std::vector<ServerAddress>& caches)
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
            ret = downloadMediumInstaller(cache.getAddress(), true, true);
            if (ret == 0)
            {
                return ret;
            }
        }
    }

    // Go direct, preferring https
    ret = downloadMediumInstaller(sophosLocation, true, false);
    if (ret == 0)
    {
        return ret;
    }
    return downloadMediumInstaller(sophosLocation, false, false);
}

int main(int argc, char ** argv)
{
    g_DebugMode = getenv("DEBUG_THIN_INSTALLER");

    std::string arg_filename=argv[1];
    if (argc < 2) {
        fprintf(stderr, "Expecting a filename as an argument but none supplied\n");
        return 40;
    } else {
        if (g_DebugMode)
        {
            printf("Filename = [%s]\n",arg_filename.c_str());
        }
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
        if (fgets(buf, 4096, f) == NULL)
        {
            break;
        }

        std::string val = buf;
        int equals_pos = val.find("=");
        if (equals_pos != std::string::npos)
        {
            std::string argname = val.substr(0, equals_pos);
            std::string argvalue = val.substr(equals_pos+1, std::string::npos);
            int newline_pos = argvalue.find("\n");
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

    if (g_mcs_url == "")
    {
            fprintf(stderr, "Didn't find mcs url in credentials file\n");
            return 42;
    }

    if (g_sul_credentials == "")
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

    return downloadMediumInstallerDirectOrCaches(update_caches);
}
