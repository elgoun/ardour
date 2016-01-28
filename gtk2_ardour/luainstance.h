#ifndef __gtkardour_luainstance_h__
#define __gtkardour_luainstance_h__

#include "pbd/xml++.h"

#include "ardour/luascripting.h"
#include "ardour/luabindings.h"
#include "ardour/session_handle.h"

#include "lua/luastate.h"
#include "LuaBridge/LuaBridge.h"

class LuaInstance : public PBD::ScopedConnectionList, public ARDOUR::SessionHandlePtr
{
public:
	static LuaInstance* instance();
	~LuaInstance();

	static void register_classes (lua_State* L);
	static bool try_compile (const std::string&, const ARDOUR::LuaScriptParamList&);

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

#if 0 // TODO
	/* event hooks */
	enum ActionHook {
		EverySecond                = 0x00000001,
		SessionLoad                = 0x00000002,
		SessionClose               = 0x00000004,
	};

	int  register_lua_slot (ActionHook, const std::string&, const ARDOUR::LuaScriptParamList&);
	bool unregister_lua_slot (const int);
	bool get_lua_slot (const int, ActionHook&, std::string&, ARDOUR::LuaScriptParamList&);
#endif

#if 0
	// TODO std::bitset ...
	enum ActionHook {
		EverySecond                = 0x00000001,
		SessionLoad                = 0x00000002,
		SessionClose               = 0x00000004,
		ConfigChanged              = 0x00000008,
		// engine
		EngineRunning              = 0x00000010,
		EngineStopped              = 0x00000020,
		EngineHalted               = 0x00000040,
		EngineDeviceListChanged    = 0x00000080
		BufferSizeChanged          = 0x00000100,
		// session
		SessionConfigChanged       = 0x00000200,
		TransportStateChange       = 0x00000400,
		DirtyChanged               = 0x00000800,
		StateSaved                 = 0x00001000
		Xrun                       = 0x00002000,
		SoloActive                 = 0x00003000,
		SoloChanged                = 0x00008000,
		RecordStateChanged         = 0x00010000,
		AuditionActive             = 0x00020000,
		LocationAdded              = 0x00030000,
		LocationRemoved            = 0x00080000,
		TempMapChanged             = 0x00100000,
		AuditionActive             = 0x00200000,
		PositionChanged            = 0x00400000,
		Located                    = 0x00800000,
		RouteAdded                 = 0x01000000,
		RouteGroupAdded            = 0x02000000,
		RouteGroupRemoved          = 0x04000000,
		RouteGroupsReordered       = 0x08000000,
		// route global
		SyncOrderKeys              = 0x10000000,
		// plugin manager
		PluginListChanged          = 0x20000000,
		PluginStatusesChanged      = 0x40000000,
		// Diskstream::DiskOverrun
		// Diskstream::DiskUnderrun
	};

	// TODO per track/route actions,
	// TODO per plugin actions
	// TODO per region actions
	// TODO any location action
#endif

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

	luabridge::LuaRef * _lua_load;
	luabridge::LuaRef * _lua_save;

	static std::string get_factory_bytecode (const std::string&);
};

#endif
