/*
    Copyright (C) 2016 Robin Gareus <robin@gareus.org>

    This program is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the Free
    Software Foundation; either version 2 of the License, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "timecode/bbt_time.h"

#include "ardour/audio_buffer.h"
#include "ardour/audio_track.h"
#include "ardour/buffer_set.h"
#include "ardour/chan_mapping.h"
#include "ardour/dB.h"
#include "ardour/luabindings.h"
#include "ardour/meter.h"
#include "ardour/midi_track.h"
#include "ardour/runtime_functions.h"
#include "ardour/session.h"
#include "ardour/session_object.h"
#include "ardour/tempo.h"

#include "LuaBridge/LuaBridge.h"

/* LUA Bindings for libardour and friends
 *
 * - Prefer factory methods over Contructors whenever possible.
 *   Don't expose the constructor method unless required.
 *
 *   e.g. Don't allow the script to construct a Track Object directly
 *   but do allow to create a BBT_TIME() object.
 *
 * - Do not dereference Shared or Weak Pointers. Pass the pointer to lua.
 * - Define Objects as boost:shared_ptr Object whenever possible.
 *
 *   Storing a boost::shared_ptr in a lua-variable keeps the reference
 *   until that variable is set to 'nil'.
 *   (if the script were to keep a direct pointer to the object instance, the
 *   behaviour is undefined if the actual object goes away)
 *
 *   Methods of the actual class are not directly exposed, but get() or lock()
 *   the pointer.
 *   LuaBridge offers "PtrClass" and "PtrFunction" for boost::shared_ptr
 *   and "WPtrClass" and "WPtrFunction" for boost::weak_ptr
 *
 * - non-const primitives passed by reference to a method are not supported.
 *   lua does not support references of built-in types as function arguments.
 *
 *   Wrap the function (additional return values if needed, or replace the
 *   parameter with a struct/class and pass a pointer).
 *   If the returned-value is not important, convert it to a primitive.
 *   (see libs/lua/LuaBridge/detail/Stack.h  boost::reference_wrapper())
 *
 * That's all for now.
 */


using namespace ARDOUR;

void
LuaBindings::stddef (lua_State* L)
{
	// std::list<std::string>
	luabridge::getGlobalNamespace (L)
		.beginNamespace ("ARDOUR")
		.beginStdList <std::string> ("StringList")
		.endClass ()
		.endNamespace ();

	// std::vector<std::string>
	luabridge::getGlobalNamespace (L)
		.beginNamespace ("ARDOUR")
		.beginStdVector <std::string> ("StringVector")
		.endClass ()
		.endNamespace ();

	// register float array (float*)
	luabridge::getGlobalNamespace (L)
		.beginNamespace ("ARDOUR")
		.registerArray <float> ("FloatArray")
		.endNamespace ();
}

void
LuaBindings::rtsafe_common (lua_State* L)
{
	luabridge::getGlobalNamespace (L)
		.beginNamespace ("ARDOUR")

		.beginClass <AudioBuffer> ("AudioBuffer")
		.addFunction ("data", (Sample*(AudioBuffer::*)(framecnt_t))&AudioBuffer::data)
		.addFunction ("silence", &AudioBuffer::silence)
		.addFunction ("silence", &AudioBuffer::silence)
		.addFunction ("apply_gain", &AudioBuffer::apply_gain)
		.endClass()

		.beginClass <ChanCount> ("ChanCount")
		.addFunction ("n_audio", &ChanCount::n_audio)
		.endClass()

		.beginClass <BufferSet> ("BufferSet")
		.addFunction ("get_audio", static_cast<AudioBuffer&(BufferSet::*)(size_t)>(&BufferSet::get_audio))
		.addFunction ("count", static_cast<const ChanCount&(BufferSet::*)()const>(&BufferSet::count))
		.endClass()

		.beginClass <ChanMapping> ("ChanMapping")
		.addFunction ("get", static_cast<uint32_t(ChanMapping::*)(DataType, uint32_t)>(&ChanMapping::get))
		.addFunction ("set", &ChanMapping::set)
		.addConst ("Invalid", 4294967295) // UINT32_MAX
		.endClass ()

		.beginClass <DataType> ("DataType")
		.addConstructor <void (*) (std::string)> ()
		.endClass ()

		.endNamespace ();

	luabridge::getGlobalNamespace (L)
		.beginNamespace ("ARDOUR")
		.beginNamespace ("DSP")
		.addFunction ("compute_peak", ARDOUR::compute_peak)
		.addFunction ("find_peaks", ARDOUR::find_peaks)
		.addFunction ("apply_gain_to_buffer", ARDOUR::apply_gain_to_buffer)
		.addFunction ("mix_buffers_no_gain", ARDOUR::mix_buffers_no_gain)
		.addFunction ("mix_buffers_with_gain", ARDOUR::mix_buffers_with_gain)
		.addFunction ("copy_vector", ARDOUR::copy_vector)
		.addFunction ("dB_to_coefficient", &dB_to_coefficient)
		.addFunction ("fast_coefficient_to_dB", &fast_coefficient_to_dB)
		.addFunction ("accurate_coefficient_to_dB", &accurate_coefficient_to_dB)
		.endNamespace ()
		.endNamespace ();
}

