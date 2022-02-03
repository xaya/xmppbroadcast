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

#include "xmppbroadcast.hpp"

#include "private/mucclient.hpp"

#include <glog/logging.h>

#include <functional>

/* Windows systems define a GetMessage macro, which makes this file fail to
   compile because of SendMessage in OffChainBroadcast.  */
#undef SendMessage

namespace xmppbroadcast
{

namespace
{

/**
 * Custom channel implementation that just forwards received messages
 * to a callback closure.
 */
class BcChannel : public MucClient::Channel
{

private:

  /** The callback for received messages.  */
  std::function<void (const std::string&)> cb;

protected:

  void
  MessageReceived (const std::string& msg) override
  {
    if (cb)
      cb (msg);
  }

public:

  explicit BcChannel (MucClient& c, const gloox::JID& j,
                      const std::function<void (const std::string&)>& r)
    : Channel(c, j), cb(r)
  {}

};

} // anonymous namespace

class XmppBroadcast::Impl : public MucClient
{

private:

  /** The main broadcast instance this belongs to.  */
  XmppBroadcast& bc;

  /** Refresher for the client, if it is running.  */
  std::unique_ptr<Refresher> refresher;

  friend class XmppBroadcast;

protected:

  std::unique_ptr<Channel> CreateChannel (const gloox::JID& j) override;

public:

  explicit Impl (XmppBroadcast& b, const std::string& gameId,
                 const gloox::JID& jid, const std::string& password,
                 const std::string& muc)
    : MucClient(gameId, jid, password, muc), bc(b)
  {}

  /**
   * When refreshed, also make sure to explicitly instantiate the channel
   * so we join it again after a reconnect.
   */
  void
  Refresh () override
  {
    MucClient::Refresh ();
    GetChannel<BcChannel> (bc.GetChannelId ());
  }

};

std::unique_ptr<MucClient::Channel>
XmppBroadcast::Impl::CreateChannel (const gloox::JID& j)
{
  return std::make_unique<BcChannel> (*this, j, [this] (const std::string& m)
    {
      bc.FeedMessage (m);
    });
}

XmppBroadcast::XmppBroadcast (
    xaya::SynchronisedChannelManager& cm,
    const std::string& gameId,
    const std::string& jid, const std::string& password,
    const std::string& mucServer)
  : xaya::ReceivingOffChainBroadcast(cm)
{
  impl = std::make_unique<Impl> (*this, gameId, jid, password, mucServer);
}

XmppBroadcast::XmppBroadcast (
    const xaya::uint256& id,
    const std::string& gameId,
    const std::string& jid, const std::string& password,
    const std::string& mucServer)
  : xaya::ReceivingOffChainBroadcast(id)
{
  impl = std::make_unique<Impl> (*this, gameId, jid, password, mucServer);
}

XmppBroadcast::~XmppBroadcast () = default;

void
XmppBroadcast::SendMessage (const std::string& msg)
{
  auto* c = impl->GetChannel<BcChannel> (GetChannelId ());
  if (c == nullptr)
    {
      LOG (WARNING) << "Cannot send message, disconnected?";
      return;
    }
  c->Send (msg);
}

void
XmppBroadcast::SetRootCA (const std::string& path)
{
  CHECK (!impl->IsConnected ()) << "XmppBroadcast is already connected";
  impl->SetRootCA (path);
}

void
XmppBroadcast::Start ()
{
  if (!impl->Connect ())
    LOG (WARNING) << "Failed with initial client connect, will keep trying";
  impl->refresher = std::make_unique<MucClient::Refresher> (*impl);

  /* The refresher will execute immediately, which ensures that we instantiate
     the channel once to join it.  */
}

void
XmppBroadcast::Stop ()
{
  impl->refresher.reset ();
  impl->Disconnect ();
}

} // namespace xmppbroadcast
