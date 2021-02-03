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

#ifndef XMPPBROADCAST_RPCSERVER_HPP
#define XMPPBROADCAST_RPCSERVER_HPP

#include <memory>
#include <string>

namespace xmppbroadcast
{

/**
 * This class encapsulates a JSON-RPC server that connects to XMPP
 * and runs a local broadcast RPC server that bridges to the XMPP
 * relay.  It will dynamically create and join channels as needed
 * based on the RPC calls received.
 */
class RpcServer
{

private:

  class Impl;

  /** The actual implementation, which is hidden from the header.  */
  std::unique_ptr<Impl> impl;

public:

  /**
   * Constructs a server that will connect with the given JID and password,
   * use the given game ID for the channels, and use a particular MUC
   * service.
   */
  explicit RpcServer (const std::string& gameId,
                      const std::string& jid, const std::string& password,
                      const std::string& mucServer);

  ~RpcServer ();

  RpcServer () = delete;
  RpcServer (const RpcServer&) = delete;
  void operator= (const RpcServer&) = delete;

  /**
   * Starts the server.  This connects the XMPP client and makes the
   * server listen for connections on the given port.
   */
  void Start (int port, bool onlyLocal = true);

  /**
   * Stops the server.  Signals it to shut down and waits for the server
   * to be down.
   */
  void Stop ();

  /**
   * Lets the server run (it should have been started already) and waits
   * for it to shut down by itself, e.g. after a "stop" RPC notification.
   */
  void Wait ();

};

} // namespace xmppbroadcast

#endif // XMPPBROADCAST_RPCSERVER_HPP
