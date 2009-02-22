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
 * preferences.h
 * Interface for the Preferences class - this allows storing user preferences /
 * settings.
 * Copyright (C) 2005-2006  Simon Newton
 */

#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <map>
#include <vector>
#include <string>

using namespace std;

namespace lla {

/*
 * The abstract Preferences class
 */
class Preferences {
  public:
    Preferences(const string name): m_preference_name(name) {}
    virtual ~Preferences() {}

    virtual int Load() = 0;
    virtual int Save() const = 0;

    virtual int SetValue(const string &key, const string &value) = 0;
    virtual int SetMultipleValue(const string &key, const string &value) = 0;

    virtual string GetValue(const string &key) const = 0;
    virtual vector<string> GetMultipleValue(const string &key) const = 0;

  protected:
    string m_preference_name;
  private:
    Preferences(const Preferences&);
    Preferences& operator=(const Preferences&);

};


/*
 * A PreferencesFactory creates preferences objects
 */
class PreferencesFactory {
  public:
    PreferencesFactory() {}
    virtual ~PreferencesFactory() {}
    virtual Preferences *NewPreference(const string &name) = 0;
};


/*
 * MemoryPreferences just stores the preferences in memory. Useful for testing.
 */
class MemoryPreferences: public Preferences {
  public:
    MemoryPreferences(const string name): Preferences(name) {}
    virtual ~MemoryPreferences();
    virtual int Load() { return 0; }
    virtual int Save() const { return 0; }
    virtual int SetValue(const string &key, const string &value);
    virtual int SetMultipleValue(const string &key, const string &value);
    virtual string GetValue(const string &key) const;
    virtual vector<string> GetMultipleValue(const string &key) const;

  protected:
    multimap<string, string> m_pref_map;
};


class MemoryPreferencesFactory: public PreferencesFactory {
  public:
    MemoryPreferences *NewPreference(const string &name) {
      return new MemoryPreferences(name);
    }
};


/*
 * FilePreferences uses one file per namespace
 */
class FileBackedPreferences: public MemoryPreferences {
  public:
    FileBackedPreferences(const string name): MemoryPreferences(name) {}
    virtual int Load();
    virtual int Save() const;

  private:
    int ChangeDir() const;
    char *StrTrim(char *str);
};

class FileBackedPreferencesFactory: public PreferencesFactory {
  public:
    FileBackedPreferences *NewPreference(const string &name) {
      return new FileBackedPreferences(name);
    }
};



} //lla
#endif