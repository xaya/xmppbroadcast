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

#ifndef XMPPBROADCAST_XMPPBROADCAST_TESTS_HPP
#define XMPPBROADCAST_XMPPBROADCAST_TESTS_HPP

#include "xmppbroadcast.hpp"

#include "testutils.hpp"

#include <xayautil/uint256.hpp>

namespace xmppbroadcast
{

/**
 * Custom broadcast instance for testing.  It uses our XMPP test accounts,
 * and stores received messages into a ReceivedMessages instance.
 */
class TestXmppBroadcast : public TestBroadcast<XmppBroadcast>
{

public:

  explicit TestXmppBroadcast (const unsigned n, const xaya::uint256& id);

};

} // namespace xmppbroadcast

#endif // XMPPBROADCAST_XMPPBROADCAST_TESTS_HPP
