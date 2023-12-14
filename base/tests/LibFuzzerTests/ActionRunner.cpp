// Copyright 2023 Sophos Limited. All rights reserved.

#include "common/fuzzer/FuzzerUtils.h"
#include "tests/LibFuzzerTests/actionrunner.pb.h"

#include "Common/FileSystem/IFileSystem.h"
#include "Common/Logging/ConsoleLoggingSetup.h"
#include "Common/Logging/LoggerConfig.h"
#include "ResponseActions/ActionRunner/responseaction_main.h"

#include "src/libfuzzer/libfuzzer_macro.h"
#include "src/mutator.h"

#include <google/protobuf/text_format.h>


/**
 * LibFuzzer works only with clang, and developers machine are configured to run gcc.
 * For this reason, the flag USING_LIBFUZZER has been used to enable buiding 2 outputs:
 *   * the real fuzzer tool
 *   * An output that is capable of consuming the same sort of input file that is used by the fuzzer
 *     but can be build and executed inside the developers IDE.
 */
#ifdef USING_LIBFUZZER
DEFINE_PROTO_FUZZER([[maybe_unused]] const ActionRunnerProto::TestCase& testCase)
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
#ifndef USING_LIBFUZZER
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