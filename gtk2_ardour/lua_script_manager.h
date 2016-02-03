/*
 * Copyright (C) 2016 Robin Gareus <robin@gareus.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <gtkmm.h>
#include "ardour/luascripting.h"

#include "ardour_window.h"

class LuaScriptManager : public ArdourWindow
{
public:
	LuaScriptManager ();

protected:
	bool on_delete_event (GdkEventAny*);
	void session_going_away();

private:
	void setup_list ();

	Gtk::Button set_button;
	Gtk::Button del_button;
	Gtk::Button edit_button;
	Gtk::Button call_button;

	void selection_changed ();
	void set_script_action_name (int, const std::string&);

	void set_btn_clicked ();
	void del_btn_clicked ();
	void edit_btn_clicked ();
	void call_btn_clicked ();

	class LuaScriptModelColumns : public Gtk::TreeModelColumnRecord
	{
		public:
			LuaScriptModelColumns ()
			{
				add (id);
				add (action);
				add (name);
				add (enabled);
			}

			Gtk::TreeModelColumn<int> id;
			Gtk::TreeModelColumn<std::string> action;
			Gtk::TreeModelColumn<std::string> name;
			Gtk::TreeModelColumn<bool> enabled;
	};

	Glib::RefPtr<Gtk::ListStore> _store;
	LuaScriptModelColumns _model;
	Gtk::TreeView _view;
	Gtk::VBox vbox;
};
