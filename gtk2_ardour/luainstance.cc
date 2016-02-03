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

#include "gtkmm2ext/gui_thread.h"

#include "ardour_ui.h"
#include "public_editor.h"
#include "region_selection.h"
#include "luainstance.h"

#include "i18n.h"

#define xstr(s) stringify(s)
#define stringify(s) #s

using namespace ARDOUR;

void
LuaInstance::register_classes (lua_State* L)
{
	LuaBindings::stddef (L);
	LuaBindings::common (L);
	LuaBindings::session (L);

	luabridge::getGlobalNamespace (L)
		.beginNamespace ("ARDOUR")
		.beginClass <RegionSelection> ("RegionSelection")
		.addFunction ("clear_all", &RegionSelection::clear_all)
		.addFunction ("start", &RegionSelection::start)
		.addFunction ("end_frame", &RegionSelection::end_frame)
		.addFunction ("n_midi_regions", &RegionSelection::n_midi_regions)
		.endClass ()

		.beginClass <PublicEditor> ("Editor")
		.addFunction ("undo", &PublicEditor::undo)
		.addFunction ("redo", &PublicEditor::redo)
		.addFunction ("play_selection", &PublicEditor::play_selection)
		.addFunction ("play_with_preroll", &PublicEditor::play_with_preroll)
		.addFunction ("maybe_locate_with_edit_preroll", &PublicEditor::maybe_locate_with_edit_preroll)
		.addFunction ("add_location_from_playhead_cursor", &PublicEditor::add_location_from_playhead_cursor)
		.addFunction ("goto_nth_marker", &PublicEditor::goto_nth_marker)
		.addFunction ("set_show_measures", &PublicEditor::set_show_measures)
		.addFunction ("mouse_add_new_marker", &PublicEditor::mouse_add_new_marker)
		.addFunction ("split_regions_at", &PublicEditor::split_regions_at)
		.addFunction ("maximise_editing_space", &PublicEditor::maximise_editing_space)
		.addFunction ("restore_editing_space", &PublicEditor::restore_editing_space)
		.addFunction ("get_regions_from_selection_and_mouse", &PublicEditor::get_regions_from_selection_and_mouse)
		.addFunction ("show_measures", &PublicEditor::show_measures)
		.addFunction ("set_zoom_focus", &PublicEditor::set_zoom_focus)
		.addFunction ("do_import", &PublicEditor::do_import)
		.addFunction ("do_embed", &PublicEditor::do_embed)
		.endClass ()
		.endNamespace ();

	luabridge::getGlobalNamespace (L)
		.beginNamespace ("Editing")

#undef ZOOMFOCUS
#undef SNAPTYPE
#undef SNAPMODE
#undef MOUSEMODE
#undef DISPLAYCONTROL
#undef IMPORTMODE
#undef IMPORTPOSITION
#undef IMPORTDISPOSITION

#define ZOOMFOCUS(NAME) .addConst (stringify(NAME), (Editing::ZoomFocus)Editing::NAME)
#define SNAPTYPE(NAME) .addConst (stringify(NAME), (Editing::SnapType)Editing::NAME)
#define SNAPMODE(NAME) .addConst (stringify(NAME), (Editing::SnapMode)Editing::NAME)
#define MOUSEMODE(NAME) .addConst (stringify(NAME), (Editing::MouseMode)Editing::NAME)
#define DISPLAYCONTROL(NAME) .addConst (stringify(NAME), (Editing::DisplayControl)Editing::NAME)
#define IMPORTMODE(NAME) .addConst (stringify(NAME), (Editing::ImportMode)Editing::NAME)
#define IMPORTPOSITION(NAME) .addConst (stringify(NAME), (Editing::ImportPosition)Editing::NAME)
#define IMPORTDISPOSITION(NAME) .addConst (stringify(NAME), (Editing::ImportDisposition)Editing::NAME)
	#include "editing_syms.h"
		.endNamespace ();
}

#undef xstr
#undef stringify

////////////////////////////////////////////////////////////////////////////////

using namespace ARDOUR;
using namespace ARDOUR_UI_UTILS;
using namespace PBD;
using namespace std;

#ifndef NDEBUG
static void _lua_print (std::string s) {
	std::cout << "LuaInstance: " << s << "\n";
}
#endif

LuaInstance* LuaInstance::_instance = 0;

LuaInstance*
LuaInstance::instance ()
{
	if (!_instance) {
		_instance  = new LuaInstance;
	}

	return _instance;
}

LuaInstance::LuaInstance ()
{
#ifndef NDEBUG
	lua.Print.connect (&_lua_print);
#endif
	init ();

	LuaScriptParamList args;
}

LuaInstance::~LuaInstance ()
{
}

