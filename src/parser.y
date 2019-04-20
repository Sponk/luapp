%locations
%define api.pure full
%lex-param {void* scanner}
%parse-param {void* scanner}

%{

#include <cstdio>
#include <cstdlib>
#include <string>
#include <iostream>
#include <memory>

#ifndef WIN32
#include <unistd.h>
#endif

//extern "C" int yyparse();
extern FILE *yyin;
extern char *yytext;

#define YY_NO_UNPUT
#define YYERROR_VERBOSE

#include <AST.h>
#include <SemanticChecker.h>

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>

#include "parser.hh"
int yylex(YYSTYPE*, YYLTYPE*, void*);
void yyerror(YYLTYPE* locp, void*, char const* msg);

AST::SourceLocation makeSourceLoc(YYLTYPE* l)
{
	AST::SourceLocation loc(l->last_line, l->first_column, l->last_column - l->first_column);
	return loc;
}

std::shared_ptr<AST::Module> ast = std::make_shared<AST::Module>();

%}

%union{
	std::string* sval;
	FunctionBody* functionBody;
	ExprList* exprList;
	float fval;
	int ival;
	AST::Expr* expr;
	AST::Variable* var;
	AST::FunctionCall* call;
	bool bval;
	char cval;
}

%start chunk
%token	<fval>			Number
%token	<ival>			Integer
%token	<sval>			LiteralString
%token	<sval>			Name "identifier"
%token 	<sval>			Operator "op"
%token	<bval>			Bool "boolean"
%token	<cval>			Char "char"
%token Break "break"
%token Goto "goto"
%token Do "do"
%token End "end"
%token While "while"
%token Repeat "repeat"
%token If "if"
%token Then "then"
%token Until "until"
%token Else "else"
%token Function "function"
%token Local "local"
%token Elseif "elseif"
%token Nil
%token Return "return"
%token ArrowRight "->"
%token ArrowLeft "<-"
%token For "for"
%token Extern "extern"
%token OperatorDef "operator"

%token EQ "=="
%token NEQ "~="
%token GEQ ">="
%token LEQ "<="

%token ThreeDot "..."

%token Class "class"
%token Meta "meta"

%type <sval> funcname
%type <functionBody> funcbody
%type <exprList> block
%type <exprList> stat
%type <exprList> statlist
%type <expr> functioncall
%type <exprList> varlist
%type <exprList> explist
%type <exprList> args
%type <exprList> parlist
%type <expr> exp

%type <var> var
%type <sval> label
%type <expr> elseif
%type <exprList> variabledef
%type <sval> pointermarklist
%type <sval> pointermark

%nonassoc Then
%nonassoc Elseif
%nonassoc Else

%left Operator
%left '<' '>' '=' EQ NEQ GEQ LEQ
%left '+' '-'
%left '*' '/'
%right '$' '@'

%%

chunk: block 
{
	for(auto& e : *$1)
		ast->addExpr(e);
		
	delete $1;
};

block:		{ $$ = new ExprList; }
		// | stat { $$ = $1; }
		// | block stat { $$ = $1; $$->insert($$->end(), $2->begin(), $2->end()); delete $2; }
		| statlist { $$ = $1; }
		//| retstat { $$ = $1; }
		;

statlist:	stat { $$ = $1; }
		| statlist stat { $$ = $1; $$->insert($$->end(), $2->begin(), $2->end()); delete $2; }
		;
