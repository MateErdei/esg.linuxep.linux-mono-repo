#include <modules/CommsComponent/SplitProcesses.h>
#include <modules/Common/FileSystem/IFileSystem.h>
#include <modules/Common/FileSystem/IFilePermissions.h>
#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>

#include <Common/HttpSenderImpl/HttpSender.h>

#include <Common/SecurityUtils/ProcessSecurityUtils.h>
#include <Common/FileSystemImpl/FilePermissionsImpl.h>
#include <Common/HttpSender/RequestConfig.h>


void printUsageAndExit(std::string name, int code)
{
    std::cerr << "Usage: " << name << " usernameOfParent  usernameOfChild  url" << std::endl;
    exit(code);

}

using ReadOnlyMount = std::pair<std::string, std::string>;

void setUpCurlDependencies(const std::string& chrootDir, CommsComponent::UserConf childConf);

void mountDependenciesReadOnly(const std::string& chrootDir, CommsComponent::UserConf userConf,
                               std::vector<ReadOnlyMount> dependencies);

int main(int argc, char *argv[])
{
    using namespace CommsComponent;
    std::string parentRoot{"/tmp/testdorequest"};
    Common::ApplicationConfiguration::applicationConfiguration().setData(
            Common::ApplicationConfiguration::SOPHOS_INSTALL, parentRoot);

    std::string progName{argv[0]};
    if (argc != 4)
    {
        std::cerr << "Incorrect number of arguments" << std::endl;
        printUsageAndExit(progName, 1);
    }
    auto fs = Common::FileSystem::fileSystem();


    std::string networkChrootDir{Common::FileSystem::join(parentRoot, "jail")};


    CommsComponent::UserConf parentConf{argv[1], argv[1], "logparent"};
    CommsComponent::UserConf childConf{argv[2], argv[2], "logchild"};
    std::string url{argv[3]};


    if (fs->exists(parentRoot))
    {
        std::cerr << "Warning: Directory parentRoot already exists" << std::endl;
    }
    else
    {
        fs->makedirs(parentRoot);
        auto fp = Common::FileSystem::filePermissions();

        fp->chown(parentRoot, parentConf.userName, parentConf.userGroup);
        fp->chmod(parentRoot, 0770);
    }

    if (fs->exists(networkChrootDir))
    {
        std::cerr << "Warning: Directory networkChrootDir already exists" << std::endl;
    }
    else
    {
        fs->makedirs(networkChrootDir);
        auto fp = Common::FileSystem::filePermissions();

        fp->chown(networkChrootDir, childConf.userName, childConf.userGroup);
        fp->chmod(networkChrootDir, 0770);
    }
    //setUpCurlDependencies(networkChrootDir, childConf);
    std::vector<std::pair<std::string, std::string>> mountSources = {
            {"/lib",             "lib"},
            {"/usr/lib",         "usr/lib"},
            {"/etc/ssl",         "etc/ssl"},
            {"/etc/hosts",       "etc/hosts"},
            {"/etc/resolv.conf", "etc/resolv.conf"}};

    CommsComponent::CommsConfigurator configurator(networkChrootDir, childConf, parentConf, mountSources);

    auto childProc = [&](std::shared_ptr<MessageChannel>/*channel*/, OtherSideApi&/*childProxy*/) {
        std::cout << "Running child request" << std::endl;
        std::shared_ptr<Common::HttpSenderImpl::HttpSender> httpSender;
        std::vector<std::string> additionalHeaders;

        std::shared_ptr<Common::HttpSender::ICurlWrapper> curlWrapper =
                std::make_shared<Common::HttpSenderImpl::CurlWrapper>();

        httpSender = std::make_shared<Common::HttpSenderImpl::HttpSender>(curlWrapper);

        std::string m_defaultCertPath;

        Path defaultCertPathAfterGoogle = "/etc/ssl/certs/ca-certificates.crt";

        Common::HttpSender::RequestConfig req(Common::HttpSender::RequestType::GET, additionalHeaders, url, 443,
                                              defaultCertPathAfterGoogle,
                                              "");
        try
        {
            std::cout << "Going to do request" << std::endl;
            auto after_chroot = httpSender->doHttpsRequest(req);

            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::cout << "Curl return code after_chroot " << after_chroot << std::endl;
        }
        catch (const std::exception& ex)
        {
            std::cout << "do request exceptiont" << ex.what() << std::endl;
        }
    };

    auto parentProc = [](std::shared_ptr<MessageChannel> channel, OtherSideApi& childProxy) {
        childProxy.pushMessage("dorequest");
        std::string msg;
        channel->pop(msg, std::chrono::milliseconds(2000));
        std::cout << "Parent proc executed" << std::endl;
    };
    std::cout << "Running the splitProcesses Reactors" << std::endl;

    int code = CommsComponent::splitProcessesReactors(parentProc, std::move(childProc), configurator);
    std::cout << "Return code: " << code << std::endl;

    Common::SecurityUtils::unMount(Common::FileSystem::join(networkChrootDir, "etc/hosts"));
    return 0;
}