/*
    xmppbroadcast - XMPP communication for game channels
    Copyright (C) 2021  Autonomous Worlds Ltd

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "private/mucclient.hpp"

#include "testutils.hpp"

#include <xayautil/hash.hpp>

#include <gtest/gtest.h>

#include <memory>

namespace xmppbroadcast
{
namespace
{

/**
 * Custom channel, which puts received messages into a ReceivedMessages
 * instance for testing.
 */
class TestChannel : public MucClient::Channel
{

private:

  /** The queue of received messages to add to.  */
  ReceivedMessages queue;

protected:

  void
  MessageReceived (const std::string& msg) override
  {
    queue.Add (msg);
  }

public:

  explicit TestChannel (MucClient& c, const gloox::JID& j)
    : Channel(c, j)
  {}

  /**
   * Expects that the given messages have been received (or waits for them).
   */
  void
  ExpectMessages (const std::vector<std::string>& expected)
  {
    queue.Expect (expected);
  }

};

/**
 * Custom MUC client, which uses our TestChannels and allows waiting for and
 * expecting messages to be received for a particular channel ID.  It also
 * sets up the XMPP accounts based on the test config.
 */
class TestClient : public MucClient
{

protected:

  std::unique_ptr<Channel>
  CreateChannel (const gloox::JID& j) override
  {
    return std::make_unique<TestChannel> (*this, j);
  }

public:

  explicit TestClient (const std::string& gameId, const unsigned n)
    : MucClient(GetTestJid (n), GetPassword (n), gameId, GetServerConfig ().muc)
  {}

  /**
   * Retrieves the channel for a given ID, and expects it to be there.
   */
  TestChannel&
  Get (const xaya::uint256& id)
  {
    auto* res = GetChannel<TestChannel> (id);
    CHECK (res != nullptr);
    return *res;
  }

};

using MucClientTests = testing::Test;

TEST_F (MucClientTests, BasicConnection)
{
  TestClient client("test", 0);
  ASSERT_TRUE (client.Connect ());
  ASSERT_TRUE (client.IsConnected ());

  const auto id = xaya::SHA256::Hash ("foo");
  auto& channel = client.Get (id);
  channel.Send ("foo");
  channel.ExpectMessages ({"foo"});

  client.Disconnect ();
  ASSERT_FALSE (client.IsConnected ());
  EXPECT_EQ (client.GetChannel<TestChannel> (id), nullptr);

  client.Connect ();
  ASSERT_TRUE (client.IsConnected ());
  EXPECT_NE (client.GetChannel<TestChannel> (id), nullptr);
}

TEST_F (MucClientTests, ReceivingMessages)
{
  TestClient client1("test", 0);
  TestClient client2("test", 1);
  TestClient other("other", 0);

  const auto id1 = xaya::SHA256::Hash ("foo");
  const auto id2 = xaya::SHA256::Hash ("bar");

  for (auto* c : {&client1, &client2, &other})
    ASSERT_TRUE (c->Connect ());

  /* These two channels should not receive any messages, but we need to
     create them to make sure the test enforces that.  */
  client1.Get (id2);
  client2.Get (id2);
  other.Get (id1);

  auto& channel1 = client1.Get (id1);
  auto& channel2 = client2.Get (id1);
  SleepSome ();

  channel1.Send ("foo");
  channel1.Send ("bar");
  channel2.ExpectMessages ({"foo", "bar"});
  channel2.Send ("baz");
  channel2.ExpectMessages ({"baz"});
  channel1.ExpectMessages ({"foo", "bar", "baz"});
}

TEST_F (MucClientTests, RefreshReconnects)
{
  /* The interval must be sufficiently longer than the time it takes
     to actually get the connection go through.  */
  constexpr auto intv = std::chrono::milliseconds (500);

  TestClient client("test", 0);
  ASSERT_TRUE (client.Connect ());

  MucClient::Refresher refresher(client, intv);
  std::this_thread::sleep_for (intv / 3);
  client.Disconnect ();
  std::this_thread::sleep_for (intv / 3);
  EXPECT_FALSE (client.IsConnected ());

  std::this_thread::sleep_for (intv);
  EXPECT_TRUE (client.IsConnected ());
}

} // anonymous namespace
} // namespace xmppbroadcast
