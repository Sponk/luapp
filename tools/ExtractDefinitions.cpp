#include "../src/Util.h"
#include <llvm/Pass.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/TypeFinder.h>
#include <iostream>
#include <fstream>

using namespace llvm;

class ExtractDefinitions : public ModulePass
{
public:
	ExtractDefinitions() : llvm::ModulePass(ID) {}
	~ExtractDefinitions() {}

	bool runOnModule(Module& M) override
	{
		std::ofstream out(M.getName().str() + ".lpp");
		if(!out)
			return false;

		llvm::TypeFinder StructTypes;
		StructTypes.run(M, true);

		for (auto& p : M.getIdentifiedStructTypes())
		{
			if(p->isLiteral())
				continue;

			out << "class " << p->getStructName().str() << "{" << std::endl;
			for(auto& t : p->elements())
				out << t->getStructName().str();
			out << "}" << std::endl;
		}

		for(auto& k : M.functions())
		{
			if(k.isPrivateLinkage(k.getLinkage()))
				continue;

			out << "function " << k.getName().str() << "(";
			size_t i = 0;
			for(auto& p : k.args())
			{
				out << type2str(p.getType()) << p.getName().str() << (++i < k.arg_size() ? ", " : "");
			}

			out << ") -> " << type2str(k.getReturnType()) << std::endl;
		}

		return true;
	}

	static char ID;
};

char ExtractDefinitions::ID = 0;
static RegisterPass<ExtractDefinitions> X("ExtractDefinitions", "Extract Lua++ definitions from generic libraries.");
