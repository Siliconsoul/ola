/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Message.cpp
 * Holds the metadata (schema) for a Message.
 * Copyright (C) 2011 Simon Newton
 */

#include <ola/messaging/Message.h>
#include <vector>

namespace ola {
namespace messaging {

using std::vector;


Message::~Message() {
  vector<const MessageFieldInterface*>::const_iterator iter = m_fields.begin();
  for (; iter != m_fields.end(); ++iter)
    delete *iter;
}


void Message::Accept(MessageVisitor &visitor) const {
  vector<const MessageFieldInterface*>::const_iterator iter = m_fields.begin();
  for (; iter != m_fields.end(); ++iter)
    (*iter)->Accept(visitor);
}


GroupMessageField::~GroupMessageField() {
  vector<const MessageFieldInterface*>::const_iterator iter = m_fields.begin();
  for (; iter != m_fields.end(); ++iter)
    delete *iter;
}


void GroupMessageField::Accept(MessageVisitor &visitor) const {
  visitor.Visit(this);
  vector<const MessageFieldInterface*>::const_iterator iter = m_fields.begin();
  for (; iter != m_fields.end(); ++iter)
    (*iter)->Accept(visitor);
  visitor.PostVisit(this);
}
}  // messaging
}  // ola