void
LuaBindings::rtsafe_session (lua_State* L)
{
	luabridge::getGlobalNamespace (L)
		.beginNamespace ("ARDOUR")
		.beginClass <Session> ("Session")
		.addFunction ("transport_rolling", &Session::transport_rolling)
		.addFunction ("request_transport_speed", &Session::request_transport_speed)
		.endClass ()
		.endNamespace ();
}

void
LuaBindings::libardour (lua_State* L)
{
	luabridge::getGlobalNamespace (L)
		.beginNamespace ("PBD")
		.beginClass <PBD::ID> ("ID")
		.addConstructor <void (*) (std::string)> ()
		.addFunction ("to_s", &PBD::ID::to_s) // TODO special case LUA __tostring ?
		.endClass ()
		.endNamespace ();

	luabridge::getGlobalNamespace (L)
		.beginNamespace ("ARDOUR") // XXX
		.beginClass <Timecode::BBT_Time> ("BBT_TIME")
		.addConstructor <void (*) (uint32_t, uint32_t, uint32_t)> ()
		.endClass ()

		.beginPtrClass <PluginInfo> ("PluginInfo")
		.addConstructor <void (*) ()> ()
		.endClass ()

		.beginPtrClass <SessionObject> ("SessionObject")
		.addPtrFunction ("name", &SessionObject::name)
		.endClass ()

		.derivePtrClass <Route, SessionObject> ("Route")
		.addPtrFunction ("set_name", &Route::set_name)
		.addPtrFunction ("comment", &Route::comment)
		.addPtrFunction ("active", &Route::active)
		.addPtrFunction ("set_active", &Route::set_active)
		.endClass ()

		.derivePtrClass <Track, Route> ("Track")
		.addPtrFunction ("set_name", &Track::set_name)
		.addPtrFunction ("can_record", &Track::can_record)
		.addPtrFunction ("record_enabled", &Track::record_enabled)
		.addPtrFunction ("record_safe", &Track::record_safe)
		.addPtrFunction ("set_record_enabled", &Track::set_record_enabled)
		.addPtrFunction ("set_record_safe", &Track::set_record_safe)
		.endClass ()

		.derivePtrClass <AudioTrack, Track> ("AudioTrack")
		.endClass ()

		.derivePtrClass <MidiTrack, Track> ("MidiTrack")
		.endClass ()

	// <std::list<boost::shared_ptr<AudioTrack> >
		.beginStdList <boost::shared_ptr<AudioTrack> > ("AudioTrackList")
		.endClass ()

	// std::list<boost::shared_ptr<MidiTrack> >
		.beginStdList <boost::shared_ptr<MidiTrack> > ("MidiTrackList")
		.endClass ()

	// RouteList ==  boost::shared_ptr<std::list<boost::shared_ptr<Route> > >
		.beginPtrStdList <boost::shared_ptr<Route> > ("RouteListPtr")
		.endClass ()

		.beginClass <Tempo> ("Tempo")
		.addConstructor <void (*) (double, double)> ()
		.addFunction ("note_type", &Tempo::note_type)
		.addFunction ("beats_per_minute", &Tempo::beats_per_minute)
		.addFunction ("frames_per_beat", &Tempo::frames_per_beat)
		.endClass ()

		.beginClass <Meter> ("Meter")
		.addConstructor <void (*) (double, double)> ()
		.addFunction ("divisions_per_bar", &Meter::divisions_per_bar)
		.addFunction ("note_divisor", &Meter::note_divisor)
		.addFunction ("frames_per_bar", &Meter::frames_per_bar)
		.addFunction ("frames_per_grid", &Meter::frames_per_grid)
		.endClass ()

		.beginClass <TempoMap> ("TempoMap")
		.addFunction ("add_tempo", &TempoMap::add_tempo)
		.addFunction ("add_meter", &TempoMap::add_meter)
		.endClass ()
		.endNamespace ();

  LuaBindings::rtsafe_session (L);

	luabridge::getGlobalNamespace (L)
		.beginNamespace ("ARDOUR")
		.beginClass <Session> ("Session")
		.addFunction ("actively_recording", &Session::actively_recording)
		.addFunction ("get_routes", &Session::get_routes)
		.addFunction ("get_tracks", &Session::get_tracks)
		.addFunction ("name", &Session::name)
		.addFunction ("path", &Session::path)
		.addFunction ("record_status", &Session::record_status)
		.addFunction ("route_by_id", &Session::route_by_id)
		.addFunction ("route_by_name", &Session::route_by_name)
		.addFunction ("route_by_remote_id", &Session::route_by_remote_id)
		.addFunction ("save_state", &Session::save_state)
		.addFunction ("set_dirty", &Session::set_dirty)
		.addFunction ("snap_name", &Session::snap_name)
		.addFunction ("tempo_map", (TempoMap& (Session::*)())&Session::tempo_map)
		.addFunction ("unknown_processors", &Session::unknown_processors)

		.addFunction<RouteList (Session::*)(uint32_t, const std::string&, const std::string&, PlaylistDisposition)> ("new_route_from_template", &Session::new_route_from_template)
		// TODO  session_add_audio_track  session_add_midi_track  session_add_mixed_track
		//.addFunction ("new_midi_track", &Session::new_midi_track)
		.endClass ()

		/* session enums */
		.beginNamespace ("Session")
		.beginNamespace ("RecordState")
		.addConst ("Disabled", ARDOUR::Session::RecordState(Session::Disabled))
		.addConst ("Enabled", ARDOUR::Session::RecordState(Session::Enabled))
		.addConst ("Recording", ARDOUR::Session::RecordState(Session::Recording))
		.endNamespace ()
		.endNamespace () // Session

		/* Ardour enums */
		.beginNamespace ("SrcQuality")
		.addConst ("SrcBest", ARDOUR::SrcQuality(SrcBest))
		.endNamespace ()

		.beginNamespace ("PlaylistDisposition")
		.addConst ("CopyPlaylist", ARDOUR::PlaylistDisposition(CopyPlaylist))
		.addConst ("NewPlaylist", ARDOUR::PlaylistDisposition(NewPlaylist))
		.addConst ("SharePlaylist", ARDOUR::PlaylistDisposition(SharePlaylist))
		.endNamespace ()

		.endNamespace (); // ARDOUR
}

