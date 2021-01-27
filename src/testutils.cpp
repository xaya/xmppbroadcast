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

#include "testutils.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <sstream>
#include <thread>

namespace xmppbroadcast
{

namespace
{

/**
 * Configuration for the local test environment.
 */
const ServerConfiguration LOCAL_SERVER =
  {
    "localhost",
    "muc.localhost",
    {
      {"xmpptest1", "password"},
      {"xmpptest2", "password"},
      {"xmpptest3", "password"},
    },
  };

} // anonymous namespace

const ServerConfiguration&
GetServerConfig ()
{
  return LOCAL_SERVER;
}

gloox::JID
GetTestJid (const unsigned n)
{
  const auto& cfg = GetServerConfig ();

  gloox::JID jid;
  jid.setUsername (cfg.accounts[n].name);
  jid.setServer (cfg.server);

  return jid;
}

std::string
GetPassword (const unsigned n)
{
  return GetServerConfig ().accounts[n].password;
}

void
SleepSome ()
{
  std::this_thread::sleep_for (std::chrono::milliseconds (10));
}

ReceivedMessages::~ReceivedMessages ()
{
  std::lock_guard<std::mutex> lock(mut);
  if (!received.empty ())
    {
      std::ostringstream items;
      while (!received.empty ())
        {
          items << received.front () << "; ";
          received.pop ();
        }
      ADD_FAILURE () << "Unexpected messages: " << items.str ();
    }
}

void
ReceivedMessages::Add (const std::string& msg)
{
  std::lock_guard<std::mutex> lock(mut);
  received.push (msg);
  cv.notify_one ();
}

void
ReceivedMessages::Expect (const std::vector<std::string>& expected)
{
  std::unique_lock<std::mutex> lock(mut);
  for (const auto& e : expected)
    {
      while (received.empty ())
        cv.wait (lock);

      ASSERT_EQ (e, received.front ());
      received.pop ();
    }
}

} // namespace xmppbroadcast
