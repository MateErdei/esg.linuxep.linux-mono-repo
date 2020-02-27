/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

 SimpleFunctionTests make it easier to create new fuzz targets for simple functions where simple function is
 a function that receives a single string and 'do-something'. It would be usefull for parsing, checking, etc...

 In order to add a function, append a new FunctionTarget to the simplefunction.proto.
 Add a new case to the switch function in mainTest
 Add test cases to the systemtests folder: SupportFiles/base_data/fuzz/SimpleFunctionTests

******************************************************************************************************/

#include "FuzzerUtils.h"

#include "google/protobuf/text_format.h"
#include <livequery.pb.h>
#ifdef HasLibFuzzer
#    include <libprotobuf-mutator/src/libfuzzer/libfuzzer_macro.h>
#    include <libprotobuf-mutator/src/mutator.h>
#endif

#include <Common/FileSystem/IFileSystem.h>

#include <osquery/flagalias.h>
namespace osquery{

    FLAG(bool, decorations_top_level, false, "test");
}


#ifdef HasLibFuzzer
DEFINE_PROTO_FUZZER(const LiveQueryProto::TestCase& testCase)
{
#else
void mainTest(const LiveQueryProto::TestCase& testCase)
{
#endif
    if (testCase.functiontarget() ==  testCase.splitString)
    {
        if ( testCase.payload().find("additionalNon") != std::string::npos)
        {
            ::abort();
        }
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
