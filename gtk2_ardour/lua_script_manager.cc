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

#include "gtkmm2ext/utils.h"

#include "lua_script_manager.h"
#include "luainstance.h"
#include "script_selector.h"
#include "i18n.h"

using namespace std;
using namespace Gtk;
using namespace ARDOUR;

LuaScriptManager::LuaScriptManager ()
	: ArdourWindow (_("Script Manager"))
	, set_button (_("Add/Set"))
	, del_button (_("Remove"))
	, edit_button (_("Edit"))
	, call_button (_("Call"))
{

	_store = ListStore::create (_model);
	_view.set_model (_store);
	_view.append_column (_("Action"), _model.action);
	_view.append_column (_("Name"), _model.name);
	_view.get_column(0)->set_resizable (true);
	_view.get_column(0)->set_expand (true);


	Gtk::HBox* edit_box = manage (new Gtk::HBox);
	edit_box->set_spacing(3);

	edit_box->pack_start (set_button, true, true);
	edit_box->pack_start (del_button, true, true);
	edit_box->pack_start (edit_button, true, true);
	edit_box->pack_start (call_button, true, true);

	vbox.pack_start (_view, false, false);
	vbox.pack_start (*edit_box, false, false);
	add (vbox);
	vbox.show_all ();

	set_button.signal_clicked().connect (sigc::mem_fun(*this, &LuaScriptManager::set_btn_clicked));
	del_button.signal_clicked().connect (sigc::mem_fun(*this, &LuaScriptManager::del_btn_clicked));
	edit_button.signal_clicked().connect (sigc::mem_fun(*this, &LuaScriptManager::edit_btn_clicked));
	call_button.signal_clicked().connect (sigc::mem_fun(*this, &LuaScriptManager::call_btn_clicked));

	_view.get_selection()->signal_changed().connect (sigc::mem_fun (*this, &LuaScriptManager::selection_changed));

	LuaInstance::instance()->ActionChanged.connect (sigc::mem_fun (*this, &LuaScriptManager::set_script_action_name));

	setup_list ();
	selection_changed ();
}

void
LuaScriptManager::setup_list ()
{
	LuaInstance *li = LuaInstance::instance();
	for (int i = 0; i < 9; ++i) {
		std::string name;
		TreeModel::Row r = *_store->append ();
		r[_model.id] = i;
		r[_model.action] = string_compose (_("Action %1"), i + 1);
		if (li->lua_action_name (i, name)) {
			r[_model.name] = name;
			r[_model.enabled] = true;
		} else {
			r[_model.name] = _("Unset");
			r[_model.enabled] = false;
		}
	}
}

void
LuaScriptManager::selection_changed ()
{
	TreeModel::Row row = *(_view.get_selection()->get_selected());
	if (row) {
		set_button.set_sensitive (true);
	}
	else {
		set_button.set_sensitive (false);
	}

	if (row && row[_model.enabled]) {
		del_button.set_sensitive (true);
		edit_button.set_sensitive (false); // TODO
		call_button.set_sensitive (true);
	} else {
		del_button.set_sensitive (false);
		edit_button.set_sensitive (false);
		call_button.set_sensitive (false);
	}
}

void
LuaScriptManager::set_btn_clicked ()
{
	LuaScriptInfoPtr spi;
	ScriptSelector ss ("Add Lua Action Script", LuaScriptInfo::EditorAction);
	switch (ss.run ()) {
		case Gtk::RESPONSE_ACCEPT:
			spi = ss.script();
			break;
		default:
			return;
	}
	std::string script = "";

	try {
		script = Glib::file_get_contents (spi->path);
	} catch (Glib::FileError e) {
		string msg = string_compose (_("Cannot read script '%1': %2"), spi->path, e.what());
		MessageDialog am (msg);
		am.run ();
		return;
	}

	LuaInstance *li = LuaInstance::instance();
	LuaScriptParamList lsp = LuaScripting::script_params (spi, "action_params");

	// get all names -> move to LuaInstance ?
	std::vector<std::string> reg; // = li->get_all_registered_name ();
	for (int i = 0; i < 9; ++i) {
		std::string name;
		if (li->lua_action_name (i, name)) {
			reg.push_back (name);
		}
	}

	ScriptParameterDialog spd (_("Set Script Parameters"), spi, reg, lsp);
	switch (spd.run ()) {
		case Gtk::RESPONSE_ACCEPT:
			break;
		default:
			return;
	}

	TreeModel::Row row = *(_view.get_selection()->get_selected());
	assert (row);
	if (!li->set_lua_action (row[_model.id], spd.name(), script, lsp)) {
		// error
	}
}

void
LuaScriptManager::del_btn_clicked ()
{
	TreeModel::Row row = *(_view.get_selection()->get_selected());
	assert (row);
	LuaInstance *li = LuaInstance::instance();
	if (!li->remove_lua_action (row[_model.id])) {
		// error
	}
}

void
LuaScriptManager::call_btn_clicked ()
{
	TreeModel::Row row = *(_view.get_selection()->get_selected());
	assert (row && row[_model.enabled]);
	LuaInstance *li = LuaInstance::instance();
	li->call_action (row[_model.id]);
}

void
LuaScriptManager::edit_btn_clicked ()
{
	TreeModel::Row row = *(_view.get_selection()->get_selected());
	assert (row);
	int id = row[_model.id];
	LuaInstance *li = LuaInstance::instance();
	std::string name, script;
	LuaScriptParamList args;
	if (!li->lua_action (id, name, script, args)) {
		return;
	}

	// TODO text-editor window, update script directly

	if (!LuaScripting::try_compile (script, args)) {
		// compilation failed, keep editing
		return;
	}

	if (li->set_lua_action (id, name, script, args)) {
		// OK
	} else {
		// load failed,  keep editing..
	}
	selection_changed ();
}

void
LuaScriptManager::set_script_action_name (int i, const std::string& name)
{
	typedef Gtk::TreeModel::Children type_children;
	type_children children = _store->children();
	for(type_children::iterator iter = children.begin(); iter != children.end(); ++iter) {
		Gtk::TreeModel::Row row = *iter;
		if (row[_model.id] == i) {
			if (name.empty()) {
				row[_model.enabled] = false;
				row[_model.name] = _("Unset");
			} else {
				row[_model.enabled] = true;
				row[_model.name] = name;
			}
			break;
		}
	}
	selection_changed ();
}

bool
LuaScriptManager::on_delete_event (GdkEventAny*)
{
	return false;
}

void
LuaScriptManager::session_going_away ()
{
	ArdourWindow::session_going_away ();
	hide_all();
}
