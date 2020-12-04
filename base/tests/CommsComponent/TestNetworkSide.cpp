#include <Common/Logging/ConsoleLoggingSetup.h>
#include <gtest/gtest.h>
#include "MockOtherSideApi.h"
#include <CommsComponent/CommsMsg.h>
#include <modules/CommsComponent/NetworkSide.h>
#include <tests/Common/Helpers/LogInitializedTests.h>


class TestNetworkSide : public LogOffInitializedTests
{};


void sendMessageToNetworkSideAndCheckItRespondsWithoutCrashing(std::string& msgContent)
{
    CommsComponent::CommsNetwork commsNetwork;
    auto channel = std::make_shared<CommsComponent::MessageChannel>();
    CommsComponent::CommsMsg commsMsg;
    Common::HttpSender::RequestConfig requestConfig;

    // Unfortunately we have to set 'server' to empty string so that curl fails, due to the way the NetworkSide
    // and HttpSenderImpl have been implemented. NetworkSide doesn't use the IHttpSender interface but instead relies on
    // the HttpSenderImpl which as added other functions which are called from NetworkSide, therefore we
    // cannot mock out the HttpSender (m_sender) part of NetworkSide.
    requestConfig.setServer("");

    requestConfig.setData(msgContent);
    commsMsg.content = requestConfig;
    MockOtherSideApi mockedOutCommsDistributorSide{};

    // Unfortunately due to CommsNetwork endless loop with no stop method we need to add another message
    // here which we know will throw so that the loop ends.
    channel->push(CommsComponent::CommsMsg::serialize(commsMsg));
    channel->push("not valid, will throw");
    // We expect NetworkSide to parse and hand the request message to HttpSender which will return with a response
    // stating that the URL is bad because we set the server string to be empty but this is enough to prove
    // that we can parse large body content inside NetworkSide. We only expect this once even though there are
    // two messages being pushed in because our proper message will be the only one to generate and send a reply to
    // mockedOutCommsDitributorSide.
    EXPECT_CALL(mockedOutCommsDistributorSide,
                pushMessage(HasSubstr("URL using bad/illegal format or missing URL"))).Times(1);
    // Only way to get out of the endless loop was to add a message we know will throw so we have to expect that to
    // throw here.
    EXPECT_THROW(commsNetwork(channel, mockedOutCommsDistributorSide), std::runtime_error);
}

TEST_F(TestNetworkSide, NetworkSideHandlesEmptyMessageFromCommsDitrbutor) // NOLINT
{
    // Set content data to be empty
    std::string content = "";
    sendMessageToNetworkSideAndCheckItRespondsWithoutCrashing(content);
}

TEST_F(TestNetworkSide, NetworkSideHandlesLargeMessageFromCommsDitrbutor) // NOLINT
{
    // Set content data to be a very large string (100MB)
    std::string content = std::string(100000000,'a');
    sendMessageToNetworkSideAndCheckItRespondsWithoutCrashing(content);
}