void
LuaBindings::set_session (lua_State* L, Session *s)
{
	/* LuaBridge uses unique keys to identify classes/c-types.
	 *
	 * Those keys are "generated" by using the memory-address of a static
	 * variable, templated for every Class.
	 * (see libs/lua/LuaBridge/detail/ClassInfo.h)
	 *
	 * When linking the final executable there must be exactly one static
	 * function (static variable) for every templated class.
	 * This works fine on OSX and Linux...
	 *
	 * Windows (mingw and MSVC) however expand the template differently for libardour
	 * AND gtk2_ardour. We end up with two identical static functions
	 * at different addresses!!
	 *
	 * The Solution: have gtk2_ardour never include LuaBridge headers directly
	 * and always go via libardour function calls for classes that are registered
	 * in libardour. (calling lua itself is fine,  calling c-functions in the GUI
	 * which expand the template is not)
	 *
	 * (the actual cause: even static symbols in a .dll have no fixed address
	 * and are mapped when loading the dll. static functions in .exe do have a fixed
	 * address)
	 *
	 * libardour:
	 *  0000000000000000 I __imp__ZZN9luabridge9ClassInfoIN6ARDOUR7SessionEE11getClassKeyEvE5value
	 *  0000000000000000 I __nm__ZZN9luabridge9ClassInfoIN6ARDOUR7SessionEE11getClassKeyEvE5value
	 *  0000000000000000 T _ZN9luabridge9ClassInfoIN6ARDOUR7SessionEE11getClassKeyEv
	 *
	 * ardour.exe
	 *  000000000104f560 d .data$_ZZN9luabridge9ClassInfoIN6ARDOUR7SessionEE11getClassKeyEvE5value
	 *  000000000104f560 D _ZZN9luabridge9ClassInfoIN6ARDOUR7SessionEE11getClassKeyEvE5value
	 *  0000000000e9baf0 T _ZN9luabridge9ClassInfoIN6ARDOUR7SessionEE11getClassKeyEv
	 *
	 *
	 */
	luabridge::push <Session *> (L, s);
	lua_setglobal (L, "Session");

	if (s) {
		// call lua function.
		luabridge::LuaRef cb_ses = luabridge::getGlobal (L, "new_session");
		if (cb_ses.type() == LUA_TFUNCTION) { cb_ses(s->name()); } // TODO args
	}
}
