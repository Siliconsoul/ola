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
 * stageprofiplugin.h
 * Interface for the stageprofi plugin class
 * Copyright (C) 2006-2007 Simon Newton
 */

#ifndef STAGEPROFIPLUGIN_H
#define STAGEPROFIPLUGIN_H

#include <vector>
#include <string>

#include <llad/plugin.h>
#include <llad/fdmanager.h>
#include <lla/plugin_id.h>

using namespace std;

class StageProfiDevice;

class StageProfiPlugin : public Plugin, public FDManager {

  public:
    StageProfiPlugin(const PluginAdaptor *pa, lla_plugin_id id):
      Plugin(pa, id) {}
    ~StageProfiPlugin() {}

    string get_name() const { return PLUGIN_NAME; }
    string get_desc() const;
    int fd_error(int error, FDListener *listener);

  protected:
    string pref_suffix() const { return PLUGIN_PREFIX; }

  private:
    int start_hook();
    int stop_hook();
    int set_default_prefs();

    vector<StageProfiDevice *>  m_devices;  // list of our devices

    static const string STAGEPROFI_DEVICE_PATH;
    static const string STAGEPROFI_DEVICE_NAME;
    static const string PLUGIN_NAME;
    static const string PLUGIN_PREFIX;
};

#endif