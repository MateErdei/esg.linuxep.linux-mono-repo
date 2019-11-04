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
#include <modules/Common/UtilityImpl/StringUtils.h>
#include <modules/Common/TelemetryHelperImpl/TelemetryJsonToMap.h>
#include <simplefunction.pb.h>
#include <thread>
#ifdef HasLibFuzzer
#    include <libprotobuf-mutator/src/libfuzzer/libfuzzer_macro.h>
#    include <libprotobuf-mutator/src/mutator.h>
#endif
#include <fcntl.h>
#include <future>
#include <stddef.h>
#include <stdint.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/UtilityImpl/RegexUtilities.h>

/** Verify EnforceUtf8 must either do nothing (input was a valid utf8) or throw a std:invalid_argument
 *  Anything else is outside the contract.
 * */
void verifyEnforceUtf8(const std::string & input )
{
    try
    {
        Common::UtilityImpl::StringUtils::enforceUTF8(input);
    }catch ( std::invalid_argument& )
    {
    }

}

/**
 * telemetryJsonToMap converts a json string into a flat map key-> value or it throw std::runtime_error if it is an invalid
 * json. This test ensure that nothing different can occurr.
 * @param input
 */
void verifyTelemetryJsonToMap(const std::string & input)
{
    try
    {
        auto map = Common::Telemetry::flatJsonToMap(input);
        for( const auto & entry: map)
        {
            if (input.find(entry.first) == std::string::npos)
            {
                throw std::logic_error( "Each key must be present in the original json file");
            }
        }
    }catch ( std::runtime_error& ex)
    {
        std::string reason{ex.what()};
        if ( reason.find("JSON conversion to map failed") == std::string::npos)
        {
            // not the expected exception
            throw;
        }
    }
}

/**
 * It must always return a string never to throw.
 * @param input
 */
void verifyRegexUtility(const std::string & input)
{
    auto pos = input.find_first_of(';');
    std::string pattern;
    std::string content;
    if ( pos != std::string::npos)
    {
        pattern = input.substr(0, pos);
        content = input.substr(pos);
    }
    else
    {
        pattern = input;
        content = input;
    }
    std::string response = Common::UtilityImpl::returnFirstMatch(pattern, content);
    if( !response.empty())
    {
        if( response.size() > content.size())
        {
            throw  std::logic_error( "Captured bigger than content");
        }
    }

}


#ifdef HasLibFuzzer
DEFINE_PROTO_FUZZER(const SimpleFunctionProto::TestCase& testCase)
{
#else
void mainTest(const SimpleFunctionProto::TestCase& testCase)
{
#endif

    switch (testCase.functiontarget())
    {
        case SimpleFunctionProto::TestCase_FunctionTarget::TestCase_FunctionTarget_enforceUTF8:
            verifyEnforceUtf8(testCase.payload());
            break;
        case SimpleFunctionProto::TestCase_FunctionTarget::TestCase_FunctionTarget_jsonToMap:
            verifyTelemetryJsonToMap(testCase.payload());
            break;
        case SimpleFunctionProto::TestCase_FunctionTarget::TestCase_FunctionTarget_regexUtility:
            verifyRegexUtility(testCase.payload());
            break;
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
    SimpleFunctionProto::TestCase testcase;
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