stat:
		//stat { $$ = $1; $$->insert($$->end(), $2->begin(), $2->end()); delete $2; } 
		/*|*/ ';' { $$ = new ExprList; }
		|		Class Name '{' block '}'
				{
					$$ = new ExprList;
					auto classdef = std::make_shared<AST::ClassDef>(*$2);
					for(auto& k : *$4)
						classdef->getBody().push_back(k);

					classdef->setLocation(makeSourceLoc(&@1));
					$$->push_back(classdef);
					delete $2;
					delete $4;
				}
		| varlist Operator explist
		{
			$$ = new ExprList;

			// FIXME: Check sizes!
			for (unsigned int i = 0; i < $1->size(); i++)
			{
				std::shared_ptr<AST::BinaryOp> op =
					std::make_shared<AST::BinaryOp>((*$1)[i], (*$3)[i], *$2);
					
				op->setLocation(makeSourceLoc(&@2));
				$$->push_back(op);
			}

			delete $1;
			delete $2;
			delete $3;
		}
		| 		exp { $$ = new ExprList; $$->push_back(std::shared_ptr<AST::Expr>($1)); }
		|		label { $$ = new ExprList; $$->push_back(std::make_shared<AST::Label>(*$1)); delete $1;}
		//|		Break
		|		Goto Name { $$ = new ExprList; $$->push_back(std::make_shared<AST::Goto>(*$2)); delete $2; }
		//|		Do block End
		| 		For Name '=' exp ',' exp ',' exp Do block End
		{
			$$ = new ExprList;
			
			// TODO: Check operator for '='
			std::shared_ptr<AST::VariableDef> vardef = std::make_shared<AST::VariableDef>(*$2, "", std::shared_ptr<AST::Expr>($4));
			std::shared_ptr<AST::For> fory = std::make_shared<AST::For>( vardef, 
											std::shared_ptr<AST::Expr>($6), 
											std::shared_ptr<AST::Expr>($8));
											
			fory->setLocation(makeSourceLoc(&@1));
			vardef->setLocation(makeSourceLoc(&@3));
			
			$$->push_back(fory);
			
			for(auto& k : *$10)
				fory->getBody().push_back(k);
			
			delete $2;
			delete $10;
		}
				
		| 		While exp Do block End
				{
					$$ = new ExprList;
					std::shared_ptr<AST::While> whily = std::make_shared<AST::While>(std::shared_ptr<AST::Expr>($2));
					$$->push_back(whily);
					whily->setLocation(makeSourceLoc(&@1));
					
					for(auto& k : *$4)
						whily->getBody().push_back(k);
				
					delete $4;
				}
		//|		Repeat block Until exp

				| If exp Then block End
				{
					$$ = new ExprList;
					std::shared_ptr<AST::If> iffi = std::make_shared<AST::If>(std::shared_ptr<AST::Expr>($2));
					$$->push_back(iffi);
					iffi->setLocation(makeSourceLoc(&@1));
					
					for(auto& k : *$4)
						iffi->getBody().push_back(k);
					
					delete $4;
				}
				| If exp Then block Else block End
				{
					$$ = new ExprList;
					std::shared_ptr<AST::If> iffi = std::make_shared<AST::If>(std::shared_ptr<AST::Expr>($2));
					$$->push_back(iffi);
					iffi->setLocation(makeSourceLoc(&@1));

					for(auto& k : *$4)
						iffi->getBody().push_back(k);
						
					for(auto& k : *$6)
						iffi->getElse().push_back(k);
					
					delete $6;
					delete $4;
				}
				
				| If exp Then block elseif End
				{
					$$ = new ExprList;
					std::shared_ptr<AST::If> iffi = std::make_shared<AST::If>(std::shared_ptr<AST::Expr>($2));
					$$->push_back(iffi);
					iffi->setLocation(makeSourceLoc(&@1));
					
					for(auto& k : *$4)
						iffi->getBody().push_back(k);
						
					iffi->getElse().push_back(std::shared_ptr<AST::Expr>($5));
						
					delete $4;
				}
				
		|		Function funcname funcbody
				{
					$$ = new ExprList;
					std::shared_ptr<AST::Function> function;
					$$->push_back(function = std::make_shared<AST::Function>(*$2, $3->Type));
					function->setLocation(makeSourceLoc(&@1));
					
					for(auto& k : *$3->Body)
						function->getBody().push_back(k);
					
					for(auto& k : *$3->Args)
						function->getArgs().push_back(k);
					
					function->setVariadic($3->IsVariadic);
					
					delete $2;
					delete $3;
				}

		|		OperatorDef pointermark Name Name Operator pointermark Name Name ArrowRight pointermark Name block End
				{
					$$ = new ExprList;
					std::shared_ptr<AST::Function> function;
					$$->push_back(function = std::make_shared<AST::Function>(
									  ast->getOperatorName(*$5, *$2 + *$3, *$6 + *$7), *$10 + *$11));

					function->setLocation(makeSourceLoc(&@1));
					for (auto& k : *$12)
						function->getBody().push_back(k);

					function->getArgs().push_back(
						std::make_shared<AST::VariableDef>(*$4, *$2 + *$3, nullptr));
					function->getArgs().push_back(
						std::make_shared<AST::VariableDef>(*$8, *$6 + *$7, nullptr));

					delete $2;
					delete $6;
					delete $3;
					delete $4;
					delete $7;
					delete $8;
					delete $5;
					delete $10;
					delete $11;
					delete $12;
				}
				
		| Extern Function funcname '(' parlist ')' ArrowRight pointermark Name
		{
			$$ = new ExprList;
			std::shared_ptr<AST::Function> function;
			$$->push_back(function = std::make_shared<AST::Function>(*$3, *$8 + *$9, true));
			function->setLocation(makeSourceLoc(&@1));

			for(auto& k : *$5)
				function->getArgs().push_back(k);
					
			delete $3;
			delete $8;
			delete $9;
			delete $5;
		}
		
		| Extern Function funcname '(' parlist ',' ThreeDot ')' ArrowRight pointermark Name
		{
			$$ = new ExprList;
			std::shared_ptr<AST::Function> function;
			$$->push_back(function = std::make_shared<AST::Function>(*$3, *$10 + *$11, true));
			function->setLocation(makeSourceLoc(&@1));

			for(auto& k : *$5)
				function->getArgs().push_back(k);
			
			function->setVariadic(true);
			
			delete $3;
			delete $10;
			delete $11;
			delete $5;
		}
		
		| Extern Function funcname '(' ThreeDot ')' ArrowRight pointermark Name
		{
			$$ = new ExprList;
			std::shared_ptr<AST::Function> function;
			$$->push_back(function = std::make_shared<AST::Function>(*$3, *$8 + *$9, true));
			function->setLocation(makeSourceLoc(&@1));

			function->setVariadic(true);
			
			delete $3;
			delete $8;
			delete $9;
		}
				
		//|       	Local Function funcname funcbody
		//|		Local namelist
		| variabledef { $$ = $1; }
		| Return exp
		{
			$$ = new ExprList;
			$$->push_back(std::make_shared<AST::Return>(std::shared_ptr<AST::Expr>($2)));
		}

		| Return
		{
			$$ = new ExprList;
			$$->push_back(std::make_shared<AST::Return>(nullptr));
			$$->back()->setLocation(makeSourceLoc(&@1));
		}
		| Meta statlist End // '{' statlist '}'
		{
			$$ = new ExprList;
                	$$->push_back(std::make_shared<AST::Meta>(std::move(*$2)));
                	$$->back()->setLocation(makeSourceLoc(&@1));
                	delete $2;
		}
		;

