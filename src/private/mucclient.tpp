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

/* Template implementation for mucclient.hpp.  */

#include <glog/logging.h>

namespace xmppbroadcast
{

template <typename C>
  C*
  MucClient::GetChannel (const xaya::uint256& id)
{
  if (!IsConnected ())
    return nullptr;

  const auto jid = GetRoomJid (id);
  std::lock_guard<std::mutex> lock(mut);

  const auto mit = channels.find (jid);
  if (mit != channels.end ())
    {
      if (mit->second->IsActive ())
        {
          auto* res = dynamic_cast<C*> (mit->second.get ());
          CHECK (res != nullptr);
          return res;
        }

      channels.erase (mit);
      return nullptr;
    }

  auto newChannel = CreateChannel (jid);
  auto* res = dynamic_cast<C*> (newChannel.get ());
  CHECK (res != nullptr);
  channels.emplace (jid, std::move (newChannel));
  return res;
}

} // namespace xmppbroadcast
