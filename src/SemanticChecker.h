#ifndef LUA_SEMANTICCHECKER_H
#define LUA_SEMANTICCHECKER_H

#include <AST.h>

class SemanticChecker
{
public:
	static void matchTypes(AST::Module& module, AST::Expr* a, AST::Expr* b);
	void check(AST::Module& module);
};

#endif //LUA_SEMANTICCHECKER_H
