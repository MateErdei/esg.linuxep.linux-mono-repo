// Copyright 2023 Sophos Limited. All rights reserved.

#include "FuzzerUtils.h"

#include "google/protobuf/text_format.h"

#include "Common/FileSystem/IFileSystem.h"
#include "Common/Logging/ConsoleLoggingSetup.h"
#include "Common/Logging/LoggerConfig.h"

#include "ResponseActions/ActionRunner/responseaction_main.h"

#include "actionrunner.pb.h"

#ifdef HasLibFuzzer
#    include <libprotobuf-mutator/src/libfuzzer/libfuzzer_macro.h>
#    include <libprotobuf-mutator/src/mutator.h>
#endif
/**
 * LibFuzzer works only with clang, and developers machine are configured to run gcc.
 * For this reason, the flag HasLibFuzzer has been used to enable buiding 2 outputs:
 *   * the real fuzzer tool
 *   * An output that is capable of consuming the same sort of input file that is used by the fuzzer
 *     but can be build and executed inside the developers IDE.
 */
#ifdef HasLibFuzzer
DEFINE_PROTO_FUZZER(const ActionRunnerProto::TestCase& testCase)
{
#else
void mainTest( ActionRunnerProto::TestCase testcase)
{

    std::vector<std::string> arguments = {testcase.type(),testcase.payload(),testcase.correlation()};
    std::vector<char*> argvs;
    for (const auto& arg : arguments)
    {
        argvs.push_back((char*)arg.data());
    }
    argvs.push_back(nullptr);
    ActionRunner::responseaction_main::main(argvs.size()-1, argvs.data());
#endif
}
#ifndef HasLibFuzzer
int main(int argc, char* argv[])
{
    ActionRunnerProto::TestCase testcase;
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

     mainTest( testcase);
     return 0;
}

#endif