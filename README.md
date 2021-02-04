# XMPP Broadcast

This project implements a [broadcast
method](https://github.com/xaya/libxayagame/blob/master/gamechannel/broadcast.hpp)
for [game channels](https://github.com/xaya/libxayagame/tree/master/gamechannel)
that uses [XMPP](https://xmpp.org/) for the underlying communication.

## Usage

Game-channel applications can use the XMPP broadcast in two ways:

1. As a library:  The project provides an [`XmppBroadcast`
  class](https://github.com/xaya/xmppbroadcast/blob/master/src/xmppbroadcast.hpp),
  which directly implements an
  [`OffChainBroadcast`](https://github.com/xaya/libxayagame/blob/master/gamechannel/broadcast.hpp)
   that can be used when interacting with the game-channel library.

1. As a local RPC server:  It is also possible to launch a local
   RPC server (through the library's [`RpcServer`
   class](https://github.com/xaya/xmppbroadcast/blob/master/src/rpcserver.hpp)
   or the stand-alone binary `xmpp-broadcast-rpc-server`),
   which the game channel can connect to using the standard
   [`RpcBroadcast`](https://github.com/xaya/libxayagame/blob/master/gamechannel/rpcbroadcast.hpp).
   This allows to "XMPP-ify" existing applications in a modular way.

## Details

The communications for each channel are done in a temporary MUC channel
on the XMPP server, named as `gameid_channelid`.  This allows all participants
to join the channel as needed, and send / receive messages through it.
Each channel message is encoded into a custom stanza tag of this form:

    <msg xmlns="https://xaya.io/xmppbroadcast">
      encoded payload
    </msg>

The tag (especially the encoded payload) is created with [Charon's `xmldata`
library](https://github.com/xaya/charon/blob/master/src/xmldata.hpp).

On the XMPP side, the encoded payload corresponds directly to the
raw broadcast data from the game-channels library.  In the RPC server,
this data is additionally base64-encoded on the side of the RPC client
(but not when sent to XMPP).
