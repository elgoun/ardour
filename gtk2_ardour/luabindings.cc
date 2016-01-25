#include "ardour/luabindings.h"

#include "ardour_ui.h"
#include "public_editor.h"
#include "luabindings.h"

#include "LuaBridge/LuaBridge.h"

#define xstr(s) stringify(s)
#define stringify(s) #s

using namespace ARDOUR;

////////////////////////////////////////////////////////////////////////////////

void
ArdourLuaScripting::register_classes (lua_State* L)
{
	LuaBindings::stddef (L);
	LuaBindings::rtsafe_common (L);
	LuaBindings::libardour (L);

	luabridge::getGlobalNamespace (L)
		.beginNamespace ("ARDOUR")
		.beginClass <PublicEditor> ("Editor")
		.addFunction ("undo", &PublicEditor::undo)
		.addFunction ("redo", &PublicEditor::redo)
		.addFunction ("play_selection", &PublicEditor::play_selection)
		.addFunction ("play_with_preroll", &PublicEditor::play_with_preroll)
		.addFunction ("maybe_locate_with_edit_preroll", &PublicEditor::maybe_locate_with_edit_preroll)
		.addFunction ("add_location_from_playhead_cursor", &PublicEditor::add_location_from_playhead_cursor)
		.addFunction ("goto_nth_marker", &PublicEditor::goto_nth_marker)
		.addFunction ("set_show_measures", &PublicEditor::set_show_measures)
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


/** some lua examples:
 *
 * print (Session:route_by_remote_id(1):name())
 *
 * a = Session:route_by_remote_id(1);
 * print (a:name());
 *
 * print(Session:get_tracks():size())
 *
 * for i, v in ipairs(Session:unknown_processors():table()) do print(v) end
 * for i, v in ipairs(Session:get_tracks():table()) do print(v:name()) end
 *
 * for t in Session:get_tracks():iter() do print(t:name()) end
 * for r in Session:get_routes():iter() do print(r:name()) end
 *
 *
 * Session:tempo_map():add_tempo(ARDOUR.Tempo(100,4), ARDOUR.BBT_TIME(4,1,0))
 *
 *
 * Editor:set_zoom_focus(Editing.ZoomFocusRight)
 * print(Editing.ZoomFocusRight);
 * Editor:set_zoom_focus(1)
 *
 *
 * files = ARDOUR.StringVector();
 * files:push_back("/home/rgareus/data/coding/ltc-tools/smpte.wav")
 * pos = -1
 * Editor:do_import(files, Editing.ImportDistinctFiles, Editing.ImportAsTrack, ARDOUR.SrcQuality.SrcBest, pos, ARDOUR.PluginInfo())
 *
 * #or in one line:
 * Editor:do_import(ARDOUR.StringVector():add({"/path/to/file.wav"}), Editing.ImportDistinctFiles, Editing.ImportAsTrack, ARDOUR.SrcQuality.SrcBest, -1, ARDOUR.PluginInfo())
 *
 * # called when a new session is loaded:
 * function new_session (name) print("NEW SESSION:", name) end
 */
