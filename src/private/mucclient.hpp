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

#ifndef XMPPBROADCAST_MUCCLIENT_HPP
#define XMPPBROADCAST_MUCCLIENT_HPP

#include <charon/xmppclient.hpp>
#include <xayautil/uint256.hpp>

#include <gloox/jid.h>
#include <gloox/message.h>
#include <gloox/mucroom.h>
#include <gloox/mucroomhandler.h>

#include <atomic>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

namespace xmppbroadcast
{

/**
 * The XMPP MUC client that we use for sending and receiving messages for
 * one or more channels.  This class is the underlying implementation for
 * both the XmppBroadcast class and our broadcast RPC server.
 */
class MucClient : private charon::XmppClient
{

public:

  class Channel;

private:

  /** The game ID this is for, which is part of channel names.  */
  const std::string gameId;

  /** The XMPP server on which rooms will be.  */
  const std::string server;

  /** Mutex for the channels map (but not the channels themselves).  */
  std::mutex mut;

  /** All channels that we have subscribed to or are currently joining.  */
  std::map<gloox::JID, std::unique_ptr<Channel>> channels;

  /**
   * Returns the JID of a room corresponding to the given channel ID.
   */
  gloox::JID GetRoomJid (const xaya::uint256& channelId) const;

  /**
   * When we get disconnected by the server, clean up the channels.
   */
  void HandleDisconnect () override;

protected:

  /**
   * Subclasses can implement this method to instantiate a new Channel
   * for this client and with the given JID.  They can use this to return
   * their own specific subclass of Channel.
   */
  virtual std::unique_ptr<Channel> CreateChannel (const gloox::JID& j);

public:

  /**
   * Sets up the client with the given data, but does not yet actually
   * try to connect.
   */
  explicit MucClient (const gloox::JID& j, const std::string& password,
                      const std::string& g, const std::string& s);

  virtual ~MucClient ();

  /**
   * Retrieves the channel to be used for the given ID.  It is created if
   * it doesn't exist (ownership remains always with the MucClient).
   * Might return null e.g. if we are not connected or the channel
   * errored.
   *
   * The result will be dynamic-casted to the template type, which should be
   * the one that CreateChannel returns.
   */
  template <typename C>
    C* GetChannel (const xaya::uint256& id);

  /**
   * Tries to connect to the XMPP server.  Returns true on success
   * and false on failure.
   */
  bool Connect ();

  /**
   * Disconnects from the XMPP server, cleaning up all the channels as well.
   */
  void Disconnect ();

  using XmppClient::IsConnected;

};

/**
 * A channel that we are subscribed to in the XMPP client.
 */
class MucClient::Channel : private gloox::MUCRoomHandler
{

private:

  /** The MucClient this belongs to.  */
  MucClient& client;

  /** The associated room's full JID.  */
  const gloox::JID roomJid;

  /** The gloox room handle.  */
  std::unique_ptr<gloox::MUCRoom> room;

  /**
   * Set to false when we received some error on the room or got disconnected
   * by the server.  Also set when we requested to leave.
   */
  std::atomic<bool> left;

  /**
   * Mutex for the local state.  This is used e.g. for the send queue
   * and condition variable, or the sender thread itself.
   */
  std::mutex mut;

  /**
   * Queue of messages to be sent.  When a message is sent throught the
   * public interface, it will just be added here.  We have a separate thread
   * that processes the queue and sends the messages, once we have gotten
   * a confirmation that the channel join succeeded.
   */
  std::queue<std::string> sendQueue;

  /**
   * Flag to indicate that the sender thread should stop, when the channel
   * instance is being destroyed.
   */
  bool stopSender;

  /** Condition variable signalled when we have a new message to send.  */
  std::condition_variable cvSendQueue;

  /**
   * The thread that processes the send queue.  It is created when
   * we have joined the channel successfully (from MarkJoined).
   */
  std::unique_ptr<std::thread> sender;

  /**
   * Runs a loop trying to send any messages queued up.  This is what the
   * sender thread executes.
   */
  void RunSendLoop ();

  void handleMUCError (gloox::MUCRoom* r, gloox::StanzaError) override;
  bool handleMUCRoomCreation (gloox::MUCRoom* r) override;
  void handleMUCMessage (gloox::MUCRoom* r, const gloox::Message& msg,
                         bool priv) override;
  void handleMUCParticipantPresence (
      gloox::MUCRoom* r, gloox::MUCRoomParticipant participant,
      const gloox::Presence& presence) override;

  void
  handleMUCSubject (gloox::MUCRoom* r, const std::string& nick,
                    const std::string& subject) override
  {}

  void
  handleMUCInviteDecline (gloox::MUCRoom* r, const gloox::JID& invitee,
                          const std::string& reason) override
  {}

  void
  handleMUCInfo (gloox::MUCRoom* r, const int features,
                 const std::string& name,
                 const gloox::DataForm* infoForm) override
  {}

  void
  handleMUCItems (gloox::MUCRoom* r,
                  const gloox::Disco::ItemList& items) override
  {}

protected:

  /**
   * Called when a message has been received on our channel.  Subclasses
   * can implement this to process the message accordingly.
   */
  virtual void
  MessageReceived (const std::string& msg)
  {}

public:

  explicit Channel (MucClient& c, const gloox::JID& j);
  virtual ~Channel ();

  Channel () = delete;
  Channel (const Channel&) = delete;
  void operator= (const Channel&) = delete;

  /**
   * Sends a message (queues it to be sent).
   */
  void Send (const std::string& msg);

  /**
   * Requests to leave the room.
   */
  void Leave ();

  /**
   * Returns true if this channel is active.  It gets inactive in case
   * it is requested to leave the room, or when we actually get some server-side
   * issue with the room / get disconnected.
   */
  bool
  IsActive () const
  {
    return !left;
  }

};

} // namespace xmppbroadcast

#include "mucclient.tpp"

#endif // XMPPBROADCAST_MUCCLIENT_HPP
