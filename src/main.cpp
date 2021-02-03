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

#include "config.h"

#include "rpcserver.hpp"

#include <gflags/gflags.h>
#include <glog/logging.h>

#include <cstdlib>
#include <iostream>

namespace
{

DEFINE_string (game_id, "", "game ID for which to run broadcasts");

DEFINE_string (jid, "", "JID for the XMPP connection");
DEFINE_string (password, "", "password for the XMPP connection");
DEFINE_string (muc, "", "XMPP MUC service JID");

DEFINE_int32 (port, 0, "port for the JSON-RPC broadcast server");
DEFINE_bool (listen_locally, true,
             "whether the RPC server should listen locally");

/**
 * Exception thrown for invalid usage.
 */
class UsageError : public std::runtime_error
{

public:

  explicit UsageError (const std::string& msg)
    : std::runtime_error(msg)
  {}

};

} // anonymous namespace

int
main (int argc, char** argv)
{
  google::InitGoogleLogging (argv[0]);

  gflags::SetUsageMessage ("Run an XMPP broadcast RPC server");
  gflags::SetVersionString (PACKAGE_VERSION);
  gflags::ParseCommandLineFlags (&argc, &argv, true);

  try
    {
      if (FLAGS_game_id.empty ())
        throw UsageError ("--game_id must be set");
      if (FLAGS_jid.empty ())
        throw UsageError ("--jid must be set");
      if (FLAGS_password.empty ())
        throw UsageError ("--password must be set");
      if (FLAGS_muc.empty ())
        throw UsageError ("--muc must be set");
      if (FLAGS_port == 0)
        throw UsageError ("--port must be set");

      xmppbroadcast::RpcServer srv(FLAGS_game_id,
                                   FLAGS_jid, FLAGS_password,
                                   FLAGS_muc);
      srv.Start (FLAGS_port, FLAGS_listen_locally);
      srv.Wait ();

      return EXIT_SUCCESS;
    }
  catch (const UsageError& exc)
    {
      std::cerr << "Error: " << exc.what () << std::endl;
      return EXIT_FAILURE;
    }
  catch (const std::exception& exc)
    {
      LOG (ERROR) << exc.what ();
      std::cerr << "Error: " << exc.what () << std::endl;
      return EXIT_FAILURE;
    }
}
