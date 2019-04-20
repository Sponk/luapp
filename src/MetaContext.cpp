#include <lua.hpp>
#include "MetaContext.h"
#include "AST.h"

using namespace AST;

extern "C"
{
int luaopen_AST(lua_State*);
}

MetaContext::MetaContext()
{
	L = luaL_newstate();
	luaL_openlibs(L);
	luaopen_AST(L);
}

MetaContext::~MetaContext()
{
	lua_close(L);
}

void MetaContext::apply(AST::Module& module, AST::Meta* meta)
{ std::cout << meta->toLua() << std::endl;
	if(luaL_dostring(L, meta->toLua().c_str()) != 0)
	{
		module.error(std::string("Could not apply meta block: ") + lua_tostring(L, -1), meta->getLocation());
	}
}