elseif: Elseif exp Then block
	{
		AST::If* iffi;
		$$ = iffi = new AST::If(std::shared_ptr<AST::Expr>($2));
		iffi->setLocation(makeSourceLoc(&@1));
		
		for(auto& k : *$4)
			iffi->getBody().push_back(k);
		delete $4;
	}
	| elseif Elseif exp Then block
	{
		$$ = $1;
		
		auto iffi = std::make_shared<AST::If>(std::shared_ptr<AST::Expr>($3));
		for(auto& k : *$5)
			iffi->getBody().push_back(k);
		
		static_cast<AST::If*>($$)->getElse().push_back(iffi);
		iffi->setLocation(makeSourceLoc(&@1));
		delete $5;
	}
	| elseif Else block
	{
		$$ = $1;
		for(auto& k : *$3)
			static_cast<AST::If*>($$)->getElse().push_back(k);
	}
	;

variabledef
: Local varlist '=' explist
{
	$$ = new ExprList;
		
	// FIXME: Check sizes!
	for(unsigned int i = 0; i < $2->size(); i++)
	{
		auto variable = std::dynamic_pointer_cast<AST::Variable>((*$2)[i]);
		std::shared_ptr<AST::VariableDef> def = std::make_shared<AST::VariableDef>(variable->getName(), "", (*$4)[i]);
		def->setLocation(makeSourceLoc(&@3));
		
		$$->push_back(def);
	}
		
	delete $2;
	delete $4;
}
	
