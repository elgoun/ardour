#include <cstdio>
#include <iostream>
#include <string>
#include <list>
#include <vector>

#include <readline/readline.h>
#include <readline/history.h>

#include "lua/luastate.h"

#include <luabind/luabind.hpp>
#include <luabind/iterator_policy.hpp>

static void my_lua_print (std::string s) {
	std::cout << s << "\n";
}

void greet()
{
	std::cout << "hello world!\n";
}

class A {
	public:
		A() { printf ("CTOR\n"); _sl.push_back("test1"); _sl.push_back("test2"); }
		~A() { printf ("DTOR\n"); }

		void set_int (int a) { _int = a; }
		int  get_int () const { return _int; }

		int& get_ref () { return _int; }
		void get_arg (int &a) { a = _int; }
		void set_ref (int const &a) { _int = a; }

		void set_list (std::list<std::string> sl) { _sl = sl; }
		std::list<std::string>& get_list () { return _sl; }

		std::vector<std::string> names;

		enum EN {
			RV1 = 1, RV2, RV3
		};

		enum EN ret_enum () { return _en;}
		void set_enum (enum EN en) { _en = en; }

	private:
		std::list<std::string> _sl;
		int _int;
		enum EN _en;
};


int main (int argc, char **argv)
{
	LuaState lua;
	lua.Print.connect (&my_lua_print);
	lua_State* L = lua.getState();

	using namespace luabind;

	open(L);
#if 0
	module(L)
		[
		def("greet", &greet)
		];
#endif
#if 1
	module(L)
		[
		class_<std::vector<std::string> >("vector")
			.def(constructor<>())
			.def("push_back",  (void(std::vector<std::string>::*)(const std::string&)) &std::vector<std::string>::push_back)
		];
#endif

	module(L)
		[
		class_<A>("A")
			.def(constructor<>())
			.def("set_int", &A::set_int)
			.def("get_int", &A::get_int)
			.def("get_ref", &A::get_ref)
			.def("get_arg", &A::get_arg)
			.def("set_ref", &A::set_ref)
			.def("set_list", &A::set_list)
			.def("get_list", &A::get_list, return_stl_iterator)
			.def_readwrite("names", &A::names, return_stl_iterator)
			.def("ret_enum", &A::ret_enum)
			.def("set_enum", &A::set_enum)
			.enum_("constants")
			[
			value("my_enum", 4),
		value("my_2nd_enum", 7),
		value("another_enum", 6)
			]
		];

	/////////////////////////////////////////////////////////////////////////////
	char *line;
	while ((line = readline ("> "))) {
		if (!strcmp (line, "quit")) {
			break;
		}
		if (strlen(line) == 0) {
			//lua.do_command("collectgarbage();");
			continue;
		}
		if (!lua.do_command (line)) {
			add_history(line); // OK
		} else {
			add_history(line); // :)
		}
	}
	printf("\n");
	return 0;
}
