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

#include "private/mucclient.hpp"
#include "rpc-stubs/broadcastrpcserverstub.h"

#include <xayautil/base64.hpp>

#include <jsonrpccpp/common/errors.h>
#include <jsonrpccpp/common/exception.h>
#include <jsonrpccpp/server/connectors/httpserver.h>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace xmppbroadcast
{

DEFINE_int32 (xmppbroadcast_receive_timeout_ms, 3'000,
              "server-side timeout for receive calls in milliseconds");

/* ************************************************************************** */

namespace
{

/**
 * A custom MUC channel that keeps track of received messages in a queue
 * and allows querying them by sequence number.
 */
class MsgChannel : public MucClient::Channel
{

private:

  /** All received messages.  */
  std::vector<std::string> messages;

  /** Mutex for locking the list of messages and waiting for more.  */
  mutable std::mutex mut;

  /** Condition variable signalled when new messages are received.  */
  std::condition_variable cv;

protected:

  void MessageReceived (const std::string& msg) override;

public:

  explicit MsgChannel (MucClient& c, const gloox::JID& j)
    : Channel(c, j)
  {}

  /* Users of this class should make sure that there are no threads waiting
     for messages anymore when it gets destructed.  This is nothing we need
     to care about here.  */

  /**
   * Returns the current sequence number.
   */
  size_t GetSequenceNumber () const;

  /**
   * Receives messages from the given sequence number onwards.  Waits for
   * a certain amount of time if there are none.  The seq argument is
   * changed to the new sequence number after the returned messages
   * are accounted for.
   */
  std::vector<std::string> Receive (size_t& seq);

};

void
MsgChannel::MessageReceived (const std::string& msg)
{
  std::lock_guard<std::mutex> lock(mut);
  messages.push_back (msg);
  cv.notify_all ();
}

size_t
MsgChannel::GetSequenceNumber () const
{
  std::lock_guard<std::mutex> lock(mut);
  return messages.size ();
}

std::vector<std::string>
MsgChannel::Receive (size_t& seq)
{
  std::unique_lock<std::mutex> lock(mut);
  if (messages.size () <= seq)
    {
      const auto timeout = std::chrono::milliseconds (
          FLAGS_xmppbroadcast_receive_timeout_ms);
      cv.wait_for (lock, timeout);
    }

  std::vector<std::string> res;
  for (unsigned i = seq; i < messages.size (); ++i)
    res.push_back (messages[i]);

  seq = messages.size ();
  return res;
}

/**
 * The MUC client for our broadcast RPC server, using the MsgChannel
 * instances for channels.
 */
class RpcMucClient : public MucClient
{

protected:

  std::unique_ptr<Channel>
  CreateChannel (const gloox::JID& j) override
  {
    return std::make_unique<MsgChannel> (*this, j);
  }

public:

  using MucClient::MucClient;

};

/* ************************************************************************** */

/**
 * The actual JSON-RPC server that handles the queries.
 */
class RealServer : public BroadcastRpcServerStub
{

private:

  /** The MUC client we use to access channels.  */
  MucClient& client;

  /** Closure called when a stop is requested.  */
  std::function<void ()> requestStop;

  /**
   * Returns the MsgChannel for a given channel ID.  This convenience
   * method handles the conversion to uint256, error checking, and verification
   * that the returned channel is not null.  It throws JSON-RPC errors.
   */
  MsgChannel& GetChannel (const std::string& hexId);

public:

  explicit RealServer (MucClient& c, jsonrpc::AbstractServerConnector& conn,
                       const std::function<void ()>& s)
    : BroadcastRpcServerStub(conn), client(c), requestStop(s)
  {}

  void send (const std::string& channel, const std::string& message) override;
  Json::Value getseq (const std::string& channel) override;
  Json::Value receive (const std::string& channel, int fromseq) override;

  void stop () override;

};

MsgChannel&
RealServer::GetChannel (const std::string& hexId)
{
  xaya::uint256 id;
  if (!id.FromHex (hexId))
    throw jsonrpc::JsonRpcException (jsonrpc::Errors::ERROR_RPC_INVALID_PARAMS,
                                     "invalid uint256: " + hexId);

  auto* channel = client.GetChannel<MsgChannel> (id);
  if (channel == nullptr)
    throw jsonrpc::JsonRpcException (jsonrpc::Errors::ERROR_RPC_INTERNAL_ERROR,
                                     "failed to access channel, disconnected?");

  return *channel;
}

void
RealServer::send (const std::string& channel, const std::string& message)
{
  std::string decoded;
  if (!xaya::DecodeBase64 (message, decoded))
    {
      LOG (WARNING) << "Failed to decode base64, ignoring message: " << message;
      return;
    }

  GetChannel (channel).Send (decoded);
}

Json::Value
RealServer::getseq (const std::string& channel)
{
  const size_t num = GetChannel (channel).GetSequenceNumber ();

  Json::Value res(Json::objectValue);
  res["seq"] = static_cast<Json::Int64> (num);

  return res;
}

Json::Value
RealServer::receive (const std::string& channel, const int fromseq)
{
  size_t seq = fromseq;
  const auto msg = GetChannel (channel).Receive (seq);

  Json::Value msgArr(Json::arrayValue);
  for (const auto& m : msg)
    msgArr.append (xaya::EncodeBase64 (m));

  Json::Value res(Json::objectValue);
  res["messages"] = msgArr;
  res["seq"] = static_cast<Json::Int64> (seq);

  return res;
}

void
RealServer::stop ()
{
  if (requestStop)
    requestStop ();
}

/* ************************************************************************** */

/**
 * The JSON-RPC server together with its HTTP server connector.
 */
class FullServer
{

private:

  /** The HTTP server connector.  */
  jsonrpc::HttpServer http;

  /** The actual RPC server.  */
  RealServer rpc;

public:

  FullServer (const int port, const bool onlyLocal,
              MucClient& client, const std::function<void ()>& requestStop)
    : http(port), rpc(client, http, requestStop)
  {
    if (onlyLocal)
      http.BindLocalhost ();
    http.StartListening ();
  }

  ~FullServer ()
  {
    http.StopListening ();
  }

};

} // anonymous namespace

/* ************************************************************************** */

/**
 * The main implementation of the RPC server.  This class is hidden from
 * the header to make the implementation (and dependencies, like MucClient)
 * opaque.
 */
class RpcServer::Impl
{

private:

  /** The underlying XMPP broadcast client.  */
  RpcMucClient client;

  /** If the server is started, the refresher used on the MucClient.  */
  std::unique_ptr<MucClient::Refresher> refresher;

  /**
   * The actual JSON-RPC server.  This is only created when the server
   * is started, and destroyed when it is stopped.  This allows us to set
   * the listening port in Start.
   */
  std::unique_ptr<FullServer> server;

  /**
   * If the server is started, we also set up a thread that just waits in
   * a loop until the server requests to be shut down, and then handles
   * the shutdown.
   */
  std::unique_ptr<std::thread> shutDownWaiter;

  /** Set to true if a server shutdown is requested.  */
  bool shouldStop;

  /** Lock for the shouldStop flag and condition variable.  */
  std::mutex mutStop;

  /** Signalled when shouldStop is set.  */
  std::condition_variable cvStop;

  /**
   * Requests to stop the server.  This method only flips the flag and
   * returns immediately, without waiting for the shutdown to complete.
   */
  void RequestStop ();

  friend class RpcServer;

public:

  explicit Impl (const std::string& gameId,
                 const std::string& jid, const std::string& password,
                 const std::string& mucServer)
    : client(gameId, jid, password, mucServer)
  {}

  ~Impl ()
  {
    /* Before the Impl instance is destroyed, the server run by it should
       be stopped properly.  The destructor of RpcServer takes care of that.  */
    CHECK (refresher == nullptr);
    CHECK (server == nullptr);
    CHECK (shutDownWaiter == nullptr);
  }

};

RpcServer::RpcServer (const std::string& gameId,
                      const std::string& jid, const std::string& password,
                      const std::string& mucServer)
{
  impl = std::make_unique<Impl> (gameId, jid, password, mucServer);
}

RpcServer::~RpcServer ()
{
  /* Make sure to clean up any running parts before destructing impl.  */
  Stop ();
}

void
RpcServer::Impl::RequestStop ()
{
  LOG (INFO) << "Requesting server shutdown...";
  std::lock_guard<std::mutex> lock(mutStop);
  shouldStop = true;
  cvStop.notify_all ();
}

void
RpcServer::SetRootCA (const std::string& path)
{
  CHECK (impl->server == nullptr) << "Server is already started";
  impl->client.SetRootCA (path);
}

void
RpcServer::Start (const int port, const bool onlyLocal)
{
  CHECK (impl->server == nullptr) << "Server is already started";
  LOG (INFO) << "Starting RPC server on port " << port;
  
  if (!impl->client.Connect ())
    LOG (WARNING) << "Failed with initial client connect, will keep trying";
  impl->refresher = std::make_unique<MucClient::Refresher> (impl->client);

  impl->shouldStop = false;
  impl->server = std::make_unique<FullServer> (
      port, onlyLocal, impl->client,
      [this] ()
        {
          impl->RequestStop ();
        });
  impl->shutDownWaiter = std::make_unique<std::thread> ([this] ()
    {
      std::unique_lock<std::mutex> lock(impl->mutStop);
      while (!impl->shouldStop)
        impl->cvStop.wait (lock);
      impl->server.reset ();
      impl->refresher.reset ();
      impl->client.Disconnect ();
    });
}

void
RpcServer::Stop ()
{
  impl->RequestStop ();
  Wait ();
}

void
RpcServer::Wait ()
{
  if (impl->shutDownWaiter != nullptr)
    {
      impl->shutDownWaiter->join ();
      impl->shutDownWaiter.reset ();
    }
}

/* ************************************************************************** */

} // namespace xmppbroadcast