| Local varlist '=' explist ArrowRight pointermark Name
{
	$$ = new ExprList;
		
	// FIXME: Check sizes!
	for(unsigned int i = 0; i < $2->size(); i++)
	{
		auto variable = std::dynamic_pointer_cast<AST::Variable>((*$2)[i]);
		std::shared_ptr<AST::VariableDef> def = std::make_shared<AST::VariableDef>(variable->getName(), *$6 + *$7, (*$4)[i]);
		def->setLocation(variable->getLocation());
		
		$$->push_back(def);
	}
		
	delete $2;
	delete $4;
	delete $6;
	delete $7;
}
	
| Local varlist ArrowRight pointermark Name
{
	$$ = new ExprList;
		
	// FIXME: Check sizes!
	for(unsigned int i = 0; i < $2->size(); i++)
	{
		auto variable = std::dynamic_pointer_cast<AST::Variable>((*$2)[i]);
		std::shared_ptr<AST::VariableDef> def = std::make_shared<AST::VariableDef>(variable->getName(), *$4 + *$5, nullptr);
		def->setLocation(makeSourceLoc(&@1));
		
		$$->push_back(def);
	}
		
	delete $2;
	delete $4;
	delete $5;
}

| Extern Local varlist ArrowRight pointermark Name
{
	$$ = new ExprList;

	// FIXME: Check sizes!
	for(unsigned int i = 0; i < $3->size(); i++)
	{
		auto variable = std::dynamic_pointer_cast<AST::Variable>((*$3)[i]);
		std::shared_ptr<AST::VariableDef> def = std::make_shared<AST::VariableDef>(variable->getName(), *$5 + *$6, nullptr);
		def->setLocation(makeSourceLoc(&@1));
		def->setExtern(true);

		$$->push_back(def);
	}

	delete $3;
	delete $5;
	delete $6;
}

// Arrays
| Local varlist '=' explist ArrowRight pointermark Name '[' Integer ']'
{
	$$ = new ExprList;
		
	// FIXME: Check sizes!
	for(unsigned int i = 0; i < $2->size(); i++)
	{
		auto variable = std::dynamic_pointer_cast<AST::Variable>((*$2)[i]);
		std::shared_ptr<AST::VariableDef> def = std::make_shared<AST::VariableDef>(variable->getName(), *$6 + *$7, (*$4)[i], $9);
		def->setLocation(makeSourceLoc(&@1));
		
		$$->push_back(def);
	}
		
	delete $2;
	delete $4;
	delete $6;
	delete $7;
}
	
| Local varlist ArrowRight pointermark Name '[' Integer ']'
{
	$$ = new ExprList;
		
	// FIXME: Check sizes!
	for(unsigned int i = 0; i < $2->size(); i++)
	{
		auto variable = std::dynamic_pointer_cast<AST::Variable>((*$2)[i]);
		std::shared_ptr<AST::VariableDef> def = std::make_shared<AST::VariableDef>(variable->getName(), *$4 + *$5, nullptr, $7);
		def->setLocation(makeSourceLoc(&@1));
		
		$$->push_back(def);
	}
		
	delete $2;
	delete $4;
	delete $5;
}
;

label: ':' ':' Name ':' ':' { $$ = $3; };
funcname: Name { $$ = $1; }
		|		funcname '.' funcname { *$1 += "." + *$3; $$ = $1; delete $3; }
		//|		funcname ':' funcname { *$1 += ":" + *$3; $$ = $1; delete $3; }
				;

varlist: var
	{ 
		$$ = new ExprList;
		$$->push_back(std::shared_ptr<AST::Expr>($1));
	}
	
	| varlist ',' var { $$ = $1; $$->push_back(std::shared_ptr<AST::Expr>($3)); }
	;

