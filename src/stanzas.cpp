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

#include <charon/xmldata.hpp>

#include <glog/logging.h>

#include <memory>

namespace xmppbroadcast
{

constexpr const char* MessageStanza::TAG;

MessageStanza::MessageStanza ()
  : StanzaExtension(EXT_TYPE), valid(false)
{}

MessageStanza::MessageStanza (const std::string& d)
  : StanzaExtension(EXT_TYPE), data(d), valid(true)
{}

MessageStanza::MessageStanza (const gloox::Tag& t)
  : StanzaExtension(EXT_TYPE)
{
  valid = charon::DecodeXmlPayload (t, data);
}

const std::string&
MessageStanza::filterString () const
{
  static const std::string filter
      = std::string ("/*/msg[@xmlns='") + XMLNS + "']";
  return filter;
}

gloox::StanzaExtension*
MessageStanza::newInstance (const gloox::Tag* t) const
{
  return new MessageStanza (*t);
}

gloox::StanzaExtension*
MessageStanza::clone () const
{
  auto res = std::make_unique<MessageStanza> ();
  res->data = data;
  res->valid = valid;
  return res.release ();
}

gloox::Tag*
MessageStanza::tag () const
{
  CHECK (IsValid ()) << "Trying to serialise an invalid stanza";

  auto res = charon::EncodeXmlPayload (TAG, data);
  res->setXmlns (XMLNS);

  return res.release ();
}

} // namespace xmppbroadcast
