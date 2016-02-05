#ifndef __gtkardour_luainstance_h__
#define __gtkardour_luainstance_h__

#include <bitset>
#include "pbd/xml++.h"
#include "pbd/signals.h"

#include "ardour/luascripting.h"
#include "ardour/luabindings.h"
#include "ardour/session_handle.h"

#include "lua/luastate.h"
#include "LuaBridge/LuaBridge.h"

#include "luasignal.h"
#include <boost/any.hpp>

typedef std::bitset<LuaSignal::LAST_SIGNAL> ActionHook;

class LuaCallback : public ARDOUR::SessionHandlePtr, public sigc::trackable
{
public:
	LuaCallback (ARDOUR::Session*, const ActionHook&, luabridge::LuaRef const * const, const int id);
	~LuaCallback ();

	void set_session (ARDOUR::Session *);
protected:
	void session_going_away ();

private:
	PBD::ScopedConnectionList _connections;
	ActionHook _signals;
	luabridge::LuaRef * _call;
	int _id;

	void reconnect ();

	template <typename S> void connect_0 (enum LuaSignal::LuaSignal, S*);
	void proxy_0 (enum LuaSignal::LuaSignal);

	template <typename C1> void connect_1 (enum LuaSignal::LuaSignal, PBD::Signal1<void, C1>*);
	template <typename C1> void proxy_1 (enum LuaSignal::LuaSignal, C1);

	template <typename C1, typename C2> void connect_2 (enum LuaSignal::LuaSignal, PBD::Signal2<void, C1, C2>*);
	template <typename C1, typename C2> void proxy_2 (enum LuaSignal::LuaSignal, C1, C2);
};

typedef boost::shared_ptr<LuaCallback> LuaCallbackPtr;
typedef std::vector<LuaCallbackPtr> LuaCallbackList;

class LuaInstance : public PBD::ScopedConnectionList, public ARDOUR::SessionHandlePtr
{
public:
	static LuaInstance* instance();
	~LuaInstance();

	static void register_classes (lua_State* L);

	void set_session (ARDOUR::Session* s);

	int set_state (const XMLNode&);
	XMLNode& get_state (void);

	/* actions */
	void call_action (const int);

	bool set_lua_action (const int, const std::string&, const std::string&, const ARDOUR::LuaScriptParamList&);
	bool remove_lua_action (const int);
	bool lua_action_name (const int, std::string&);
	bool lua_action (const int, std::string&, std::string&, ARDOUR::LuaScriptParamList&);

	sigc::signal<void,int,std::string> ActionChanged;

	int  register_lua_slot (const std::string&, const ARDOUR::LuaScriptParamList&);
	bool unregister_lua_slot (const int);
	bool get_lua_slot (const int, ActionHook&, std::string&, ARDOUR::LuaScriptParamList&);

private:
	LuaInstance();
	static LuaInstance* _instance;

	void init ();
	void session_going_away ();

	LuaState lua;

	luabridge::LuaRef * _lua_call_action;
	luabridge::LuaRef * _lua_add_action;
	luabridge::LuaRef * _lua_del_action;
	luabridge::LuaRef * _lua_get_action;
	luabridge::LuaRef * _lua_call_hook;

	luabridge::LuaRef * _lua_load;
	luabridge::LuaRef * _lua_save;

	LuaCallbackList _callbacks;
};

#endif
