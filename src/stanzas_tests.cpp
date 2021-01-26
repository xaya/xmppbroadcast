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

#include "private/stanzas.hpp"

#include <gtest/gtest.h>

namespace xmppbroadcast
{
namespace
{

using StanzasTests = testing::Test;

TEST_F (StanzasTests, InvalidMessage)
{
  gloox::Tag t(MessageStanza::TAG);
  t.setXmlns (XMLNS);
  t.addChild (new gloox::Tag ("invalid", "foo"));

  const MessageStanza s(t);
  EXPECT_FALSE (s.IsValid ());
}

TEST_F (StanzasTests, MessageRoundtrip)
{
  const MessageStanza original("payload");
  ASSERT_TRUE (original.IsValid ());

  std::unique_ptr<gloox::Tag> tag(original.tag ());
  ASSERT_EQ (tag->name (), MessageStanza::TAG);
  ASSERT_EQ (tag->xmlns (), XMLNS);

  std::unique_ptr<gloox::StanzaExtension> parsed(
      original.newInstance (tag.get ()));
  std::unique_ptr<MessageStanza> cloned(
      dynamic_cast<MessageStanza*> (parsed->clone ()));

  ASSERT_NE (cloned, nullptr);
  ASSERT_TRUE (cloned->IsValid ());
  ASSERT_EQ (cloned->GetData (), original.GetData ());
}

} // anonymous namespace
} // namespace xmppbroadcast
