/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "FuzzerUtils.h"

#include "google/protobuf/text_format.h"
#include <livequery.pb.h>
#ifdef HasLibFuzzer
#    include <libprotobuf-mutator/src/libfuzzer/libfuzzer_macro.h>
#    include <libprotobuf-mutator/src/mutator.h>
#endif
#include <Common/Logging/PluginLoggingSetup.h>
#include <Common/FileSystem/IFileSystem.h>

#include <osquery/flagalias.h>
#include <Common/Process/IProcess.h>
#include <redist/pluginapi/tests/include/Common/Helpers/TempDir.h>
#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/FileSystem/IFilePermissions.h>
#include <modules/pluginimpl/OsqueryConfigurator.h>
#include <modules/pluginimpl/ApplicationPaths.h>
#include <Common/UtilityImpl/StringUtils.h>
#include <thirdparty/nlohmann-json/json.hpp>
#include <livequery/IQueryProcessor.h>
#include <modules/osqueryclient/OsqueryProcessor.h>

namespace osquery{

    FLAG(bool, decorations_top_level, false, "test");
}
namespace livequery
{
    class DummyDispatcher : public livequery::ResponseDispatcher
    {
    public:
        void sendResponse(const std::string& /*correlationId*/, const QueryResponse& response) override
        {
            auto content = serializeToJson(response);
            nlohmann::json jsoncontent = nlohmann::json::parse(content);
            // uncomment to see the response from osquery
            //std::cout << jsoncontent.dump(2) << std::endl;
        }
    };

} // namespace livequery
class OsqueryRunnerSingleton{
public:
    OsqueryRunnerSingleton(): m_tempDir("/tmp")
    {
        Common::ApplicationConfiguration::applicationConfiguration().setData(
                Common::ApplicationConfiguration::SOPHOS_INSTALL, m_tempDir.dirPath()
                );
        std::vector<std::string> relativePaths;
        for( std::string dirname : std::vector<std::string>{{"etc"},{"log"},{"var"}}) {
            relativePaths.push_back("plugins/edr/" + dirname);

        }
        m_tempDir.makeDirs(relativePaths);
        m_tempDir.createFile("base/etc/logger.conf", "[global]\nVERBOSITY=INFO\n");
        m_loggingSetup.reset(new Common::Logging::PluginLoggingSetup("edr"));

        setupForTest();

        m_osqueryProc = Common::Process::createProcess();
        std::vector<std::string> arguments = { "--config_path=" + Plugin::osqueryConfigFilePath(),
                                               "--flagfile=" + Plugin::osqueryFlagsFilePath() };

        // add if you want more information from osquery
        //arguments.push_back("--verbose");

        m_osqueryProc->exec(getOsqueryPath(), arguments, {} );
        std::this_thread::sleep_for(std::chrono::seconds(2));
        m_osqueryProcessor = std::unique_ptr<osqueryclient::OsqueryProcessor>(new osqueryclient::OsqueryProcessor{ Plugin::osquerySocket() });

    }
    ~OsqueryRunnerSingleton()
    {
        m_osqueryProc->kill(5);
        std::cout << m_osqueryProc->output() << std::endl;
        std::string syslogpipe = m_tempDir.absPath("./plugins/edr/var/syslog_pipe");
        if( unlink(syslogpipe.c_str())== -1 ) {
            int err = errno;

            std::cerr << "Failed to unlink: " << strerror(err) << std::endl;
        }
        m_loggingSetup.reset();

        // add if you want to see the full log file.
        //std::cout << Common::FileSystem::fileSystem()->readFile( m_tempDir.absPath("plugins/edr/log/edr.log") ) << std::endl;

    }
    std::string getRedistOsqueryPath()
    {
        const char * redist = getenv("REDIST");
        if (redist == nullptr)
        {
            throw std::runtime_error("No REDIST environment variable defined");
        }
        return Common::FileSystem::join(redist,"osquery/usr/bin/osqueryd") ;
    }
    std::string getOsqueryPath()
    {
        return m_tempDir.absPath("fuzz/bin/osqueryd");
    }
    livequery::IQueryProcessor & queryProcessor() {
        return *m_osqueryProcessor;
    }


private:
    void setupForTest()
    {
        m_tempDir.makeDirs("fuzz/bin");

        std::string sourcePath = getRedistOsqueryPath();
        std::string destPath = getOsqueryPath();
        Common::FileSystem::fileSystem()->copyFile(sourcePath,destPath);
        Common::FileSystem::filePermissions()->chmod(destPath, 0700);

        Plugin::OsqueryConfigurator::regenerateOSQueryFlagsFile(Plugin::osqueryFlagsFilePath(), true);
        Plugin::OsqueryConfigurator::regenerateOsqueryConfigFile(Plugin::osqueryConfigFilePath());

        std::string fileContents = Common::FileSystem::fileSystem()->readFile( Plugin::osqueryFlagsFilePath());
        std::string updatedFileContents = Common::UtilityImpl::StringUtils::replaceAll
                (fileContents,"--disable_watchdog=false","--disable_watchdog=true");
        Common::FileSystem::fileSystem()->writeFile(Plugin::osqueryFlagsFilePath(),updatedFileContents);

    }

    Tests::TempDir m_tempDir;
    std::unique_ptr<Common::Logging::PluginLoggingSetup> m_loggingSetup;
    std::unique_ptr<osqueryclient::OsqueryProcessor> m_osqueryProcessor;
    Common::Process::IProcessPtr m_osqueryProc;
};


#ifdef HasLibFuzzer
DEFINE_PROTO_FUZZER(const LiveQueryProto::TestCase& testCase)
{
#else
void mainTest(const LiveQueryProto::TestCase& testCase)
{
#endif
    static OsqueryRunnerSingleton osqueryRunnerSingleton;

    assert( Common::FileSystem::fileSystem()->isFile(osqueryRunnerSingleton.getOsqueryPath()));
    assert( Common::FileSystem::fileSystem()->isExecutable(osqueryRunnerSingleton.getOsqueryPath()));

    std::string query = testCase.query();

    nlohmann::json queryJson;

    queryJson["type"] = "sophos.mgt.action.RunLiveQuery";
    queryJson["name"] = "";
    queryJson["query"] = "%QUERY%";
    std::string serializedQuery = Common::UtilityImpl::StringUtils::replaceAll(queryJson.dump(), "%QUERY%", query);

    livequery::DummyDispatcher dummyDispatcher;


    livequery::processQuery(osqueryRunnerSingleton.queryProcessor(), dummyDispatcher , serializedQuery, "2322-2323");
}



/**
 * LibFuzzer works only with clang, and developers machine are configured to run gcc.
 * For this reason, the flag HasLibFuzzer has been used to enable buiding 2 outputs:
 *   * the real fuzzer tool
 *   * An output that is capable of consuming the same sort of input file that is used by the fuzzer
 *     but can be build and executed inside the developers IDE.
 */
#ifndef HasLibFuzzer
int main(int argc, char* argv[])
{
    LiveQueryProto::TestCase testcase;
    if (argc < 2)
    {
        std::cerr << " Invalid argument. Usage: " << argv[0] << " <input_file>" << std::endl;
        return 1;
    }
    std::string content = Common::FileSystem::fileSystem()->readFile(argv[1]);
    google::protobuf::TextFormat::Parser parser;
    parser.AllowPartialMessage(true);
    if (!parser.ParseFromString(content, &testcase))
    {
        std::cerr << "Failed to parse file. Content: " << content << std::endl;
        return 3;
    }

    mainTest(testcase);
    return 0;
}

#endif
