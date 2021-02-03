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

#include "xmppbroadcast.hpp"

#include "testutils.hpp"

#include <xayautil/hash.hpp>
#include <xayautil/uint256.hpp>

#include <glog/logging.h>
#include <gtest/gtest.h>

namespace xmppbroadcast
{
namespace
{

/**
 * Custom broadcast instance for testing.  It uses our XMPP test accounts,
 * and stores received messages into a ReceivedMessages instance.
 */
class TestBroadcast : public XmppBroadcast
{

private:

  /** Received messages are stored here.  */
  ReceivedMessages messages;

protected:

  void
  FeedMessage (const std::string& msg) override
  {
    messages.Add (msg);
  }

public:

  TestBroadcast (const unsigned n, const xaya::uint256& id)
    : XmppBroadcast(id, "test",
                    GetTestJid (n).full (), GetPassword (n),
                    GetServerConfig ().muc)
  {
    Start ();
  }

  void
  ExpectMessages (const std::vector<std::string>& expected)
  {
    messages.Expect (expected);
  }

  using XmppBroadcast::SendMessage;

};

class XmppBroadcastTests : public testing::Test
{

protected:

  static const xaya::uint256 id1;
  static const xaya::uint256 id2;

  XmppBroadcastTests () = default;

};

const xaya::uint256 XmppBroadcastTests::id1 = xaya::SHA256::Hash ("foo");
const xaya::uint256 XmppBroadcastTests::id2 = xaya::SHA256::Hash ("bar");

TEST_F (XmppBroadcastTests, BasicMessageExchange)
{
  TestBroadcast bc1(0, id1);
  TestBroadcast bc2(1, id1);
  SleepSome ();

  bc1.SendMessage ("foo");
  bc2.ExpectMessages ({"foo"});
  bc2.SendMessage ("bar");
  bc1.ExpectMessages ({"foo", "bar"});
  bc2.ExpectMessages ({"bar"});
}

TEST_F (XmppBroadcastTests, MultipleChannels)
{
  TestBroadcast bc1(0, id1);
  TestBroadcast bc2(1, id1);
  TestBroadcast bc3(2, id2);
  SleepSome ();

  bc1.SendMessage ("foo");
  bc3.SendMessage ("bar");
  bc3.ExpectMessages ({"bar"});

  bc1.SendMessage ("baz");
  bc1.ExpectMessages ({"foo", "baz"});
  bc2.ExpectMessages ({"foo", "baz"});
}

TEST_F (XmppBroadcastTests, IntermittentStop)
{
  TestBroadcast bc1(0, id1);
  TestBroadcast bc2(1, id1);

  bc2.Stop ();
  SleepSome ();

  bc1.SendMessage ("foo");
  SleepSome ();

  bc2.Start ();
  SleepSome ();

  bc1.SendMessage ("bar");
  bc2.ExpectMessages ({"bar"});
  bc1.ExpectMessages ({"foo", "bar"});
}

} // anonymous namespace
} // namespace xmppbroadcast
