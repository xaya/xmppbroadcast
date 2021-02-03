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

#include "private/stanzas.hpp"

#include <xayautil/cryptorand.hpp>

#include <sstream>

namespace xmppbroadcast
{

DEFINE_int32 (xmppbroadcast_refresh_ms, 30'000,
              "Milliseconds between refresh / reconnection attempts");

/* ************************************************************************** */

MucClient::MucClient (const std::string& g,
                      const gloox::JID& j, const std::string& password,
                      const std::string& s)
  : XmppClient(j, password), gameId(g), server(s)
{
  RunWithClient ([] (gloox::Client& c)
    {
      c.registerStanzaExtension (new MessageStanza ());
    });
}

MucClient::~MucClient ()
{
  Disconnect ();
}

bool
MucClient::Connect ()
{
  return XmppClient::Connect (-1);
}

void
MucClient::Disconnect ()
{
  {
    std::lock_guard<std::mutex> lock(mut);
    for (auto& entry : channels)
      entry.second->Leave ();
  }

  /* This calls HandleDisconnect, which obtains the mutex lock again.
     Thus release the lock for this call.  */
  XmppClient::Disconnect ();

  std::lock_guard<std::mutex> lock(mut);
  channels.clear ();
}

gloox::JID
MucClient::GetRoomJid (const xaya::uint256& channelId) const
{
  std::ostringstream res;
  res << gameId << "_" << channelId.ToHex ()
      << "@" << server;
  return gloox::JID (res.str ());
}

void
MucClient::HandleDisconnect ()
{
  std::lock_guard<std::mutex> lock(mut);

  /* If we are still connected (i.e. this is an explicit request to
     disconnect), signal all rooms to leave if they haven't already.
     Otherwise (we were force-disconnected), just clean up the channels.  */
  if (IsConnected ())
    {
      for (auto& entry : channels)
        entry.second->Leave ();
    }
  else
    channels.clear ();
}

std::unique_ptr<MucClient::Channel>
MucClient::CreateChannel (const gloox::JID& j)
{
  return std::make_unique<Channel> (*this, j);
}

void
MucClient::Refresh ()
{
  VLOG (1) << "Refresh cycle for MUC client";

  if (!IsConnected ())
    {
      LOG (INFO) << "MUC client is disconnected, attempting reconnect...";
      Connect ();
    }
}

/* ************************************************************************** */

MucClient::Refresher::Refresher (MucClient& c)
  : Refresher(c, std::chrono::milliseconds (FLAGS_xmppbroadcast_refresh_ms))
{}

MucClient::Refresher::~Refresher ()
{
  {
    std::lock_guard<std::mutex> lock(mut);
    shouldStop = true;
    cv.notify_all ();
  }
  runner.join ();
}

void
MucClient::Refresher::Run ()
{
  std::unique_lock<std::mutex> lock(mut);
  while (!shouldStop)
    {
      lock.unlock ();
      client.Refresh ();
      lock.lock ();
      cv.wait_for (lock, intv);
    }
}

/* ************************************************************************** */

MucClient::Channel::Channel (MucClient& c, const gloox::JID& j)
  : client(c), roomJid(j), left(false)
{
  /* The nick names in the room are not used for anything.  But they have to be
     unique in order to avoid failures when joining.  Thus we simply use
     a random value, which will be (almost) guaranteed to be unique.  */
  xaya::CryptoRand rnd;
  const auto nick = rnd.Get<xaya::uint256> ();
  gloox::JID roomWithNick = roomJid;
  roomWithNick.setResource (nick.ToHex ());

  gloox::MUCRoomHandler* handler = this;
  client.RunWithClient ([&] (gloox::Client& c)
    {
      LOG (INFO) << "Attempting to join room " << roomWithNick.full ();
      room = std::make_unique<gloox::MUCRoom> (&c, roomWithNick, handler);
      room->join ();
    });
}

MucClient::Channel::~Channel ()
{
  std::unique_lock<std::mutex> lock(mut);

  Leave ();

  /* Make sure to wake up the sender thread if there is one, so we can
     join it without getting into a deadlock.  */
  stopSender = true;
  cvSendQueue.notify_all ();

  if (sender != nullptr)
    {
      lock.unlock ();
      sender->join ();
      sender.reset ();
      lock.lock ();
    }
}

void
MucClient::Channel::RunSendLoop ()
{
  std::unique_lock<std::mutex> lock(mut);
  while (!stopSender)
    {
      if (sendQueue.empty ())
        cvSendQueue.wait (lock);
      if (sendQueue.empty () || !client.IsConnected ())
        continue;

      /* Move the queue to a local variable, so we can release the general lock
         while we try to obtain the client lock.  Since only one sender thread
         is running, this is fine and won't lead to out-of-order messages.  */
      auto localQueue = std::move (sendQueue);
      CHECK (sendQueue.empty ());
      CHECK (!localQueue.empty ());
      lock.unlock ();
      client.RunWithClient ([this, &localQueue] (gloox::Client& c)
        {
          VLOG (2)
              << "Sending " << localQueue.size ()
              << " queued messages for " << roomJid.full ();
          while (!localQueue.empty ())
            {
              auto ext = std::make_unique<MessageStanza> (localQueue.front ());
              gloox::Message glooxMsg(gloox::Message::Groupchat, roomJid);
              glooxMsg.addExtension (ext.release ());
              c.send (glooxMsg);
              localQueue.pop ();
            }
        });
      lock.lock ();
    }
}

void
MucClient::Channel::Send (const std::string& msg)
{
  std::lock_guard<std::mutex> lock(mut);
  sendQueue.push (msg);
  cvSendQueue.notify_one ();
}

void
MucClient::Channel::Leave ()
{
  if (room != nullptr && !left)
    {
      LOG (INFO) << "Leaving room " << roomJid.full ();
      left = true;
      room->leave ();
    }
}

void
MucClient::Channel::handleMUCError (gloox::MUCRoom* r,
                                    const gloox::StanzaError error)
{
  CHECK_EQ (r, room.get ());
  LOG (WARNING)
      << "Received error for MUC room " << room->name () << ": " << error;
  left = true;
  room->leave ();
}

bool
MucClient::Channel::handleMUCRoomCreation (gloox::MUCRoom* r)
{
  CHECK_EQ (r, room.get ());
  LOG (WARNING) << "Creating non-existing MUC room " << roomJid.full ();
  return true;
}

void
MucClient::Channel::handleMUCMessage (gloox::MUCRoom* r,
                                      const gloox::Message& msg,
                                      const bool priv)
{
  CHECK_EQ (r, room.get ());

  if (priv)
    {
      LOG (WARNING)
          << "Ignoring private message on room " << room->name ()
          << " from " << msg.from ().full ();
      return;
    }

  VLOG (1)
      << "Received message from " << msg.from ().full ()
      << " on room " << room->name ();
  CHECK_EQ (msg.from ().bareJID (), roomJid);

  const auto* ext = msg.findExtension<MessageStanza> (MessageStanza::EXT_TYPE);
  if (ext != nullptr && ext->IsValid ())
    MessageReceived (ext->GetData ());
}

void
MucClient::Channel::handleMUCParticipantPresence (
    gloox::MUCRoom* r, const gloox::MUCRoomParticipant participant,
    const gloox::Presence& presence)
{
  CHECK_EQ (r, room.get ());
  VLOG (1)
      << "Presence for " << participant.jid->full ()
      << " with flags " << participant.flags
      << " on room " << room->name ()
      << ": " << presence.presence ();

  /* We are only interested in self presence, to mark the channel as joined
     or handle a disconnect.  */
  if (!(participant.flags & gloox::UserSelf))
    return;

  /* Nick changes also send an unavailable presence.  We want to not consider
     them as such, though.  */
  bool unavailable = (presence.presence () == gloox::Presence::Unavailable);
  if (participant.flags & gloox::UserNickChanged)
    unavailable = false;

  if (unavailable)
    {
      LOG (WARNING) << "We have been disconnected from " << room->name ();
      left = true;
      return;
    }

  std::lock_guard<std::mutex> lock(mut);
  if (sender == nullptr)
    {
      LOG (INFO) << "We have joined " << room->name () << " successfully";
      stopSender = false;
      sender = std::make_unique<std::thread> ([this] ()
        {
          RunSendLoop ();
        });
    }
}

/* ************************************************************************** */

} // namespace xmppbroadcast