var: Name { $$ = new AST::Variable(*$1, nullptr); delete $1; $$->setLocation(makeSourceLoc(&@1)); }
	//| 	prefixexp '[' exp ']'
	| var '.' Name
	{
		$$ = $1;
		AST::Variable* var = $1;

		// Search last in linked list
		while(var->getField())
			var = var->getField().get();

		var->setField(std::make_shared<AST::Variable>(*$3, nullptr));
		delete $3;
		var->getField()->setLocation(makeSourceLoc(&@3));
	}
	;
		
pointermark: { $$ = new std::string(); } | pointermarklist { $$ = $1; }
pointermarklist: '@' { $$ = new std::string("@"); }
		| '@' pointermarklist { $$ = $2; *$$ += "@"; }
		;

// namelist:		Name | namelist ',' Name
		//;

explist:	exp { $$ = new ExprList; $$->push_back(std::shared_ptr<AST::Expr>($1)); } 
		| explist ',' exp
		{
			$$ = $1;
			$$->push_back(std::shared_ptr<AST::Expr>($3));
		}
		;

exp:	'(' exp ')' { $$ = $2; }
		| 		'@' exp { $$ = new AST::UnaryOp(std::shared_ptr<AST::Expr>($2), "@"); $$->setLocation(makeSourceLoc(&@1));}
		| 		'$' exp { $$ = new AST::UnaryOp(std::shared_ptr<AST::Expr>($2), "$"); $$->setLocation(makeSourceLoc(&@1));}
		| 		exp Operator exp { $$ = new AST::BinaryOp(std::shared_ptr<AST::Expr>($1), std::shared_ptr<AST::Expr>($3), *$2); $$->setLocation(makeSourceLoc(&@2)); }
		| 		exp '+' exp { $$ = new AST::BinaryOp(std::shared_ptr<AST::Expr>($1), std::shared_ptr<AST::Expr>($3), "+"); $$->setLocation(makeSourceLoc(&@2)); }
		| 		exp '-' exp { $$ = new AST::BinaryOp(std::shared_ptr<AST::Expr>($1), std::shared_ptr<AST::Expr>($3), "-"); $$->setLocation(makeSourceLoc(&@2)); }
		| 		exp '*' exp { $$ = new AST::BinaryOp(std::shared_ptr<AST::Expr>($1), std::shared_ptr<AST::Expr>($3), "*"); $$->setLocation(makeSourceLoc(&@2)); }
		| 		exp '/' exp { $$ = new AST::BinaryOp(std::shared_ptr<AST::Expr>($1), std::shared_ptr<AST::Expr>($3), "/"); $$->setLocation(makeSourceLoc(&@2)); }
		| 		exp '=' exp { $$ = new AST::BinaryOp(std::shared_ptr<AST::Expr>($1), std::shared_ptr<AST::Expr>($3), "="); $$->setLocation(makeSourceLoc(&@2)); }
		| 		exp '<' exp { $$ = new AST::BinaryOp(std::shared_ptr<AST::Expr>($1), std::shared_ptr<AST::Expr>($3), "<"); $$->setLocation(makeSourceLoc(&@2)); }
		| 		exp '>' exp { $$ = new AST::BinaryOp(std::shared_ptr<AST::Expr>($1), std::shared_ptr<AST::Expr>($3), ">"); $$->setLocation(makeSourceLoc(&@2)); }
		| 		exp LEQ exp { $$ = new AST::BinaryOp(std::shared_ptr<AST::Expr>($1), std::shared_ptr<AST::Expr>($3), "<="); $$->setLocation(makeSourceLoc(&@2)); }
		| 		exp GEQ exp { $$ = new AST::BinaryOp(std::shared_ptr<AST::Expr>($1), std::shared_ptr<AST::Expr>($3), ">="); $$->setLocation(makeSourceLoc(&@2)); }
		| 		exp EQ exp { $$ = new AST::BinaryOp(std::shared_ptr<AST::Expr>($1), std::shared_ptr<AST::Expr>($3), "=="); $$->setLocation(makeSourceLoc(&@2)); }
		| 		exp NEQ exp { $$ = new AST::BinaryOp(std::shared_ptr<AST::Expr>($1), std::shared_ptr<AST::Expr>($3), "~="); $$->setLocation(makeSourceLoc(&@2)); }

		| 		Number { $$ = new AST::Number($1); $$->setLocation(makeSourceLoc(&@1)); } 
		| 		Integer { $$ = new AST::Integer($1); $$->setLocation(makeSourceLoc(&@1)); }
		| 		Bool { $$ = new AST::Bool($1); $$->setLocation(makeSourceLoc(&@1)); }
		| 		Char { $$ = new AST::Byte($1); $$->setLocation(makeSourceLoc(&@1)); }
		| 		var { $$ = $1; $$->setLocation(makeSourceLoc(&@1)); }
		| 		var '[' exp ']' { $$ = $1; static_cast<AST::Variable*>($1)->setIndex(std::shared_ptr<AST::Expr>($3)); $$->setLocation(makeSourceLoc(&@1)); }
		| 		LiteralString { auto str = new AST::String(*$1); str->unescape(); $$ = str; delete $1; $$->setLocation(makeSourceLoc(&@1)); }
		
		|		'<' pointermark Name '>' exp
				{
					$$ = new AST::TypeCast(*$2 + *$3, std::shared_ptr<AST::Expr>($5));
					$$->setLocation(makeSourceLoc(&@5));

					delete $2;
					delete $3;
				}
		| 		functioncall
				{
					//auto call = $1; // new AST::FunctionCall($1->Type, $1->IsMethod);
					//$$ = call;
					//call->getArgs() = *$1->Args;
					//delete $1;

					$$ = $1;
					$$->setLocation(makeSourceLoc(&@1)); 
				}
		| 		Operator exp
				{ 
					$$ = new AST::UnaryOp(std::shared_ptr<AST::Expr>($2), *$1); 
					delete $1; 
					$$->setLocation(makeSourceLoc(&@1)); 
				}
		;