void
LuaInstance::init ()
{
	lua.do_command (
			"function ScriptManager ()"
			"  local self = { scripts = {}, instances = {} }"
			""
			"  local remove = function (id)"
			"   self.scripts[id] = nil"
			"   self.instances[id] = nil"
			"  end"
			""
			"  local addinternal = function (i, n, s, f, a)"
			"   assert(type(i) == 'number', 'id must be numeric')"
			"   assert(type(n) == 'string', 'Name must be string')"
			"   assert(type(s) == 'string', 'Script must be string')"
			"   assert(type(f) == 'function', 'Factory is a not a function')"
			"   assert(type(a) == 'table' or type(a) == 'nil', 'Given argument is invalid')"
			"   self.scripts[i] = { ['n'] = n, ['s'] = s, ['f'] = f, ['a'] = a }"
			"   local env = _ENV;  env.f = nil env.debug = nil os.exit = nil require = nil dofile = nil loadfile = nil package = nil"
			"   self.instances[i] = load (string.dump(f, true), nil, nil, env)(a)"
			"  end"
			""
			"  local call = function (id)"
			"   if type(self.instances[id]) == 'function' then"
			"     local status, err = pcall (self.instances[id])"
			"     if not status then"
			"       print ('action \"'.. id .. '\": ', err)" // error out
			"       remove (id)"
			"     end"
			"   end"
			"   collectgarbage()"
			"  end"
			""
			"  local add = function (i, n, s, b, a)"
			"   assert(type(b) == 'string', 'ByteCode must be string')"
			"   load (b)()" // assigns f
			"   assert(type(f) == 'string', 'Assigned ByteCode must be string')"
			"   addinternal (i, n, s, load(f), a)"
			"  end"
			""
			"  local get = function (id)"
			"   if type(self.scripts[id]) == 'table' then"
			"    return { ['name'] = self.scripts[id]['n'],"
			"             ['script'] = self.scripts[id]['s'],"
			"             ['args'] = self.scripts[id]['a'] }"
			"   end"
			"   return nil"
			"  end"
			""
			"  local function basic_serialize (o)"
			"    if type(o) == \"number\" then"
			"     return tostring(o)"
			"    else"
			"     return string.format(\"%q\", o)"
			"    end"
			"  end"
			""
			"  local function serialize (name, value)"
			"   local rv = name .. ' = '"
			"   collectgarbage()"
			"   if type(value) == \"number\" or type(value) == \"string\" or type(value) == \"nil\" then"
			"    return rv .. basic_serialize(value) .. ' '"
			"   elseif type(value) == \"table\" then"
			"    rv = rv .. '{} '"
			"    for k,v in pairs(value) do"
			"     local fieldname = string.format(\"%s[%s]\", name, basic_serialize(k))"
			"     rv = rv .. serialize(fieldname, v) .. ' '"
			"     collectgarbage()" // string concatenation allocates a new string
			"    end"
			"    return rv;"
			"   elseif type(value) == \"function\" then"
			"     return rv .. string.format(\"%q\", string.dump(value, true))"
			"   else"
			"    error('cannot save a ' .. type(value))"
			"   end"
			"  end"
			""
			""
			"  local save = function ()"
			"   return (serialize('scripts', self.scripts))"
			"  end"
			""
			"  local restore = function (state)"
			"   self.scripts = {}"
			"   load (state)()"
			"   for i, s in pairs (scripts) do"
			"    addinternal (i, s['n'], s['s'], load(s['f']), s['a'])"
			"   end"
			"  end"
			""
			" return { call = call, add = add, remove = remove,"
		  "          get = get, restore = restore, save = save}"
			" end"
			" "
			" manager = ScriptManager ()"
			" ScriptManager = nil"
			);

	lua_State* L = lua.getState();

	try {
		luabridge::LuaRef lua_mgr = luabridge::getGlobal (L, "manager");
		lua.do_command ("manager = nil"); // hide it.
		lua.do_command ("collectgarbage()");

		_lua_add_action = new luabridge::LuaRef(lua_mgr["add"]);
		_lua_del_action = new luabridge::LuaRef(lua_mgr["remove"]);
		_lua_get_action = new luabridge::LuaRef(lua_mgr["get"]);
		_lua_call_action = new luabridge::LuaRef(lua_mgr["call"]);
		_lua_save = new luabridge::LuaRef(lua_mgr["save"]);
		_lua_load = new luabridge::LuaRef(lua_mgr["restore"]);

	} catch (luabridge::LuaException const& e) {
		fatal << string_compose (_("programming error: %1"),
				X_("Failed to setup Lua interpreter"))
			<< endmsg;
		abort(); /*NOTREACHED*/
	}

	register_classes (L);

	luabridge::push <PublicEditor *> (L, &PublicEditor::instance());
	lua_setglobal (L, "Editor");
}

void LuaInstance::set_session (Session* s)
{
	SessionHandlePtr::set_session (s);
	if (!_session) {
		return;
	}

	lua_State* L = lua.getState();
	LuaBindings::set_session (L, _session);
}

void
LuaInstance::session_going_away ()
{
	ENSURE_GUI_THREAD (*this, &LuaInstance::session_going_away);
	lua.do_command ("collectgarbage();");

#if 0 //re-init lua-engine (to drop all references) ??
	luabridge::LuaRef savedstate ((*_lua_save)());
	sta::string saved = savedstate.cast<std::string>();
	lua.reset ();
	init ();
	(*_lua_load)(saved)
#endif

	SessionHandlePtr::session_going_away ();
	_session = 0;

	lua_State* L = lua.getState();
	LuaBindings::set_session (L, _session);
}

