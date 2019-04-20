#ifndef LUA_METACONTEXT_H
#define LUA_METACONTEXT_H

#include <lua.hpp>

namespace AST { class Module; class Meta; }
class MetaContext
{
	lua_State* L;
public:
	MetaContext();
	~MetaContext();

	void apply(AST::Module& module, AST::Meta* meta);
};

#endif //LUA_METACONTEXT_H