//prefixexp:		 var | functioncall | '(' exp ')'
//		;

functioncall:	//prefixexp args 
		//| prefixexp ':' Name args
		Name args 
		{
			AST::FunctionCall* call;
			$$ = call = new AST::FunctionCall(*$1);
			call->getArgs() = std::move(*$2);
			delete $1;
			//delete $2;
		}
		| var ':' Name args
		{
			AST::FunctionCall* call;
			$$ = call = new AST::FunctionCall(*$3, true);
			call->getArgs() = std::move(*$4);
			
			std::reverse(call->getArgs().begin(), call->getArgs().end());
			call->getArgs().push_back(std::shared_ptr<AST::Expr>($1));
			std::reverse(call->getArgs().begin(), call->getArgs().end());

			delete $3;
		}
		| var '.' Name args
		{
			$$ = $1;
			AST::Variable* var = $1;

			// Search last in linked list
			while(var->getField())
				var = var->getField().get();

			auto call = new AST::FunctionCall(*$3);
			call->getArgs() = std::move(*$4);
			delete $3;

			var->setFunctionCall(std::shared_ptr<AST::FunctionCall>(call));
		}
		;

args:	'(' explist ')' { $$ = $2; }
	| '(' ')' { $$ = new ExprList; }
	;

funcbody:	'(' parlist ')' ArrowRight pointermark Name block End 
			{ 
				$$ = new FunctionBody(); 
				$$->Type = *$5 + *$6;
				$$->Body = $7;
				$$->Args = $2;
				
				delete $5;
				delete $6;
			}
			
		| '(' parlist ',' ThreeDot ')' ArrowRight pointermark Name block End 
			{ 
				$$ = new FunctionBody(); 
				$$->Type = *$7 + *$8;
				$$->Body = $9;
				$$->Args = $2;
				$$->IsVariadic = true;
				
				delete $7;
				delete $8;
			}
			
		| '(' ThreeDot ')' ArrowRight pointermark Name block End 
			{ 
				$$ = new FunctionBody(); 
				$$->Type = *$5 + *$6;
				$$->Body = $7;
				$$->Args = new ExprList;
				$$->IsVariadic = true;
				
				delete $6;
				delete $5;
			}
		;

