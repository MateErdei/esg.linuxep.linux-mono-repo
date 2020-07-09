
#include <iostream>
#include <Common/HttpSenderImpl/HttpSender.h>
#include <Common/HttpSenderImpl/RequestConfig.h>

#include <unistd.h>
#include <Common/SecurityUtils/ProcessSecurityUtils.h>


using namespace Common::HttpSenderImpl;

int main(/*int argc, char *argv[]*/)
{
    const char *defaultServer = "t1.sophosupd.com";
    const char *defaultServer2 = "google.com";
    const int defaultPort = 443;

    std::shared_ptr<HttpSender> httpSender;
    std::vector<std::string> additionalHeaders;

    std::string m_defaultCertPath;

    std::cout << "Starting dorequest in chroot" << std::endl;
    std::string argument;

    std::shared_ptr<Common::HttpSender::ICurlWrapper> curlWrapper =
            std::make_shared<Common::HttpSenderImpl::CurlWrapper>();

    httpSender = std::make_shared<HttpSender>(curlWrapper);

    additionalHeaders.emplace_back("header1");

    Path defaultCertPathb4Sophos = Common::FileSystem::join("/tmp/testchroot/etc", "telemetry_cert.pem");
    Path defaultCertPathB4Google = Common::FileSystem::join("/tmp/testchroot/etc", "ca-certificates.crt");


    RequestConfig req(RequestType::GET, additionalHeaders, defaultServer, defaultPort, defaultCertPathb4Sophos,
                      "linux/prod");

    auto before_chroot = httpSender->doHttpsRequest(req);
    std::cout << "\nreturn code" << before_chroot << std::endl;
    RequestConfig req1(RequestType::GET, additionalHeaders, defaultServer2, defaultPort, defaultCertPathB4Google,
                       "linux/prod");
    auto before_chroot2 = httpSender->doHttpsRequest(req1);
    std::cout << "\nreturn code" << before_chroot2 << std::endl;

    Common::SecurityUtils::chrootAndDropPrivileges("nobody", "nobody", "/tmp/testchroot");
    sleep(3);

    Path defaultCertPathAfterSophos = Common::FileSystem::join("/etc", "telemetry_cert.pem");
    Path defaultCertPathAfterGoogle = Common::FileSystem::join("/etc", "ca-certificates.crt");
    RequestConfig req2(RequestType::GET, additionalHeaders, defaultServer, defaultPort, defaultCertPathAfterSophos,
                       "linux/prod");
    auto after_chroot = httpSender->doHttpsRequest(req2);
    std::cout << "return code" << after_chroot << std::endl;
    sleep(2);
    RequestConfig req3(RequestType::GET, additionalHeaders, defaultServer2, defaultPort, defaultCertPathAfterGoogle,
                       "");
    auto after_chroot2 = httpSender->doHttpsRequest(req3);
    std::cout << "return code" << after_chroot2 << std::endl;
    return 0;
}