#include <SemanticChecker.h>

void SemanticChecker::matchTypes(AST::Module& module, AST::Expr* a, AST::Expr* b)
{
	const std::string typeA = a->getType();
	const std::string typeB = b->getType();

	if(typeA != typeB)
	{
		module.error("Types do not match. Expected " + typeA + " but got " + typeB , b->getLocation());
	}
}

void SemanticChecker::check(AST::Module& module)
{
	module.visit([&module](AST::Expr* expr) {
		if(auto fn = dynamic_cast<AST::Function*>(expr))
		{
			if(fn->getReturnType() == "void")
				return;

			bool hasReturn = false;
			for(auto& innerExpr : fn->getBody())
			{
				if(auto ret = dynamic_cast<AST::Return*>(innerExpr.get()))
				{
					hasReturn = true;
					break;
				}
			}

			if(!hasReturn)
			{
				module.error("A non-void function always has to return something! Return statement missing!", fn->getLocation());
			}
		}
		/*else if(auto binop = dynamic_cast<AST::BinaryOp*>(expr))
		{
			matchTypes(module, binop->getLeft().get(), binop->getRight().get());
		}*/
	});
}