parlist: { $$ = new ExprList; }
	| pointermark Name Name { $$ = new ExprList; $$->push_back(std::make_shared<AST::VariableDef>(*$3, *$1 + *$2, nullptr)); delete $1; delete $2; delete $3; }
	| parlist ',' pointermark Name Name { $$ = $1; $$->push_back(std::make_shared<AST::VariableDef>(*$5, *$3 + *$4, nullptr)); delete $3; delete $4; delete $5; }
	;

%%

#define LEXER_IMPLEMENTED
#include <iostream>
static bool parserError = false;

extern int yylex_init(void**);
extern int yylex_destroy(void*);
extern void yyset_in(FILE*, void*);
extern FILE* yyget_in(void*);

int parse(const std::string& file, void* scanner, const AST::CompilationFlags& flags)
{
	std::string path = file;
	if(file[0] == '/')
	{
		path.erase(path.find_last_of("/") + 1);
		ast->setSourcePath(path);
	}
	else
	{
		int idx = path.find_last_of("/");
		if(idx != -1)
		{
			std::string pwd;
			char buf[256];
			getcwd(buf, sizeof(buf));
			pwd = buf;
			
			path.erase(idx + 1);
			path = "/" + path;

			ast->setSourcePath(buf + path);
		}
	}
		
	//ast->setSourcePath(path);
	ast->setSourceName(file);
	
	FILE* yyin = yyget_in(scanner);
	
	parserError = false;
	do {
#ifdef LEXER_IMPLEMENTED
		yyparse(scanner);

		if(parserError)
			return 1;
#else
		int x;
		std::cout << "Resulting tokens: ";
		while (x = yylex())
		{
			std::cout << "<" << yytext << "> ";
		}
		std::cout << std::endl;
#endif

	} while(!feof(yyin));
#ifndef LEXER_IMPLEMENTED
	exit(EXIT_SUCCESS);
#endif

	//ast->dump();
	int idx = file.find_last_of('/');
	std::string fname;
	if(idx != -1)
		fname = file.substr(idx + 1);
	else
		fname = file;
	
	fname.erase(fname.find_last_of('.'));

	SemanticChecker checker;
	checker.check(*ast);

	ast->writeLlvm(flags.output + ".raw.ll");
	ast->writeModule(flags.output + ".lmod");

	system(("opt -S -O3 " + flags.output + ".raw.ll -o " + flags.output + ".ll ").c_str());

	if(!flags.isModule)
		system(("clang -O3 -march=native -Wno-override-module " + ast->getRequiredLibraries() + " " + flags.output + ".ll -o " + flags.output).c_str());
	return 0;
}

int parse(FILE *fp, const AST::CompilationFlags& flags)
{
	ast->setFlags(flags);
	ast->setIncludeCallback([] (const std::string& file) -> std::shared_ptr<AST::Module> {
		FILE* fp = fopen(file.c_str(), "r");
		if(!fp)
			return nullptr;
		
		std::shared_ptr<AST::Module> oldAst = ast;
		std::shared_ptr<AST::Module> newAst = std::make_shared<AST::Module>();
		ast = newAst;
		ast->setFlags(oldAst->getFlags());

		void* scanner;
		yylex_init(&scanner);
		yyset_in(fp, scanner);
		
		int retval = yyparse(scanner);
		yylex_destroy(scanner);
		
		ast = oldAst;
		
		fclose(fp);
		return newAst;
	});
	
	void* scanner;
	yylex_init(&scanner);
	yyset_in(fp, scanner);
	
	int retval = parse(flags.input, scanner, flags);
	yylex_destroy(scanner);

	return retval;
}

void yyerror(YYLTYPE* locp, void*, char const* msg)
{
	//std::cout << "ERROR: " << locp->last_column << " STUFF " << msg << " at line " << yylineno << " ('" << yytext << "')" << std::endl;
	ast->error(msg, makeSourceLoc(locp));
	parserError = true;
	std::exit(EXIT_FAILURE);
}

