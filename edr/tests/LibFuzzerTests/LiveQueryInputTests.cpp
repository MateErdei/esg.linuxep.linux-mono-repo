/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "FuzzerUtils.h"
#include <livequery.pb.h>
#include <livequeryinput.pb.h>
#ifdef HasLibFuzzer
#    include <libprotobuf-mutator/src/libfuzzer/libfuzzer_macro.h>
#    include <libprotobuf-mutator/src/mutator.h>
#endif
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/FileSystem/IFileSystem.h>

#include <modules/osqueryclient/OsqueryProcessor.h>
#include <modules/livequery/IQueryProcessor.h>
#include <modules/livequery/ParallelQueryProcessor.h>
#include <modules/osqueryclient/IOsqueryClient.h>

#include <thirdparty/nlohmann-json/json.hpp>
#include "google/protobuf/text_format.h"

#include <future>

namespace osquery
{
    FLAG(bool, decorations_top_level, false, "test");
}

/* This replaces the osquery sdk and always return the same output 
 * The substitution is done via the factory replacement. 
 * */ 
class FakeIOsqueryClient : public osqueryclient::IOsqueryClient{
    public: 
    void connect(const std::string& ) override{}
    osquery::Status query(const std::string& , osquery::QueryData & ) override
    {
        return osquery::Status::success();
    }

    osquery::Status getQueryColumns(const std::string& , osquery::QueryData & ) override
    {
        return osquery::Status::success();
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
        std::shared_ptr<std::promise<void>> m_promise; 
        std::shared_ptr<std::string> m_result; 

    public:
        DummyDispatcher( std::shared_ptr<std::promise<void>> p, std::shared_ptr<std::string> r) : 
            m_promise(p), m_result(r){}
        void sendResponse(const std::string& /*correlationId*/, const QueryResponse& response) override
        {

            *m_result = serializeToJson(response); 
            m_promise->set_value(); 
            // uncomment to see the response from osquery
            //std::cout << jsonContent.dump(2) << std::endl;
        }
        std::unique_ptr<IResponseDispatcher> clone() override{
            return std::unique_ptr<IResponseDispatcher>{new DummyDispatcher(m_promise, m_result)}; 
        }

    };

} // namespace livequery


#ifdef HasLibFuzzer
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
    "name": ")sophos" << iname << R"sophos(",
    "query": ")sophos" << iquery << R"sophos("
})sophos"; 
    std::string serializedQuery = buildjson.str(); 
    /*    std::cout << serializedQuery << std::endl; */

    std::shared_ptr<std::promise<void>> promise = std::make_shared<std::promise<void>>(); 
    std::shared_ptr<std::string> holdResult = std::make_shared<std::string>(); 
    std::future<void> queryFinishedFuture = promise->get_future();

    livequery::ParallelQueryProcessor processor{
	   std::unique_ptr<livequery::IQueryProcessor>{new osqueryclient::OsqueryProcessor("/tmp/notused")}, 
           std::unique_ptr<livequery::IResponseDispatcher>{new livequery::DummyDispatcher(promise, holdResult)}}; 

    osqueryclient::factory().replace([](){
            return std::unique_ptr<osqueryclient::IOsqueryClient>(new FakeIOsqueryClient{}); 
            });
    
    processor.addJob(serializedQuery, "1234");    

    // query should timeout after 10 seconds so if the thread is still running after 10 secs then osquery is hanging
    if (queryFinishedFuture.wait_for(std::chrono::seconds(1)) == std::future_status::timeout)
    {
        std::cerr << "Query did not pass the validation test" << std::endl; 
        return; 
    }
 
   std::string expectSuccessContains = R"sophos("errorCode":0,"errorMessage":"OK","sizeBytes":0})sophos"; 
   std::string result = *holdResult; 
   if ( result.find(expectSuccessContains) == std::string::npos){
       std::cerr << "Result not expected" << *holdResult << std::endl; 
   }

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
    Common::Logging::ConsoleLoggingSetup consoleLoggingSetup;
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
