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

#include "rpcserver.hpp"

#include "rpc-stubs/broadcastrpcclient.h"
#include "testutils.hpp"
#include "xmppbroadcast_tests.hpp"

#include <gamechannel/rpcbroadcast.hpp>
#include <xayautil/hash.hpp>

#include <json/json.h>
#include <jsonrpccpp/client/connectors/httpclient.h>
#include <jsonrpccpp/common/exception.h>

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <gtest/gtest.h>

#include <atomic>
#include <sstream>
#include <thread>

namespace xmppbroadcast
{

DECLARE_int32 (xmppbroadcast_receive_timeout_ms);

namespace
{

/** The port we use for the test server.  */
constexpr int PORT = 29'183;

/**
 * Returns the full endpoint of the local server.
 */
std::string
GetEndpoint ()
{
  std::ostringstream res;
  res << "http://localhost:" << PORT;
  return res.str ();
}

/**
 * Parses a given string as JSON.
 */
Json::Value
ParseJson (const std::string& str)
{
  std::istringstream in(str);
  Json::Value res;
  in >> res;
  return res;
}

/**
 * RpcBroadcast (from game channels) subclass that connects to our server
 * and stores received messages into a queue.
 */
class TestRpcBroadcast : public TestBroadcast<xaya::RpcBroadcast>
{

public:

  explicit TestRpcBroadcast (const xaya::uint256& id)
    : TestBroadcast<xaya::RpcBroadcast>(GetEndpoint (), id)
  {}

};

/**
 * Wrapper around the base BroadcastRpcClient that also contains the
 * HTTP client for it and connects to our test server.
 */
class TestRpcClient
{

private:

  /** The HTTP client connector.  */
  jsonrpc::HttpClient http;

  /** The actual RPC client.  */
  BroadcastRpcClient rpc;

public:

  TestRpcClient ()
    : http(GetEndpoint ()), rpc(http)
  {}

  /**
   * Exposes the actual RPC client.
   */
  BroadcastRpcClient*
  operator-> ()
  {
    return &rpc;
  }

};

/**
 * RpcServer instance used in the test.  It already uses our test
 * account for XMPP, and the test port for listening.
 */
class TestServer : public RpcServer
{

public:

  TestServer ()
    : RpcServer("test", GetTestJid (0).full (), GetPassword (0),
                GetServerConfig ().muc)
  {}

  void
  Start ()
  {
    RpcServer::Start (PORT);
  }

};

class RpcServerTests : public testing::Test
{

protected:

  static const std::string id1;
  static const std::string id2;

  TestServer srv;
  TestRpcClient client;

  RpcServerTests () = default;

};

const std::string RpcServerTests::id1 = xaya::SHA256::Hash ("foo").ToHex ();
const std::string RpcServerTests::id2 = xaya::SHA256::Hash ("bar").ToHex ();

TEST_F (RpcServerTests, StartStop)
{
  srv.Start ();
  SleepSome ();
  srv.Stop ();

  srv.Start ();
  SleepSome ();
  /* The destructor should stop the server.  */
}

TEST_F (RpcServerTests, StopNotification)
{
  srv.Start ();

  std::atomic<bool> started(false);
  std::thread t([&] ()
    {
      started = true;
      srv.Wait ();
    });

  while (!started)
    SleepSome ();

  try
    {
      client->stop ();
    }
  catch (const jsonrpc::JsonRpcException& exc)
    {
      /* When shutting down the server, it can happen that the server shuts
         down so quickly that it can't properly reply to / close down the
         HTTP connection and the client throws an error.  That is fine.  */
      LOG (WARNING) << "Ignoring RPC error on stop: " << exc.what ();
    }

  t.join ();
}

TEST_F (RpcServerTests, Errors)
{
  /* The server is not started.  */
  EXPECT_THROW (client->send (id1, "Zm9v"), jsonrpc::JsonRpcException);
  EXPECT_THROW (client->getseq (id1), jsonrpc::JsonRpcException);
  EXPECT_THROW (client->receive (id1, 0), jsonrpc::JsonRpcException);

  srv.Start ();
  /* These channels are invalid.  */
  EXPECT_THROW (client->getseq ("x"), jsonrpc::JsonRpcException);
  EXPECT_THROW (client->receive ("x", 0), jsonrpc::JsonRpcException);
  /* send is just a notification, so we don't expect a result (not even
     an exception thrown).  The server should just ignore it and not
     crash, though.  */
  client->send ("x", "Zm9v");
  client->send (id1, "invalid base64");

  /* Make sure the server is fine.  */
  client->send (id1, "YmFy");
  EXPECT_EQ (client->receive (id1, 0), ParseJson (R"({
    "seq": 1,
    "messages": ["YmFy"]
  })"));
}

TEST_F (RpcServerTests, BasicReceiving)
{
  srv.Start ();
  EXPECT_EQ (client->getseq (id1), ParseJson (R"({"seq": 0})"));

  client->send (id1, "Zm9v");
  client->send (id1, "YmFy");
  SleepSome ();

  EXPECT_EQ (client->getseq (id1), ParseJson (R"({"seq": 2})"));
  EXPECT_EQ (client->receive (id1, 0), ParseJson (R"({
    "seq": 2,
    "messages": ["Zm9v", "YmFy"]
  })"));
  EXPECT_EQ (client->receive (id1, 1), ParseJson (R"({
    "seq": 2,
    "messages": ["YmFy"]
  })"));
}

TEST_F (RpcServerTests, ReceiveWaits)
{
  srv.Start ();

  /* Send delayed and asynchronously.  */
  std::thread sender([] ()
    {
      TestRpcClient client2;
      SleepSome ();
      client2->send (id1, "YmF6");
    });

  EXPECT_EQ (client->receive (id1, 0), ParseJson (R"({
    "seq": 1,
    "messages": ["YmF6"]
  })"));
  sender.join ();
}

TEST_F (RpcServerTests, MultipleChannels)
{
  srv.Start ();

  client->send (id1, "Zm9v");
  client->send (id2, "YmFy");
  client->send (id1, "YmF6");
  SleepSome ();

  EXPECT_EQ (client->receive (id1, 0), ParseJson (R"({
    "seq": 2,
    "messages": ["Zm9v", "YmF6"]
  })"));
  EXPECT_EQ (client->receive (id2, 0), ParseJson (R"({
    "seq": 1,
    "messages": ["YmFy"]
  })"));
}

TEST_F (RpcServerTests, CompatibilityToXmppBroadcast)
{
  /* This test connects a direct XmppBroadcast and a game-channel
     RpcBroadcast going through our server together.  Both of them
     should be able to talk to each other.  */

  FLAGS_xmppbroadcast_receive_timeout_ms = 100;

  srv.Start ();

  xaya::uint256 id;
  CHECK (id.FromHex (id1));

  TestRpcBroadcast bc1(id);
  TestXmppBroadcast bc2(1, id);
  SleepSome ();

  bc1.SendMessage ("foo");
  bc2.ExpectMessages ({"foo"});
  bc2.SendMessage ("bar");
  bc1.ExpectMessages ({"foo", "bar"});
  bc2.ExpectMessages ({"bar"});
}

} // anonymous namespace
} // namespace xmppbroadcast
