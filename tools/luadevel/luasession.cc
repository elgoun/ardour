#include <stdint.h>
#include <assert.h>

#include <cstdio>
#include <iostream>
#include <string>
#include <list>
#include <vector>

#include <glibmm.h>

#include "pbd/debug.h"
#include "pbd/event_loop.h"
#include "pbd/error.h"
#include "pbd/failed_constructor.h"
#include "pbd/pthread_utils.h"
#include "pbd/reallocpool.h"
#include "pbd/receiver.h"
#include "pbd/transmitter.h"

#include "ardour/ardour.h"
#include "ardour/audioengine.h"
#include "ardour/filename_extensions.h"
#include "ardour/luabindings.h"
#include "ardour/session.h"
#include "ardour/types.h"
#include "ardour/vst_types.h"

#include <readline/readline.h>
#include <readline/history.h>

#include "lua/luastate.h"
#include "LuaBridge/LuaBridge.h"

using namespace std;
using namespace ARDOUR;
using namespace PBD;

static const char* localedir = LOCALEDIR;
static PBD::ScopedConnectionList forever_connections;
static Session *_session = NULL;
static LuaState lua;

class TestReceiver : public Receiver
{
  protected:
    void receive (Transmitter::Channel chn, const char * str);
};

static TestReceiver test_receiver;

void
TestReceiver::receive (Transmitter::Channel chn, const char * str)
{
	const char *prefix = "";

	switch (chn) {
	case Transmitter::Error:
		prefix = ": [ERROR]: ";
		break;
	case Transmitter::Info:
		/* ignore */
		return;
	case Transmitter::Warning:
		prefix = ": [WARNING]: ";
		break;
	case Transmitter::Fatal:
		prefix = ": [FATAL]: ";
		break;
	case Transmitter::Throw:
		/* this isn't supposed to happen */
		abort ();
	}

	/* note: iostreams are already thread-safe: no external
	   lock required.
	*/

	std::cout << prefix << str << std::endl;

	if (chn == Transmitter::Fatal) {
		::exit (9);
	}
}

class MyEventLoop : public sigc::trackable, public EventLoop
{
	public:
		MyEventLoop (std::string const& name) : EventLoop (name) {
			run_loop_thread = Glib::Threads::Thread::self();
		}

		void call_slot (InvalidationRecord*, const boost::function<void()>& f) {
			if (Glib::Threads::Thread::self() == run_loop_thread) {
				f ();
			}
		}

		Glib::Threads::Mutex& slot_invalidation_mutex() { return request_buffer_map_lock; }

	private:
		Glib::Threads::Thread* run_loop_thread;
		Glib::Threads::Mutex   request_buffer_map_lock;
};

static MyEventLoop *event_loop = NULL;

static int do_audio_midi_setup (uint32_t desired_sample_rate)
{
	return 0;
}

static void init ()
{
	if (!ARDOUR::init (false, true, localedir)) {
		cerr << "Ardour failed to initialize\n" << endl;
		::exit (EXIT_FAILURE);
	}

	event_loop = new MyEventLoop ("util");
	EventLoop::set_event_loop_for_thread (event_loop);
	SessionEvent::create_per_thread_pool ("util", 512);

	test_receiver.listen_to (error);
	test_receiver.listen_to (info);
	test_receiver.listen_to (fatal);
	test_receiver.listen_to (warning);

	ARDOUR::Session::AudioEngineSetupRequired.connect_same_thread (forever_connections, &do_audio_midi_setup);
}

static void set_session (ARDOUR::Session *s)
{
	_session = s;
	lua_State* L = lua.getState();
	LuaBindings::set_session (L, _session);
}

static void unset_session ()
{
	set_session (NULL);
}

static Session * _load_session (string dir, string state)
{
	AudioEngine* engine = AudioEngine::create ();

	if (!engine->current_backend()) {
		if (!engine->set_backend ("None (Dummy)", "Unit-Test", "")) {
			std::cerr << "Cannot create Audio/MIDI engine\n";
			return 0;
		}
	}

	if (engine->running()) {
		engine->stop();
	}

	float sr;
	SampleFormat sf;

	std::string s = Glib::build_filename (dir, state + statefile_suffix);
	if (!Glib::file_test (dir, Glib::FILE_TEST_EXISTS)) {
		std::cerr << "Cannot find session: " << s << "\n";
		return 0;
	}

	if (Session::get_info_from_path (s, sr, sf) == 0) {
		if (engine->set_sample_rate (sr)) {
			std::cerr << "Cannot set session's samplerate.\n";
			return 0;
		}
	} else {
		std::cerr << "Cannot get samplerate from session.\n";
		return 0;
	}

	init_post_engine ();

	if (engine->start () != 0) {
		std::cerr << "Cannot start Audio/MIDI engine\n";
		return 0;
	}

	Session* session = new Session (*engine, dir, state);
	engine->set_session (session);
	return session;
}

static Session* load_session (string dir, string state)
{
	Session* s = 0;
	if (_session) {
		cerr << "Session already open" << "\n";
		return 0;
	}
	try {
		s = _load_session (dir, state);
	} catch (failed_constructor& e) {
		cerr << "failed_constructor: " << e.what() << "\n";
		return 0;
	} catch (AudioEngine::PortRegistrationFailure& e) {
		cerr << "PortRegistrationFailure: " << e.what() << "\n";
		return 0;
	} catch (exception& e) {
		cerr << "exception: " << e.what() << "\n";
		return 0;
	} catch (...) {
		cerr << "unknown exception.\n";
		return 0;
	}
	assert (s);
	set_session (s);
	s->DropReferences.connect_same_thread (forever_connections, &unset_session);
	return s;
}

static void close_session ()
{
	if (!_session) {
		cerr << "No open session" << "\n";
		return;
	}
	delete _session;
	_session = NULL;
}

static int close_session_lua (lua_State *L)
{
	close_session ();
	return 0;
}

/* extern VST functions */
int vstfx_init (void*) { return 0; }
void vstfx_exit () {}
void vstfx_destroy_editor (VSTState*) {}

static void my_lua_print (std::string s) {
	std::cout << s << "\n";
}

int main (int argc, char **argv)
{
	init ();
	lua.Print.connect (&my_lua_print);
	lua_State* L = lua.getState();

	LuaBindings::stddef (L);
	LuaBindings::common (L);
	LuaBindings::session (L);

	luabridge::getGlobalNamespace (L)
		.beginNamespace ("_G")
		.addFunction ("load_session", load_session)
		.addFunction ("close_session", close_session)
		.endNamespace ();

	luabridge::getGlobalNamespace (L)
		.beginNamespace ("ARDOUR")
		.beginClass <Session> ("Session")
		.addExtCFunction ("close", &close_session_lua)
		.endClass ()
		.endNamespace ();



	char *line;
	while ((line = readline ("> "))) {
		if (!strcmp (line, "quit")) {
			break;
		}
		if (strlen(line) == 0) {
			continue;
		}
		if (!lua.do_command (line)) {
			add_history(line); // OK
		} else {
			add_history(line); // :)
		}
	}
	printf("\n");

	if (_session) {
		close_session ();
	}

	AudioEngine::instance()->stop ();
	AudioEngine::destroy ();

	// cleanup
	ARDOUR::cleanup ();
	delete event_loop;
	pthread_cancel_all ();
	return 0;
}
