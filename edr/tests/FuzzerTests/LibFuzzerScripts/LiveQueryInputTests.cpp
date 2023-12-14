// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "edr/modules/livequery/IQueryProcessor.h"
#include "edr/modules/osqueryclient/IOsqueryClient.h"
#include "edr/modules/osqueryclient/OsqueryProcessor.h"
#include "edr/tests/FuzzerTests/LibFuzzerScripts/livequery.pb.h"
#include "edr/tests/FuzzerTests/LibFuzzerScripts/livequeryinput.pb.h"

#include "Common/FileSystem/IFileSystem.h"
#include "Common/Logging/ConsoleLoggingSetup.h"
#include "Common/Logging/LoggerConfig.h"

#include "common/fuzzer/FuzzerUtils.h"
#include "common/livequery/include/OsquerySDK/OsquerySDK.h"

#include "src/libfuzzer/libfuzzer_macro.h"
#include "src/mutator.h"

#include <nlohmann/json.hpp>
#include <google/protobuf/text_format.h>

#include <future>
#include <iostream>
#include <sstream>
#include <string>


/* This replaces the osquery sdk and always return the same output
 * The substitution is done via the factory replacement.
 * */
class FakeIOsqueryClient : public osqueryclient::IOsqueryClient
{
public:
    void connect(const std::string&) override
    {
    }
    OsquerySDK::Status query(const std::string&, OsquerySDK::QueryData&) override
    {
        return OsquerySDK::Status{0, ""};
    }

    OsquerySDK::Status getQueryColumns(const std::string&, [[maybe_unused]] OsquerySDK::QueryColumns& qc) override
    {
        return OsquerySDK::Status{0, ""};
    }
};

namespace livequery
{
    /* This response dispatcher has two main functions. It is here to prevent the
     * ResponseDispatcher from writing the result to the file as it does not help the
     * fuzzer in anyway or form.
     * It is also here to allow the main test to verify that the report was called and the
     * result produced.
     */
    class DummyDispatcher : public livequery::ResponseDispatcher
    {
        std::string m_result;
        livequery::IResponseDispatcher::QueryResponseStatus m_responseStatus;
        Common::Logging::ConsoleLoggingSetup m_consoleSetup{Common::Logging::LOGOFFFORTEST()};

    public:
        DummyDispatcher()
        {
        }
        void sendResponse(const std::string& /*correlationId*/, const QueryResponse& response) override
        {
            m_result = serializeToJson(response);
        }

        std::unique_ptr<IResponseDispatcher> clone() override
        {
            return std::unique_ptr<IResponseDispatcher> { new DummyDispatcher() };
        }

        void feedbackResponseStatus(QueryResponseStatus queryResponseStatus) override
        {
            m_responseStatus = queryResponseStatus;
        }
        QueryResponseStatus responseStatus() const
        {
            return m_responseStatus;
        }
        std::string serializedResult() const
        {
            return m_result;
        }
    };

} // namespace livequery

#ifdef USING_LIBFUZZER
DEFINE_PROTO_FUZZER(const LiveQueryInputProto::TestCase& itestCase)
{
#else
void mainTest(const LiveQueryInputProto::TestCase& itestCase)
{
#endif

    std::string iquery = itestCase.query();
    std::string iname = itestCase.name();

    /*std::cout << "iquery " << iquery << std::endl;
    std::cout << "iname " << iname << std::endl; */
    std::stringstream buildjson;
    buildjson << R"sophos({
    "type": "sophos.mgt.action.RunLiveQuery",
    "name": ")sophos"
              << iname << R"sophos(",
    "query": ")sophos"
              << iquery << R"sophos("
})sophos";
    std::string serializedQuery = buildjson.str();
    /*    std::cout << serializedQuery << std::endl; */

    osqueryclient::OsqueryProcessor queryP { "/tmp/notused" };
    livequery::DummyDispatcher queryD;

    osqueryclient::factory().replace(
        []() { return std::unique_ptr<osqueryclient::IOsqueryClient>(new FakeIOsqueryClient {}); });

    livequery::processQuery(queryP, queryD, serializedQuery, "1234");

    switch (queryD.responseStatus())
    {
        case livequery::IResponseDispatcher::QueryResponseStatus::UnexpectedExceptionOnHandlingQuery:
            std::cerr << "Input data triggered exception. This is a fatal error" << std::endl;
            std::terminate();
            break;
        case livequery::IResponseDispatcher::QueryResponseStatus::QueryFailedValidation:
        case livequery::IResponseDispatcher::QueryResponseStatus::QueryInvalidJson:
            break;
        case livequery::IResponseDispatcher::QueryResponseStatus::QueryResponseProduced:
        default:
        {
            std::string expectSuccessContains = R"sophos("errorCode":0,"errorMessage":"OK","sizeBytes":0})sophos";
            std::string result = queryD.serializedResult();
            if (result.find(expectSuccessContains) == std::string::npos)
            {
                std::cerr << "Result not expected" << result << std::endl;
                std::terminate();
            }
        }
        break;
    }
}

/**
 * LibFuzzer works only with clang, and developers machine are configured to run gcc.
 * For this reason, the flag USING_LIBFUZZER has been used to enable buiding 2 outputs:
 *   * the real fuzzer tool
 *   * An output that is capable of consuming the same sort of input file that is used by the fuzzer
 *     but can be build and executed inside the developers IDE.
 */
#ifndef USING_LIBFUZZER
int main(int argc, char* argv[])
{
    Common::Logging::ConsoleLoggingSetup consoleLoggingSetup{Common::Logging::LOGOFFFORTEST()};
    LiveQueryInputProto::TestCase testcase;
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
