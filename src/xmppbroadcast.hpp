/*
    xmppbroadcast - XMPP communication for game channels
    Copyright (C) 2021-2022  Autonomous Worlds Ltd

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

#ifndef XMPPBROADCAST_XMPPBROADCAST_HPP
#define XMPPBROADCAST_XMPPBROADCAST_HPP

#include <gamechannel/recvbroadcast.hpp>
#include <gamechannel/syncmanager.hpp>

#include <memory>
#include <string>

namespace xmppbroadcast
{

/**
 * A ReceivingOffChainBroadcast for game channels that connects to an XMPP
 * server and uses a MUC room for broadcasting and receiving messages.
 */
class XmppBroadcast : public xaya::ReceivingOffChainBroadcast
{

private:

  class Impl;

  /** The actual implementation, which is hidden from the header.  */
  std::unique_ptr<Impl> impl;

protected:

  /**
   * For testing, we support constructing the broadcast instance
   * without a ChannelManager.
   */
  explicit XmppBroadcast (
      const xaya::uint256& id, const std::string& gameId,
      const std::string& jid, const std::string& password,
      const std::string& mucServer);

  void SendMessage (const std::string& msg) override;

public:

  explicit XmppBroadcast (
      xaya::SynchronisedChannelManager& cm, const std::string& gameId,
      const std::string& jid, const std::string& password,
      const std::string& mucServer);
  ~XmppBroadcast ();

  XmppBroadcast () = delete;
  XmppBroadcast (const XmppBroadcast&) = delete;
  void operator= (const XmppBroadcast&) = delete;

  /**
   * Sets the trusted root CA for the XMPP TLS connection.
   */
  void SetRootCA (const std::string& path);

  /* We use our own custom start/stop, which connects the XMPP client
     and runs a refresher.  The XMPP receiving thread will push messages
     to us, which we feed back to OffChainBroadcast.  */
  void Start () override;
  void Stop () override;

};

} // namespace xmppbroadcast

#endif // XMPPBROADCAST_XMPPBROADCAST_HPP
