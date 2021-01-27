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

#ifndef XMPPBROADCAST_TESTUTILS_HPP
#define XMPPBROADCAST_TESTUTILS_HPP

#include <gloox/jid.h>

#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

namespace xmppbroadcast
{

/**
 * Data for one of the test accounts that we use.
 */
struct TestAccount
{

  /** The username for the XMPP server.  */
  const char* name;

  /** The password for logging into the server.  */
  const char* password;

};

/**
 * Full set of "server configuration" used for testing.
 */
struct ServerConfiguration
{

  /** The XMPP server used.  */
  const char* server;

  /** The MUC service.  */
  const char* muc;

  /** The test accounts.  */
  TestAccount accounts[3];

};

/**
 * Returns the ServerConfiguration instance that should be used throughout
 * testing.
 *
 * This expects a local environment (with server running on localhost)
 * as it is set up e.g. by Charon's test/env Docker scripts.
 */
const ServerConfiguration& GetServerConfig ();

/**
 * Returns the JID of the n-th test account from the selected server config.
 */
gloox::JID GetTestJid (unsigned n);

/**
 * Returns the password for the n-th test account.
 */
std::string GetPassword (unsigned n);

/**
 * Sleeps some amount of time that should be sufficient to let e.g. the XMPP
 * server process stuff.
 */
void SleepSome ();

/**
 * Helper class that represents received messages from a broadcast and allows
 * to wait for them / compare them against expectations.  Messages can be
 * added from different threads (e.g. received from XMPP), and the test
 * itself can then expect a given sequence of messages, which will check
 * the received messages as well as wait for more if needed.
 */
class ReceivedMessages
{

private:

  /** Messages received and not yet checked from the test.  */
  std::queue<std::string> received;

  /** Mutex for the received messages and condition variable.  */
  std::mutex mut;

  /** Condition variable for waiting on received messages.  */
  std::condition_variable cv;

public:

  ReceivedMessages () = default;

  /**
   * The destructor checks that no unexpected messages are left.
   */
  ~ReceivedMessages ();

  /**
   * Adds a new message to the received queue.
   */
  void Add (const std::string& msg);

  /**
   * Expects that we receive (or have already received) the given messages
   * in order.  Waits for messages to arrive if necessary.
   */
  void Expect (const std::vector<std::string>& expected);

};

} // namespace xmppbroadcast

#endif // XMPPBROADCAST_TESTUTILS_HPP
