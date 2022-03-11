#include <iostream>
#include <fstream>

#include <clang/AST/AST.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Frontend/ASTConsumers.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/CommandLine.h>

using namespace std;

using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;

static llvm::cl::OptionCategory category("My tool options");
static llvm::cl::opt<std::string>
	OutputFilename("o", llvm::cl::desc("<output file>"), llvm::cl::Required);

/*static llvm::cl::opt<std::string>
	InputFilename("c", llvm::cl::desc("<input file>"), llvm::cl::Required);

static llvm::cl::opt<std::string>
	SystemPath("isystem", llvm::cl::desc("System path"), llvm::cl::Optional);*/

class ExampleVisitor : public RecursiveASTVisitor<ExampleVisitor> {
private:
	ASTContext *astContext;
	std::ofstream out;

public:
	explicit ExampleVisitor(CompilerInstance *CI)
		: astContext(&(CI->getASTContext()))
	{
		// FIXME: Pass in from outside!
		out.open(OutputFilename.getValue().c_str(), ios::out);
		if(!out)
			throw std::runtime_error("Could not open output file!");
	}

	bool VisitFunctionDecl(FunctionDecl* decl)
	{
		//if(returnType.getAsString())
		//	returnTypeName = returnType->getAsTagDecl()->getNameAsString();

		out << "function " << decl->getNameAsString() << "(";

		size_t i = 0;
		for(auto& k : decl->parameters())
			out << ctype2luapp(k->getType()) << " " << k->getNameAsString() << (++i < decl->param_size() ? ", " : "");

		out << ") -> " << ctype2luapp(decl->getReturnType()) << std::endl;
	}

	bool VisitRecordDecl(RecordDecl* r)
	{
		out << "class " << r->getNameAsString() << " {" << std::endl;

		for(auto& k : r->decls())
			if(isa<FieldDecl>(k))
			{
				FieldDecl* decl = dyn_cast<FieldDecl>(k);
				out << "local " << decl->getNameAsString() << " -> " << ctype2luapp(decl->getType()) << std::endl;
			}

		out << "}" << std::endl;
	}

	std::string ctype2luapp(QualType type)
	{
		std::string name;
		std::string prefix;

		while(type->isPointerType())
		{
			type = type->getPointeeType();
			prefix += "@";
		}

		/*type.removeLocalConst();
		type.removeLocalVolatile();

		name = type.getAsString();*/

		if(type->getAsStructureType())
		{
			auto tagDecl = type->getAsStructureType()->getDecl();
			name = tagDecl->getNameAsString();

		}
		else if(type->isScalarType())
		{
			if(type->isUnsignedIntegerType())
				prefix += "u";

			if(type->isIntegerType())
			{
				if (type->isChar32Type())
					name = "int";
				else if (type->isChar16Type())
					name = "short";
				else if (type->isCharType())
					name = "byte";
				else
				{
					name = "int";
				}
			}
			else if(type->isFloatingType())
			{
				name = "float";
			}
		}
		else if(type->isVoidType())
			name = "void";

		return prefix + name;
	}
};

class ExampleASTConsumer : public ASTConsumer {
private:
	ExampleVisitor *visitor;

public:
	explicit ExampleASTConsumer(CompilerInstance *CI)
		: visitor(new ExampleVisitor(CI))
	{ }

	virtual void HandleTranslationUnit(ASTContext &Context) {
		for(auto& k : Context.getTranslationUnitDecl()->decls())
			visitor->TraverseDecl(k);
	}
};

class ExampleFrontendAction : public ASTFrontendAction {
public:
	virtual unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef file) {
		return std::unique_ptr<ASTConsumer>(new ExampleASTConsumer(&CI)); // pass CI pointer to ASTConsumer
	}
};

int main(int argc, const char **argv)
{
	auto op = CommonOptionsParser::create(argc, argv, category);
	ClangTool Tool(op->getCompilations(), op->getSourcePathList());

	int result = Tool.run(newFrontendActionFactory<ExampleFrontendAction>().get());
	return result;
}

