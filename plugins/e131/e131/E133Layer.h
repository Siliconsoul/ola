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
 * E133Layer.h
 * Interface for the E133Layer class, this abstracts the encapsulation and
 * sending of DMP PDUs contained within E133PDUs as well as the setting of
 * the DMP inflator inflator.
 * Copyright (C) 2011 Simon Newton
 */

#ifndef PLUGINS_E131_E131_E133LAYER_H_
#define PLUGINS_E131_E131_E133LAYER_H_

#include "ola/network/IPV4Address.h"
#include "plugins/e131/e131/ACNPort.h"
#include "plugins/e131/e131/E133Header.h"
#include "plugins/e131/e131/E133Inflator.h"

namespace ola {
namespace plugin {
namespace e131 {

class E133Layer {
  public:
    explicit E133Layer(class RootLayer *root_layer);
    ~E133Layer() {}

    bool SendDMP(const E133Header &header,
                 const class DMPPDU *pdu,
                 const ola::network::IPV4Address &destination,
                 uint16_t destination_port = ACN_PORT);
    bool SetInflator(class DMPE133Inflator *inflator);

  private:
    class RootLayer *m_root_layer;
    E133Inflator m_e133_inflator;

    E133Layer(const E133Layer&);
    E133Layer& operator=(const E133Layer&);
};
}  // e131
}  // plugin
}  // ola
#endif  // PLUGINS_E131_E131_E133LAYER_H_
