// Copyright 2018-2023 Sophos Limited. All rights reserved.

/******************************************************************************************************
 SimpleFunctionTests make it easier to create new fuzz targets for simple functions where simple function is
 a function that receives a single string and 'do-something'. It would be useful for parsing, checking, etc...

 In order to add a function, append a new FunctionTarget to the simplefunction.proto.
 Add a new case to the switch function in mainTest
 Add test cases to the systemtests folder: SupportFiles/base_data/fuzz/SimpleFunctionTests

******************************************************************************************************/

#include "FuzzerUtils.h"

#include "google/protobuf/text_format.h"

#include "Common/FileSystem/IFileSystem.h"
#include "Common/Logging/ConsoleLoggingSetup.h"
#include "Common/Logging/LoggerConfig.h"
#include "Common/Obfuscation/ICipherException.h"
#include "Common/ObfuscationImpl/Base64.h"
#include "Common/ObfuscationImpl/Obfuscate.h"
#include "Common/Process/IProcess.h"
#include "Common/SslImpl/Digest.h"
#include "Common/TelemetryConfigImpl/Serialiser.h"
#include "Common/TelemetryHelperImpl/TelemetryJsonToMap.h"
#include "Common/UtilityImpl/RegexUtilities.h"
#include "Common/UtilityImpl/StringUtils.h"
#include "tests/Common/Helpers/TempDir.h"

#include "simplefunction.pb.h"

#ifdef HasLibFuzzer
#    include <libprotobuf-mutator/src/libfuzzer/libfuzzer_macro.h>
#    include <libprotobuf-mutator/src/mutator.h>
#endif

class FileWriterForBase64
{
    Tests::TempDir m_tempdir;
public:
    FileWriterForBase64() : m_tempdir("/tmp"){}
    std::string writeContent(const std::string & content)
    {
        std::string filename{"testcase_base64.txt"};
        m_tempdir.createFile(filename, content);
        return m_tempdir.absPath(filename);
    }

};
void callRegex(const std::string & regex,const std::string & content)
{
    try
    {
        std::string response = Common::UtilityImpl::returnFirstMatch(regex, content);
        if( !response.empty())
        {
            if( response.size() > content.size())
            {
                throw  std::logic_error( "Captured bigger than content");
            }
        }

    }catch (std::regex_error& )
    {
        // this is ignored.
    }
}
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

    }
    catch ( std::runtime_error& ex)
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
    callRegex(R"(([\\\\w]+)(-[\\\\w]+)?_policy.xml)",input);
    callRegex(R"(\d{4}\d{2}\d{2} (\d{2})\d{2}\d{2})",input);
}

void verifyBase64(const std::string & input)
{
    static FileWriterForBase64 fileWriterForBase64;
    std::string encoded;
    try
    {
        std::string fullPath = fileWriterForBase64.writeContent(input);

        auto proc= Common::Process::createProcess();
        proc->exec("/usr/bin/base64", {fullPath});
        encoded = proc->output();
        (void) proc->exitCode();

    }catch(std::exception& ex)
    {
        std::cerr << "Ignoring error in preparation of the test: " << ex.what();
        return;
    }

    std::string decoded = Common::ObfuscationImpl::Base64::Decode(encoded);
    if (decoded != input)
    {
        std::cerr << "Failed decode message. Input:" << input << " encoded: "<< encoded << " decoded: " << decoded << std::endl;
        abort();
    }
}

void verifyDeobfuscate(const std::string& input)
{
    try
    {
        Common::ObfuscationImpl::SECDeobfuscate(input);
    }
    catch (Common::Obfuscation::ICipherException& e)
    {
        return;
    }
    catch (std::exception& ex)
    {
        std::string reason = ex.what();

        if (reason.find("SECDeobfuscation Failed") != std::string::npos)
        {
            return;
        }
        std::cerr << "Failed for the invalid reason: " << reason << std::endl;
        abort();
    }
}

