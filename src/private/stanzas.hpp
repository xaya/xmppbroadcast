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

#ifndef XMPPBROADCAST_STANZAS_HPP
#define XMPPBROADCAST_STANZAS_HPP

#include <gloox/stanzaextension.h>
#include <gloox/tag.h>

#include <string>

namespace xmppbroadcast
{

/** XML namespace for xmppbroadcast's stanza tags.  */
constexpr const char* XMLNS = "https://xaya.io/xmppbroadcast";

/**
 * A gloox stanza extension that wraps our messages into the <msg> tags.
 */
class MessageStanza : public gloox::StanzaExtension
{

private:

  /** The payload data (which is the game-channel message string).  */
  std::string data;

  /** Set to false if this is invalid, e.g. failed to parse.  */
  bool valid;

public:

  /** The tag name for this stanza.  */
  static constexpr const char* TAG = "msg";

  /** Extension type for this stanza.  */
  static constexpr int EXT_TYPE = gloox::ExtUser + 1;

  /**
   * Constructs an empty, invalid instance.  This is used e.g. for the factory
   * objects needed by gloox.
   */
  MessageStanza ();

  /**
   * Constructs an instance with the given underlying payload.
   */
  explicit MessageStanza (const std::string& d);

  /**
   * Constructs an instance from a given tag.
   */
  explicit MessageStanza (const gloox::Tag& t);

  bool
  IsValid () const
  {
    return valid;
  }

  const std::string&
  GetData () const
  {
    return data;
  }

  const std::string& filterString () const override;
  gloox::StanzaExtension* newInstance (const gloox::Tag* tag) const override;
  gloox::StanzaExtension* clone () const override;
  gloox::Tag* tag () const override;

};

} // namespace xmppbroadcast

#endif // XMPPBROADCAST_STANZAS_HPP