int
LuaInstance::set_state (const XMLNode& node)
{
	LocaleGuard lg (X_("C"));
	XMLNode* child;

	if ((child = find_named_node (node, "ActionScript"))) {
		for (XMLNodeList::const_iterator n = child->children ().begin (); n != child->children ().end (); ++n) {
			if (!(*n)->is_content ()) { continue; }
			gsize size;
			guchar* buf = g_base64_decode ((*n)->content ().c_str (), &size);
			try {
				(*_lua_load)(std::string ((const char*)buf, size));
			} catch (luabridge::LuaException const& e) {
				cerr << "LuaException:" << e.what () << endl;
			}
			for (int i = 0; i < 9; ++i) {
				std::string name;
				if (lua_action_name (i, name)) {
					ActionChanged (i, name); /* EMIT SIGNAL */
				}
			}
			g_free (buf);
		}
	}

	return 0;
}

XMLNode&
LuaInstance::get_state ()
{
	LocaleGuard lg (X_("C"));
	std::string saved;
	{
		luabridge::LuaRef savedstate ((*_lua_save)());
		saved = savedstate.cast<std::string>();
	}
	lua.collect_garbage ();

	gchar* b64 = g_base64_encode ((const guchar*)saved.c_str (), saved.size ());
	std::string b64s (b64);
	g_free (b64);

	XMLNode* script_node = new XMLNode (X_("ActionScript"));
	script_node->add_property (X_("lua"), LUA_VERSION);
	script_node->add_content (b64s);
	return *script_node;
}


void
LuaInstance::call_action (const int id)
{
	try {
		(*_lua_call_action)(id + 1);
	} catch (luabridge::LuaException const& e) {
		cerr << "LuaException:" << e.what () << endl;
	}
}

bool
LuaInstance::set_lua_action (
		const int id,
		const std::string& name,
		const std::string& script,
		const LuaScriptParamList& args)
{
	try {
		lua_State* L = lua.getState();
		// get bytcode of factory-function in a sandbox
		// (don't allow scripts to interfere)
		const std::string& bytecode = LuaScripting::get_factory_bytecode (script);
		luabridge::LuaRef tbl_arg (luabridge::newTable(L));
		for (LuaScriptParamList::const_iterator i = args.begin(); i != args.end(); ++i) {
			if ((*i)->optional && !(*i)->is_set) { continue; }
			tbl_arg[(*i)->name] = (*i)->value;
		}
		(*_lua_add_action)(id + 1, name, script, bytecode, tbl_arg);
		ActionChanged (id, name); /* EMIT SIGNAL */
	} catch (luabridge::LuaException const& e) {
		cerr << "LuaException:" << e.what () << endl;
		return false;
	}
	return true;
}

bool
LuaInstance::remove_lua_action (const int id)
{
	try {
		(*_lua_del_action)(id + 1);
	} catch (luabridge::LuaException const& e) {
		cerr << "LuaException:" << e.what () << endl;
		return false;
	}
	ActionChanged (id, ""); /* EMIT SIGNAL */
	return true;
}

bool
LuaInstance::lua_action_name (const int id, std::string& rv)
{
	try {
		luabridge::LuaRef ref ((*_lua_get_action)(id + 1));
		if (ref.isNil()) {
			return false;
		}
		if (ref["name"].isString()) {
			rv = ref["name"].cast<std::string>();
			return true;
		}
		return true;
	} catch (luabridge::LuaException const& e) {
		cerr << "LuaException:" << e.what () << endl;
		return false;
	}
	return false;
}

bool
LuaInstance::lua_action (const int id, std::string& name, std::string& script, LuaScriptParamList& args)
{
	try {
		luabridge::LuaRef ref ((*_lua_get_action)(id + 1));
		if (ref.isNil()) {
			return false;
		}
		if (!ref["name"].isString()) {
			return false;
		}
		if (!ref["script"].isString()) {
			return false;
		}
		if (!ref["args"].isTable()) {
			return false;
		}
		name = ref["name"].cast<std::string>();
		script = ref["script"].cast<std::string>();

		args.clear();
		LuaScriptInfoPtr lsi = LuaScripting::script_info (script);
		if (!lsi) {
			return false;
		}
		args = LuaScripting::script_params (lsi, "action_params");
		for (luabridge::Iterator i (static_cast<luabridge::LuaRef>(ref["args"])); !i.isNil (); ++i) {
			if (!i.key ().isString ()) { assert(0); continue; }
			std::string name = i.key ().cast<std::string> ();
			std::string value = i.value ().cast<std::string> ();
			for (LuaScriptParamList::const_iterator ii = args.begin(); ii != args.end(); ++ii) {
				if ((*ii)->name == name) {
					(*ii)->value = value;
					break;
				}
			}
		}
		return true;
	} catch (luabridge::LuaException const& e) {
		cerr << "LuaException:" << e.what () << endl;
		return false;
	}
	return false;
}