void verifyObfuscate(const std::string& input)
{
    try
    {
        std::string obfuscatedInput = Common::ObfuscationImpl::SECObfuscate(input);
        std::string deobfuscatedInput = Common::ObfuscationImpl::SECDeobfuscate(obfuscatedInput);
        if (input != deobfuscatedInput)
        {
            std::cerr << "Input string does not match output string.\nInput: " << input
                      << "\nOutput: " << deobfuscatedInput << std::endl;
            abort();
        }
    }
    catch (Common::Obfuscation::IObfuscationException& e)
    {
        return;
    }
    catch (std::invalid_argument& e)
    {
        return;
    }
    catch (std::exception& ex)
    {
        std::string reason = ex.what();

        if (reason.find("SECObfuscation Failed") != std::string::npos ||
            reason.find("SECDeobfuscation Failed") != std::string::npos)
        {
            return;
        }
        std::cerr << "Failed for the invalid reason: " << reason << std::endl;
        abort();
    }
}

void verifyMD5(const std::string& input)
{
    std::ignore = Common::SslImpl::calculateDigest(Common::SslImpl::Digest::md5, input);
}

void verifySplitString(const std::string & input)
{
    auto pos = input.find_first_of(';');
    std::string delimiter;
    std::string content;
    if ( pos != std::string::npos)
    {
        delimiter = input.substr(0, pos);
        content = input.substr(pos);
    }
    else
    {
        delimiter = "=";
        content = input;
    }
    std::vector<std::string> response = Common::UtilityImpl::StringUtils::splitString(content, delimiter);
    if(!content.empty())
    {
        if (response.empty())
        {
            std::cerr << "vector returned from splitString can not be empty for a non empty content" << std::endl;

            ::abort();
        }
    }
    else
    {
        if (!(response.size() == 1 && response[0] == ""))
        {
            std::cerr << "for no data, vector returned from splitString should be {''}" << std::endl;
            ::abort();
        }
        else
        {
            return;
        }
    }
    size_t count=0;
    for( auto & element: response)
    {
        count += element.size();
    }

    int delimit_chars = (response.size() -1) * delimiter.size();
    count += delimit_chars;

    if( count != content.size())
    {
        std::cerr << "The sum of the parts should be equal the content" << std::endl;
        ::abort();
    }
}

void verifyTelemetryConfig(const std::string & input)
{
    try
    {
        auto config = Common::TelemetryConfigImpl::Serialiser::deserialise(input);
        (void) config.isValid();

    }catch (std::exception& ex)
    {
        std::string reason = ex.what();
        if( reason.find("Configuration JSON is invalid") != std::string::npos)
        {
            return;
        }
        else if( reason.find("Configuration from deserialised JSON is invalid") != std::string::npos)
        {
            return;
        }
        std::cerr << "Non expected error " << ex.what() << std::endl;
        throw;
    }
}

class LogConf{
    public: 
    LogConf(): m_consoleSetup{Common::Logging::LOGOFFFORTEST()}{}
    Common::Logging::ConsoleLoggingSetup m_consoleSetup;
};

#ifdef HasLibFuzzer
DEFINE_PROTO_FUZZER(const SimpleFunctionProto::TestCase& testCase)
{
#else
void mainTest(const SimpleFunctionProto::TestCase& testCase)
{
#endif
    static LogConf logconf;
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
        case SimpleFunctionProto::TestCase_FunctionTarget::TestCase_FunctionTarget_base64:
            verifyBase64(testCase.payload());
            break;
        case SimpleFunctionProto::TestCase_FunctionTarget::TestCase_FunctionTarget_deobfuscate:
            verifyDeobfuscate(testCase.payload());
            break;
        case SimpleFunctionProto::TestCase_FunctionTarget::TestCase_FunctionTarget_obfuscate:
            verifyObfuscate(testCase.payload());
            break;
        case SimpleFunctionProto::TestCase_FunctionTarget::TestCase_FunctionTarget_md5:
            verifyMD5(testCase.payload());
            break;
        case SimpleFunctionProto::TestCase_FunctionTarget::TestCase_FunctionTarget_splitString:
            verifySplitString(testCase.payload());
            break;
        case SimpleFunctionProto::TestCase_FunctionTarget::TestCase_FunctionTarget_telemetryConfig:
            verifyTelemetryConfig(testCase.payload());
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
