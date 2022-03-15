/***********************************************************************************************

Copyright 2022-2022 Sophos Limited. All rights reserved.

***********************************************************************************************/

#include <iostream>
#include <string>
#include <set>
#include <cstdlib>

#include <unistd.h>

#include <json.hpp>

#include <sophlib/logging/Logging.h>
#include <SyncLogic.h>

#include <Common/UtilityImpl/StringUtils.h>


static const std::string DEFAULT_SERVICE_URL = "https://sus.sophosupd.com";
static const std::string DEFAULT_UPDATE_URL = "https://sdds3.sophosupd.com";
static const int DEFAULT_TIMEOUT_MS = 5000;


struct CloudSubscription
{
    std::string rigidName;
    std::string releaseTag;
};

struct SUSRequestParameters
{
    std::string product;
    bool isServer = false;
    std::string platformToken;
    std::vector<CloudSubscription> subscriptions;
};


static std::string writeSUSRequest(const SUSRequestParameters& parameters)
{
    nlohmann::json json;
    json["schema_version"] = 1;
    json["product"] = parameters.product;
    json["server"] = parameters.isServer;
    json["platform_token"] = parameters.platformToken;
    json["subscriptions"] = nlohmann::json::array();
    for (const auto& s : parameters.subscriptions)
    {
        json["subscriptions"].push_back({ { "id", s.rigidName }, { "tag", s.releaseTag } });
    }

    return json.dump();
}

static void parseSUSResponse(
    const std::string& response,
    std::set<std::string>& suites,
    std::set<std::string>& releaseGroups)
{
    auto json = nlohmann::json::parse(response);

    if (json.contains("suites"))
    {
        for (const auto& s : json["suites"].items())
        {
            suites.insert(std::string(s.value()));
        }
    }

    if (json.contains("release-groups"))
    {
        for (const auto& g : json["release-groups"].items())
        {
            releaseGroups.insert(std::string(g.value()));
        }
    }
}

static std::string getUserAgent(const std::string& tenant_id, const std::string& device_id)
{
    return "SophosUpdate/SDDS/3.0 (t=\"" + tenant_id + "\" d=\"" + device_id + "\" os=\"Linux\")";
}

static void printUsageAndExit(const std::string name)
{
    std::cout << "usage: " << name << " [-c <certificates directory> -r <repo directory> -u <source URL> -v] <MCS config>" << std::endl;
    exit(EXIT_FAILURE);
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        printUsageAndExit(argv[0]);
    }

    std::string certs_dir = ".";
    std::string repo_dir = ".";
    std::string src_url = DEFAULT_UPDATE_URL;
    bool verbose = false;
    int opt = 0;

    while ((opt = getopt(argc, argv, "c:r:u:v")) != -1)
    {
        switch (opt)
        {
            case 'c':
                certs_dir = optarg;
                break;
            case 'r':
                repo_dir = optarg;
                break;
            case 'u':
                src_url = optarg;
                break;
            case 'v':
                verbose = true;
                break;
            default:
                printUsageAndExit(argv[0]);
        }
    }

    if (argc <= optind)
    {
        printUsageAndExit(argv[0]);
    }

    std::string config_filename = argv[optind];

    sophlib::logging::LogLevel log_level = verbose ? sophlib::logging::LogLevel::Debug : sophlib::logging::LogLevel::Info;
    sophlib::logging::g_logger.SetupConsole(log_level);

    std::string jwt_token;
    std::string tenant_id;
    std::string device_id;
    try
    {
        jwt_token = Common::UtilityImpl::StringUtils::extractValueFromConfigFile(config_filename, "jwt_token");
        tenant_id = Common::UtilityImpl::StringUtils::extractValueFromConfigFile(config_filename, "tenant_id");
        device_id = Common::UtilityImpl::StringUtils::extractValueFromConfigFile(config_filename, "device_id");
    }
    catch (const std::exception& ex)
    {
        std::cerr << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

    if (jwt_token.empty() || tenant_id.empty() || device_id.empty())
    {
        std::cerr << "invalid MCS configuration" << std::endl;
        return EXIT_FAILURE;
    }


    std::set<std::string> suites;
    std::set<std::string> releaseGroups;

    try
    {
        SUSRequestParameters request_parameters;
        request_parameters.product = "linuxep";
        request_parameters.platformToken = "LINUX_INTEL_LIBC6";
        request_parameters.subscriptions = { { "ServerProtectionLinux-Base", "RECOMMENDED" } };
        std::string request_json = writeSUSRequest(request_parameters);
        std::cout << request_json << std::endl;

        // start of SUS
        auto http_session = std::make_unique<utilities::LinuxHttpClient::Session>(getUserAgent(tenant_id, device_id), utilities::LinuxHttpClient::Proxy(), true);
        http_session->SetTimeouts(DEFAULT_TIMEOUT_MS, DEFAULT_TIMEOUT_MS, DEFAULT_TIMEOUT_MS, DEFAULT_TIMEOUT_MS);

        std::string url = DEFAULT_SERVICE_URL + "/v3/" + tenant_id + "/" + device_id;
        auto http_connection = std::make_unique<utilities::LinuxHttpClient::Connection>(*http_session, url);

        auto request = std::make_unique<utilities::LinuxHttpClient::Request>(*http_connection, "POST", "");
        request->AddRequestHeader_Authorization("Bearer", jwt_token);
        request->AddRequestHeader_ContentType("application/json");
        request->SetRequestPayload(request_json);
        unsigned int status_code = request->Send();
        if (status_code != 200)
        {
            std::cerr << "received HTTP response code " << status_code << std::endl;
            return EXIT_FAILURE;
        }

        parseSUSResponse(request->Read(), suites, releaseGroups);
        // end of SUS
    }
    catch (const std::exception& ex)
    {
        std::cerr << ex.what() << std::endl;
        return EXIT_FAILURE;
    }



    std::cout << "suites:";
    for (const auto& s : suites)
    {
        std::cout << " " << s;
    }
    std::cout << std::endl;
    std::cout << "release groups:";
    for (const auto& g : releaseGroups)
    {
        std::cout << " " << g;
    }
    std::cout << std::endl;

    // CDN sync....

    sdds3::Session session(certs_dir);
    sdds3::Repo repo(repo_dir);
    sdds3::Config config;

    session.httpConfig.userAgent = getUserAgent(tenant_id, device_id);
    config.suites = suites;
    config.release_groups_filter = releaseGroups;

    try
    {
        sdds3::sync(session, repo, src_url, config);
    }
    catch (const std::exception& ex)
    {
        std::cerr << ex.what() << std::endl;
        return EXIT_FAILURE;
    }
}
