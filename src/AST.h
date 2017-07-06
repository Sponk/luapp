#pragma once

#include <vector>
#include <string>
#include <memory>
#include <iostream>
#include <fstream>
#include <stack>
#include <unordered_map>
#include <sstream>

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>

//#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/Bitcode/ReaderWriter.h>

#include <llvm/Support/FileSystem.h>
#include <llvm/IR/Argument.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Linker/Linker.h>

namespace AST
{

class SourceLocation
{
	size_t Line;
	size_t Col;
	size_t Size;
public:
	SourceLocation() : SourceLocation(0, 0, 0) {}
	SourceLocation(size_t line, size_t col, size_t size)
		: Line(line), Col(col), Size(size) {}
		
	size_t getLine() const { return Line; }
	size_t getCol() const { return Col; }
	size_t getSize() const { return Size; }
	
	void dump() const
	{
		std::cout << "SourceLocation: Line " << Line << " Col " << Col << " Size " << Size << std::endl;
	}
};

class Expr
{
	SourceLocation Location;
public:
	virtual ~Expr() {}
	virtual void dump()
	{
		std::cout << "Expr" << std::endl;
	}
	
	virtual std::string getDefinitionString()
	{
		return "";
	}
	
	SourceLocation getLocation() { return Location; }
	void setLocation(const SourceLocation& loc) { Location = loc; }
};

class TypeCast : public Expr
{
	std::shared_ptr<Expr> Value;
	std::string Type;
public:
	TypeCast(const std::string& type, const std::shared_ptr<Expr>& value)
		: Type(type), Value(value) {}
		
	std::string getType() { return Type; }
	std::shared_ptr<Expr> getValue() { return Value; }
};

class Number : public Expr
{
	float Value;
public:
	Number(float value) : Value(value) {}
	float getValue() const { return Value; }
	void dump() override { std::cout << "Number: " << Value << std::endl; }
};

class Integer : public Expr
{
	float Value;
public:
	Integer(int value) : Value(value) {}
	int getValue() const { return Value; }
	void dump() override { std::cout << "Integer: " << Value << std::endl; }
};

class Bool : public Expr
{
	bool Value;
public:
	Bool(bool value) : Value(value) {}
	bool getValue() const { return Value; }
	void dump() override { std::cout << "Bool: " << Value << std::endl; }
};

class Byte : public Expr
{
	char Value;
public:
	Byte(char value) : Value(value) {}
	char getValue() const { return Value; }
	void dump() override { std::cout << "Byte: " << Value << std::endl; }
};

class String : public Expr
{
	std::string Value;
public:
	String(const std::string& value) : Value(value) {}
	std::string getValue() const { return Value; }
	void setValue(const std::string& value) { Value = value; }
	void dump() override { std::cout << "String: '" << Value << "'" << std::endl; }
	
	void unescape()
	{
		std::string escaped;
		for(size_t i = 0; i < Value.size(); i++)
		{
			if(Value[i] == '\\')
				switch(Value[++i])
				{
					case 'n': escaped += '\n'; break;
					case 't': escaped += '\t'; break;
					case 'b': escaped += '\b'; break;
					default: escaped += "\\" + Value[i];
				}
			else
				escaped += Value[i];
		}
		
		Value = escaped;
	}
};

class If : public Expr
{
	std::shared_ptr<Expr> Head;
	std::vector<std::shared_ptr<Expr>> Body;
	std::vector<std::shared_ptr<Expr>> Else;
public:
	If(std::shared_ptr<Expr> head) : Head(head) {}
	
	void dump() override
	{
		std::cout << "If\n";
		std::cout << "Head:\n";
		Head->dump();
		
		std::cout << "Body:\n";
		for(auto& k : Body)
			k->dump();
		
		std::cout << "Else:\n";
		for(auto& k : Else)
			k->dump();
	}

	std::vector<std::shared_ptr<Expr>>& getElse() { return Else; }
	std::vector<std::shared_ptr<Expr>>& getBody() { return Body; }
	std::shared_ptr<Expr>& getHead() { return Head; }
};

class While : public Expr
{
	std::shared_ptr<Expr> Head;
	std::vector<std::shared_ptr<Expr>> Body;
public:
	While(std::shared_ptr<Expr> head) : Head(head) {}
	
	void dump() override
	{
		std::cout << "While\n";
		std::cout << "Head:\n";
		Head->dump();
		
		std::cout << "Body:\n";
		for(auto& k : Body)
			k->dump();
	}

	std::vector<std::shared_ptr<Expr>>& getBody() { return Body; }
	std::shared_ptr<Expr>& getHead() { return Head; }
};

class For : public Expr
{
	std::shared_ptr<Expr> Init;
	std::shared_ptr<Expr> Cond;
	std::shared_ptr<Expr> Inc;
	std::vector<std::shared_ptr<Expr>> Body;
public:
	For(std::shared_ptr<Expr> init, std::shared_ptr<Expr> cond, std::shared_ptr<Expr> inc) 
		: Init(init), Cond(cond), Inc(inc) {}
	
	void dump() override
	{
		std::cout << "For\n";
		std::cout << "Init:\n";
		Init->dump();
		
		std::cout << "Cond:\n";
		Cond->dump();
		
		std::cout << "Inc:\n";
		Inc->dump();
		
		std::cout << "Body:\n";
		for(auto& k : Body)
			k->dump();
	}

	std::vector<std::shared_ptr<Expr>>& getBody() { return Body; }
	std::shared_ptr<Expr>& getInit() { return Init; }
	std::shared_ptr<Expr>& getCond() { return Cond; }
	std::shared_ptr<Expr>& getInc() { return Inc; }
};

class VariableDef : public Expr
{
	std::string Name;
	std::string Type;
	std::shared_ptr<Expr> Initial;
	unsigned int Size; // Array size
public:
	VariableDef(const std::string& name, const std::string& type, std::shared_ptr<Expr> initial, unsigned int size = 0) 
		: Name(name), Type(type), Initial(initial), Size(size) {}
	
	std::string getName() const { return Name; }
	std::string getType() const { return Type; }
	unsigned int getSize() const { return Size; }
	std::shared_ptr<Expr>& getInitial() { return Initial; }
	void dump() override { std::cout << "Variable Definition: '" << Name << "' as '" << Type << "'" << std::endl; }
	
	std::string getDefinitionString() override
	{
		return "local " + Name + " -> " + Type + (Size > 0 ? "[" + std::to_string(Size) + "]" : "") + "\n";
	}
};

class Function : public Expr
{
	bool Extern;
	std::string Name;
	std::string ReturnType;
	std::vector<std::shared_ptr<Expr>> Body;
	std::vector<std::shared_ptr<Expr>> Args;
	
	bool Variadic;
public:
	Function(const std::string& name, const std::string& ret, bool ext = false) 
		: Name(name), ReturnType(ret), Extern(ext), Variadic(false) {}
	
	void dump() override
	{
		std::cout << "Function '" << Name << "'\n";
		std::cout << "Arguments:\n";
		for(auto& k : Args)
			k->dump();
		
		std::cout << "Body:\n";
		for(auto& k : Body)
			k->dump();
	}
	
	bool getVariadic() const { return Variadic; }
	void setVariadic(bool value) { Variadic = value; }
	
	bool getExtern() const { return Extern; }
	
	const std::string getName() const { return Name; }
	const std::string getReturnType() const { return ReturnType; }
	std::vector<std::shared_ptr<Expr>>& getBody() { return Body; }
	std::vector<std::shared_ptr<Expr>>& getArgs() { return Args; }

	void setName(const std::string& name) { Name = name; }
	std::string getDefinitionString() override
	{
		if(Extern)
			return "";
		
		std::stringstream ss;
		ss << "function " << Name << "(";
		for(auto& k : Args)
		{
			VariableDef* v = static_cast<VariableDef*>(k.get());
			ss << v->getType() << " " << v->getName() << (k != Args.back() ? ", " : "");
		}
		
		ss << ") -> " << ReturnType << "\n";
		return ss.str();
	}
};

class FunctionCall : public Expr
{
	std::string Name;
	std::vector<std::shared_ptr<Expr>> Args;
	bool IsMethod = false;
public:
	FunctionCall(const std::string& name, bool isMethod = false) 
	: Name(name), IsMethod(isMethod) {}
	
	void dump() override
	{
		std::cout << "Function Call '" << Name << "'\n";
		for(auto& k : Args)
			k->dump();
	}
	
	bool isMethod() const { return IsMethod; }
	const std::string getName() const { return Name; }
	std::vector<std::shared_ptr<Expr>>& getArgs() { return Args; }
};

class BinaryOp : public Expr
{
	std::shared_ptr<Expr> Left;
	std::shared_ptr<Expr> Right;
	std::string Op;
public:
	BinaryOp(const std::shared_ptr<Expr>& left, 
		 const std::shared_ptr<Expr>& right, 
		 const std::string& op) : Left(left), Right(right), Op(op) {}
		 
	std::shared_ptr<Expr>& getLeft() { return Left; }
	std::shared_ptr<Expr>& getRight() { return Right; }
	std::string getOp() { return Op; }
	
	void dump() override
	{
		std::cout << "BinaryOp: '" << Op << "'" << std::endl;
		Left->dump();
		Right->dump();
	}
};

class UnaryOp : public Expr
{
	std::shared_ptr<Expr> Exp;
	std::string Op;
public:
	UnaryOp(const std::shared_ptr<Expr>& exp, 
		 const std::string& op) : Exp(exp), Op(op) {}
		 
	std::shared_ptr<Expr>& getExp() { return Exp; }
	std::string getOp() { return Op; }
	
	void dump() override
	{
		std::cout << "UnaryOp: '" << Op << "'" << std::endl;
		Exp->dump();
	}
};

class Return : public Expr
{
	std::shared_ptr<Expr> Value;
public:
	Return(const std::shared_ptr<Expr>& value) : Value(value) {}
	std::shared_ptr<Expr>& getValue() { return Value; }
	void dump() override { std::cout << "Return" << std::endl; if(Value) Value->dump(); }
};

class Variable : public Expr
{
	std::string Name;
	std::shared_ptr<Expr> Index;
	std::shared_ptr<Variable> Field;
public:
	Variable(const std::string& name, std::shared_ptr<Variable> field, std::shared_ptr<Expr> index = nullptr)
		: Name(name), Field(field), Index(index) {}
	std::string getName() const { return Name; }
	std::shared_ptr<Expr> getIndex() const { return Index; }
	std::shared_ptr<Variable> getField() const { return Field; }

	void setField(std::shared_ptr<Variable> field) { Field = field; }	
	void setIndex(std::shared_ptr<Expr> idx) { Index = idx; }
	
	void dump() override 
	{ 
		std::cout << "Variable: '" << Name << "'[" << Index << "]" << std::endl; 
		if(Field)
			Field->dump();
	}
};

class Label : public Expr
{
	std::string Name;
public:
	Label(const std::string& name) : Name(name) {}
	std::string getName() const { return Name; }
	void dump() override { std::cout << "Label: '" << Name << "'" << std::endl; }
};

class Goto : public Expr
{
	std::string Name;
public:
	Goto(const std::string& name) : Name(name) {}
	std::string getName() const { return Name; }
	void dump() override { std::cout << "Goto: '" << Name << "'" << std::endl; }
};

class ClassDef : public Expr
{
	std::string Name;
	std::vector<std::shared_ptr<Expr>> Body;
	
	std::vector<std::shared_ptr<VariableDef>> Fields;
	std::vector<std::shared_ptr<Function>> Methods;
public:
	ClassDef(const std::string& name) : Name(name) {}
	std::string getName() const { return Name; }
	std::vector<std::shared_ptr<Expr>>& getBody() { return Body; }
	
	std::vector<std::shared_ptr<VariableDef>>& getFields() { return Fields; }
	std::vector<std::shared_ptr<Function>>& getMethods() { return Methods; }
	
	int getMemberIdx(const std::string& name)
	{
		unsigned int i = 0;
		for(auto& v : Fields)
			if(v->getName() == name)
				return i;
			else
				i++;
			
		return -1;
	}
	
	std::shared_ptr<VariableDef> getMember(const std::string& name)
	{
		for(auto& v : Fields)
			if(v->getName() == name)
				return v;
		return nullptr;
	}
	
	void dump() override
	{
		std::cout << "ClassDef: '" << Name << std::endl;
		for(auto& k : Body)
			k->dump();
	}
	
	std::string getDefinitionString() override
	{
		std::stringstream ss;
		ss << "class " << Name << " {\n";
		for(auto& k : Body)
		{
			ss << "\t" << k->getDefinitionString();
		}
		ss << "}\n";
		return ss.str();
	}
};

class Module
{
	std::vector<std::shared_ptr<Expr>> TopLevel;
	
	struct LocalScope
	{
		typedef std::unordered_map<std::string, llvm::Value*> Scope;
		std::vector<Scope*> LocalVariables;
		std::unordered_map<std::string, std::shared_ptr<ClassDef>> Classes;
		
		void addLevel(Scope* scope)
		{
			LocalVariables.push_back(scope);
		}
		
		void exit()
		{
			LocalVariables.pop_back();
		}
		
		llvm::Value* find(const std::string& str)
		{
			for(auto i = LocalVariables.rbegin(); i != LocalVariables.rend(); i++)
			{
				Scope& scope = **i;
				auto result = scope.find(str);
				if(result != scope.end())
					return result->second;
			}
			
			return nullptr;
		}
		
		Scope& current() { return *LocalVariables.back(); }
	};
	
	inline llvm::Value* var2val(llvm::IRBuilder<>& builder, llvm::Value* v)
	{
		return (llvm::isa<llvm::AllocaInst>(v) || v->getType()->isPointerTy() ? builder.CreateLoad(v) : v);
	}

	llvm::LLVMContext context;
	std::string SourceName;
	std::string SourcePath;
	unsigned int ErrorCount = 0;
	
	std::function<std::shared_ptr<Module>(const std::string&)> IncludeCallback = [](const std::string&) { return nullptr; };
public:
	void dump()
	{
		std::cout << "Dumping module:" << std::endl;
		for(auto& k : TopLevel)
			k->dump();
	}
	
	void setIncludeCallback(const std::function<std::shared_ptr<Module>(const std::string&)>& func) { IncludeCallback = func; }
	void setSourceName(const std::string& name) { SourceName = name; }
	void setSourcePath(const std::string& name) { SourcePath = name; }
	
	void addExpr(const std::shared_ptr<Expr>& expr)
	{
		TopLevel.push_back(expr);
	}
	
	llvm::Value* generateIr(std::shared_ptr<Expr> k, LocalScope& scope, llvm::IRBuilder<>& builder, llvm::Module* module)
	{
		LocalScope::Scope currentScope;
		scope.addLevel(&currentScope);
		
		Function* function = dynamic_cast<Function*>(k.get());
		if(function)
		{	
			std::vector<llvm::Type*> args;
			
			for(auto& p : function->getArgs())
				args.push_back(getType(builder, dynamic_cast<VariableDef*>(p.get())->getType(), module));
			
			llvm::ArrayRef<llvm::Type*>  argsRef(args);
			llvm::FunctionType* funcType = getFunctionTypeFromName(builder, module, function->getReturnType(), function->getVariadic(), argsRef);
			llvm::Function* llvmFunction = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, function->getName(), module);
			
			if(!function->getExtern())
			{
				llvm::BasicBlock* entry = llvm::BasicBlock::Create(context, function->getName() + "_entrypoint", llvmFunction);
				builder.SetInsertPoint(entry);
				
				{
					unsigned int i = 0;
					for(auto& param : llvmFunction->getArgumentList())
					{
						const std::string& name = dynamic_cast<VariableDef*>(function->getArgs()[i++].get())->getName();
						param.setName(name);
						
						currentScope[name] = builder.CreateAlloca(param.getType(), 0, (name + "_local"));
						builder.CreateStore(&param, currentScope[name]);
					}
				}
					
				generateIr(function->getBody(), scope, builder, module);
				if(funcType->getReturnType()->isVoidTy())
					builder.CreateRetVoid();
			}

			scope.exit();
			return llvmFunction;
		}
		
		BinaryOp* binop = dynamic_cast<BinaryOp*>(k.get());
		if(binop)
		{
			llvm::Value* retval = nullptr;
			llvm::Value* left = generateIr(binop->getLeft(), scope, builder, module);
			llvm::Value* right = generateIr(binop->getRight(), scope, builder, module);
			
			if(!left || !right)
				return nullptr;
			
			if(binop->getOp().size() == 1)			
				switch(binop->getOp()[0])
				{
					case '+':
						left = var2val(builder, left); right = var2val(builder, right);
						if(right->getType()->isFloatingPointTy())
							retval = builder.CreateFAdd(left, right, "fadd");
						else
							retval = builder.CreateAdd(left, right, "add");
					break;
						
					case '-':
						left = var2val(builder, left); right = var2val(builder, right);
						if(right->getType()->isFloatingPointTy())
							retval = builder.CreateFSub(left, right, "fsub");
						else
							retval = builder.CreateSub(left, right, "sub");
						break;
						
					case '*':
						left = var2val(builder, left); right = var2val(builder, right);
						if(right->getType()->isFloatingPointTy())
							retval = builder.CreateFMul(left, right, "fmul");
						else
							retval = builder.CreateMul(left, right, "mul");
						break;
						
					case '/':
						left = var2val(builder, left); right = var2val(builder, right);
						if(left->getType()->isFloatingPointTy())
							retval = builder.CreateFDiv(left, right, "fdiv");
						else
							retval = builder.CreateExactSDiv(left, right, "div");
						break;
					
					case '>':
						left = var2val(builder, left); right = var2val(builder, right);
						if(left->getType()->isFloatingPointTy())
							retval = builder.CreateFCmpOGT(left, right, "fcmpgt");
						else
							retval = builder.CreateICmpSGT(left, right, "cmpgt");
						break;
						
					case '<':
						left = var2val(builder, left); right = var2val(builder, right);
						if(left->getType()->isFloatingPointTy())
							retval = builder.CreateFCmpOLT(left, right, "fcmplt");
						else
							retval = builder.CreateICmpSLT(left, right, "cmplt");
						break;
						
					case '=':
						/*if(llvm::isa<llvm::AllocaInst>(right))
						{
							right = var2val(builder, right);
						}*/
						
						left = static_cast<llvm::LoadInst*>(left)->getPointerOperand();
						if(!left)
						{
							error("left assignment operand is not a variable", binop->getLocation());
							return nullptr;
						}
						
						// Places have to be switched: The value is on the left and pointer on the right
						if(left->getType()->getPointerElementType() != right->getType())
						{	error("assignment expected '" 
								+ type2str(left->getType()->getPointerElementType())
								+ "' but got '" + type2str(right->getType()) + "'",
								binop->getLocation());
							//	llvm::report_fatal_error("Assignment type mismatch!");
						
							return nullptr;
						}
						
						//left = builder.CreateGEP(left, 0);
						if(!left->getType()->isPointerTy())
						{
							llvm::report_fatal_error("Can only store into references!");
						}
						
						retval = builder.CreateStore(right, left);
						break;
				}
			else
				if(binop->getOp() == "==")
				{
					if(left->getType()->isPointerTy())
						left = builder.CreatePtrToInt(left, builder.getInt32Ty(), "left_ptr_to_int");
					
					if(right->getType()->isPointerTy())
						right = builder.CreatePtrToInt(right, builder.getInt32Ty(), "right_ptr_to_int");
					
					module->dump();
					error("comparison expected '" 
								+ type2str(left->getType()->getPointerElementType())
								+ "' but got '" + type2str(right->getType()) + "'",
								binop->getLocation());
					
					if(left->getType()->isFloatingPointTy())
						retval = builder.CreateFCmpOEQ(left, right, "fcmp");
					else
						retval = builder.CreateICmpEQ(left, right, "cmp");
				}
				else if(binop->getOp() == "<=")
				{
					left = var2val(builder, left); right = var2val(builder, right);
					if(left->getType()->isFloatingPointTy())
						retval = builder.CreateFCmpOLE(left, right, "fcmpleq");
					else
						retval = builder.CreateICmpSLE(left, right, "cmpleq");
				}
				else if(binop->getOp() == ">=")
				{
					left = var2val(builder, left); right = var2val(builder, right);
					if(left->getType()->isFloatingPointTy())
						retval = builder.CreateFCmpOGE(left, right, "fcmpgeq");
					else
						retval = builder.CreateICmpSGE(left, right, "cmpgeq");
				}
				else if(binop->getOp() == "~=")
				{				
					if(left->getType()->isPointerTy())
						left = builder.CreatePtrToInt(left, builder.getInt32Ty(), "left_ptr_to_int");
					
					if(right->getType()->isPointerTy())
						right = builder.CreatePtrToInt(right, builder.getInt32Ty(), "right_ptr_to_int");
					
					if(left->getType()->isFloatingPointTy())
						retval = builder.CreateFCmpONE(left, right, "fcmpneq");
					else
						retval = builder.CreateICmpNE(left, right, "cmpneq");
				}
			
			if(retval == nullptr)
			{
				llvm::Function* function = module->getFunction(getOperatorName(binop->getOp(),
												type2str(left->getType()),
												type2str(right->getType())));

				if(!function)
					llvm::report_fatal_error("Could not find fitting operator implementation!");

				std::vector<llvm::Value*> args;
				args.push_back(var2val(builder, left));
				args.push_back(var2val(builder, right));

				llvm::ArrayRef<llvm::Value*> argRef(args);
				retval = builder.CreateCall(function, argRef, "call");
			}
			
			scope.exit();
			return retval;
		}
		
		if(auto op = dynamic_cast<UnaryOp*>(k.get()))
		{
			llvm::Value* retval = nullptr;
			llvm::Value* operand = generateIr(op->getExp(), scope, builder, module);
			
			if(!operand) return nullptr;
			
			if(op->getOp().size() == 1)
				switch(op->getOp()[0])
				{
					case '~':
						if(operand->getType() != getType(builder, "bool"))
							llvm::report_fatal_error("Type mismatch: Expected bool!");
						
						retval = builder.CreateNot(operand, "not");
						break;
						
					case '-':
						retval = builder.CreateNeg(operand, "neg");
						break;
					
					case '@':
						operand = static_cast<llvm::LoadInst*>(operand)->getPointerOperand();
						if(operand->getType()->isPointerTy())
						{
							retval = builder.CreateGEP(operand->getType()->getPointerElementType(), operand, builder.getInt32(0), "@gep");
							//retval = builder.CreatePointerCast(retval, retval->getType(), "@cast");
						}
						else
							error("can not take the address of a literal", op->getLocation());
							//llvm::report_fatal_error("Can't take the address of a literal!");
						break;
						
					case '$':
						if(operand->getType()->isPointerTy())
							retval = builder.CreateLoad(operand, "ptrload");
						else
							llvm::report_fatal_error("Can't load the value of a literal!");
						break;
						
					default:
						error("operator '" + op->getOp() + "' is undefined!", op->getLocation());
						return nullptr;
				}
				
			scope.exit();
			return retval;
		}
		
		if(auto var = dynamic_cast<Variable*>(k.get()))
		{
			scope.exit();
			llvm::Value* v = scope.find(var->getName());
			
			if(!v)
			{
				error("undefined variable '" + var->getName() + "'", var->getLocation());
			}
			
			
			if(var->getIndex() != 0 && !v->getType()->isPointerTy())
			{
				error("can not index scalar values", var->getLocation());
				return nullptr;
			}
			else if(var->getIndex() != nullptr)
			{
				auto index = var->getIndex();
				llvm::Value* indexValue = generateIr(index, scope, builder, module);
				if(!indexValue)
					return nullptr;
				
				if(!v->getType()->getPointerElementType()->isArrayTy())
					v = builder.CreateGEP(var2val(builder, v), var2val(builder, indexValue), "array_gep");
				else
				{
					auto pelem = v->getType()->getPointerElementType()->getArrayElementType()->getPointerTo();//->dump();
					v = builder.CreateGEP(v, var2val(builder, indexValue), "pointer_array_gep");
					v = builder.CreatePointerCast(v, pelem, "pointercast");
				}
			}
			
			while(var->getField())
			{
				if(!llvm::isa<llvm::StructType>(v->getType()->getPointerElementType()))
				{
					v = builder.CreateLoad(v, var->getName() + "_implicit_deref");
					if(!llvm::isa<llvm::StructType>(v->getType()->getPointerElementType()))
					{
						error("can not access a field of a non-class object", var->getLocation());
						return nullptr;
					}
				}
				
				auto structType = llvm::dyn_cast<llvm::StructType>(v->getType()->getPointerElementType());
				if(!structType)
				{
					error("can not access a field of a non-class object", var->getLocation());
					return nullptr;
				}
				
				std::string name = structType->getName();
				ClassDef* classdef = scope.Classes[name].get();
				if(!classdef)
				{
					error("class '" + name + "' is undefined", var->getLocation());
					return nullptr;
				}
				
				Variable* field = var->getField().get();
				VariableDef* fieldDef = classdef->getMember(field->getName()).get();
				
				if(!fieldDef)
				{
					error("field '" + field->getName() + "' is not a member of class '" + name + "'", field->getLocation());
					return nullptr;
				}
				
				int fieldIndex = classdef->getMemberIdx(field->getName());
				llvm::Type* fieldType = getType(builder, fieldDef->getType(), module);
				
				v = builder.CreateGEP(v, builder.getInt32(fieldIndex), field->getName() + "_gep");
				v = builder.CreatePointerCast(v, fieldType->getPointerTo(), field->getName() + "_cast");
				var = field;
			}
			
			return builder.CreateLoad(v);
		}
		
		if(auto var = dynamic_cast<VariableDef*>(k.get()))
		{
			// Variable needs to be visible to the parent scope
			scope.exit();
			if(scope.current().find(var->getName()) != scope.current().end())
				error("variable name collision", var->getLocation());

			// Auto type
			if(var->getInitial() && var->getType().empty())
			{
				llvm::Value* initial = generateIr(var->getInitial(), scope, builder, module);
				if(!initial) return nullptr;
				
				llvm::Value* llvmVar = builder.CreateAlloca(initial->getType(), 0, var->getName());
				
				scope.current()[var->getName()] = llvmVar;
				return builder.CreateStore(initial, llvmVar, "var_init");
			}
			else
			{
				llvm::Value* initial = nullptr;
				if(var->getInitial())
					initial = generateIr(var->getInitial(), scope, builder, module);
				
				llvm::Type* type = getType(builder, var->getType(), module);
				if(initial && initial->getType() != type)
				{
					error("variable type mismatch, expected " + type2str(type) + " but got " + type2str(initial->getType()), var->getInitial()->getLocation());
				}
				
				llvm::Value* llvmVar = nullptr;
				if(var->getSize() == 0)
					llvmVar = builder.CreateAlloca(type, 0, var->getName());
				else
					llvmVar = builder.CreateAlloca(llvm::ArrayType::get(type, var->getSize()), 0, var->getName());
				
				scope.current()[var->getName()] = llvmVar;
				return (initial ? builder.CreateStore(initial, llvmVar, "var_init_typed") : llvmVar);
			}

			error("could not define variable", var->getLocation());
		}
		
		if(auto label = dynamic_cast<Label*>(k.get()))
		{
			scope.exit();

			llvm::Function* function = builder.GetInsertBlock()->getParent();
			llvm::BasicBlock* block = llvm::BasicBlock::Create(context, label->getName(), function);
			scope.current()[label->getName()] = block;
			
			builder.CreateBr(block);
			builder.SetInsertPoint(block);
			
			return block;
		}
		
		if(auto iffi = dynamic_cast<If*>(k.get()))
		{
			scope.exit();

			llvm::Function* function = builder.GetInsertBlock()->getParent();
			llvm::BasicBlock* if_true = llvm::BasicBlock::Create(context, "if_true", function);			
			llvm::BasicBlock* if_false = llvm::BasicBlock::Create(context, "if_false", function);
			llvm::BasicBlock* if_continue = llvm::BasicBlock::Create(context, "if_continue", function);

			llvm::Value* condition = generateIr(iffi->getHead(), scope, builder, module);
			if(!condition) return nullptr;
			
			auto branch = builder.CreateCondBr(var2val(builder, condition), if_true, if_false);
			builder.SetInsertPoint(if_true);
			generateIr(iffi->getBody(), scope, builder, module);
			builder.CreateBr(if_continue);
			
			builder.SetInsertPoint(if_false);
			generateIr(iffi->getElse(), scope, builder, module);
			builder.CreateBr(if_continue);
			
			builder.SetInsertPoint(if_continue);
			return branch;
		}
		
		if(auto whily = dynamic_cast<While*>(k.get()))
		{
			scope.exit();

			llvm::Function* function = builder.GetInsertBlock()->getParent();
			llvm::BasicBlock* while_cond = llvm::BasicBlock::Create(context, "while_cond", function);			
			llvm::BasicBlock* while_true = llvm::BasicBlock::Create(context, "while_true", function);			
			llvm::BasicBlock* while_continue = llvm::BasicBlock::Create(context, "while_continue", function);

			builder.CreateBr(while_cond);
			builder.SetInsertPoint(while_cond);
			
			llvm::Value* condition = generateIr(whily->getHead(), scope, builder, module);
			if(!condition) return nullptr;
			
			auto branch = builder.CreateCondBr(var2val(builder, condition), while_true, while_continue);
			builder.SetInsertPoint(while_true);
			generateIr(whily->getBody(), scope, builder, module);
			builder.CreateBr(while_cond);
			
			builder.SetInsertPoint(while_continue);
			return branch;
		}
		
		if(auto fory = dynamic_cast<For*>(k.get()))
		{
			scope.exit();

			llvm::Function* function = builder.GetInsertBlock()->getParent();
			llvm::BasicBlock* for_cond = llvm::BasicBlock::Create(context, "for_cond", function);
			llvm::BasicBlock* for_true = llvm::BasicBlock::Create(context, "for_true", function);
			llvm::BasicBlock* for_continue = llvm::BasicBlock::Create(context, "for_continue", function);

			generateIr(fory->getInit(), scope, builder, module);
			
			builder.CreateBr(for_cond);
			builder.SetInsertPoint(for_cond);
			
			llvm::Value* condition = generateIr(fory->getCond(), scope, builder, module);
			if(!condition) return nullptr;
			
			auto branch = builder.CreateCondBr(var2val(builder, condition), for_true, for_continue);
			
			builder.SetInsertPoint(for_true);
			generateIr(fory->getBody(), scope, builder, module);
			generateIr(fory->getInc(), scope, builder, module);			
			builder.CreateBr(for_cond);
			
			builder.SetInsertPoint(for_continue);
			return branch;
		}

		if(auto var = dynamic_cast<ClassDef*>(k.get()))
		{
			if(scope.Classes.find(var->getName()) != scope.Classes.end())
			{
				error("class '" + var->getName() + "' is already defined", var->getLocation());
				return nullptr;
			}
			
			// Create type first
			llvm::StructType* type = llvm::StructType::create(context, var->getName());
			scope.Classes[var->getName()] = std::dynamic_pointer_cast<ClassDef>(k);
			
			std::vector<llvm::Type*> members;
			
			// First, build data fields
			for(auto& vardef : var->getFields())
			{
				if(vardef->getType() != var->getName())
					members.push_back(getType(builder, vardef->getType(), module));
			}
			
			llvm::ArrayRef<llvm::Type*> membersRef(members);
			type->setBody(membersRef);
			
			// Then methods
			for(auto& f : var->getMethods())
			{
				std::string realname = f->getName();
				f->setName(var->getName() + "_" + f->getName());
				auto& args = f->getArgs();
				args.insert(args.begin(), std::make_shared<VariableDef>("self", "@" + var->getName(), nullptr));
							
				generateIr(f, scope, builder, module);
				f->setName(realname);
			}
			
			scope.exit();
			return nullptr;
		}
		
		if(auto jmp = dynamic_cast<Goto*>(k.get()))
		{
			scope.exit();
			llvm::BasicBlock* target = (llvm::BasicBlock*) scope.current()[jmp->getName()];
			if(!target)
				error("goto target '" + jmp->getName() + "' not found", jmp->getLocation());
			
			return builder.CreateBr(target);
		}
		
		if(auto var = dynamic_cast<Number*>(k.get()))
		{
			scope.exit();
			return llvm::ConstantFP::get(context, llvm::APFloat(var->getValue()));
		}
		
		if(auto cast = dynamic_cast<TypeCast*>(k.get()))
		{
			// Cast!
			if(llvm::Type* type = getType(builder, cast->getType()))
			{
				auto value = cast->getValue();
				llvm::Value* arg = generateIr(value, scope, builder, module);
				if(!arg)
					return nullptr;
				
				if(type->isPointerTy())
				{
					scope.exit();
					return builder.CreatePointerCast(arg, type, "pointer_cast");
				}
				else
				{
					if(!arg->getType()->canLosslesslyBitCastTo(type))
						warning("converting '" + type2str(arg->getType()) 
							+ "' to '" + type2str(type) + "' looses precision", cast->getLocation());
					
					scope.exit();
					return builder.CreateBitCast(arg, type, "bit_cast");
				}
			}
			scope.exit();
		}
		
		if(auto var = dynamic_cast<Integer*>(k.get()))
		{
			scope.exit();
			return llvm::ConstantInt::get(context, llvm::APInt(32, var->getValue(), true));
		}
		
		if(auto var = dynamic_cast<Bool*>(k.get()))
		{
			scope.exit();
			return llvm::ConstantInt::get(context, llvm::APInt(1, var->getValue(), true));
		}
		
		if(auto var = dynamic_cast<Byte*>(k.get()))
		{
			scope.exit();
			return llvm::ConstantInt::get(context, llvm::APInt(8, var->getValue(), true));
		}
		
		if(auto var = dynamic_cast<String*>(k.get()))
		{
			scope.exit();
			llvm::GlobalVariable* str = builder.CreateGlobalString(var->getValue(), "string");
			str->setConstant(false);
			
			llvm::Value* zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0);
			llvm::Value* Args[] = { zero, zero };
			return builder.CreateInBoundsGEP(str->getValueType(), str, Args, "string_literal_gep");
		}
		
		if(auto ret = dynamic_cast<Return*>(k.get()))
		{
			llvm::Value* retval = generateIr(ret->getValue(), scope, builder, module);
			if(!retval) return nullptr;
			
			auto value = builder.CreateRet(var2val(builder, retval));
			scope.exit();
			return value;
		}
		
		if(auto call = dynamic_cast<FunctionCall*>(k.get()))
		{
			// Handle include
			if(call->getName() == "include")
				return nullptr;
			
			std::vector<llvm::Value*> args;	
			for(auto& p : call->getArgs()) //args.push_back(var2val(builder, generateIr(p, scope, builder, module)));
			{
				llvm::Value* value = generateIr(p, scope, builder, module);
				if(!value) return nullptr;
				args.push_back(value);
			}
			
			std::string funcname = call->getName();
			if(call->isMethod())
			{
				llvm::Value* self = args[0];
				
				if(self->getType()->isPointerTy())
					funcname = type2str(self->getType()->getPointerElementType()) + "_" + funcname;
				else
				{
					funcname = type2str(self->getType()) + "_" + funcname;
					args[0] = generateIr(std::make_shared<UnaryOp>(call->getArgs()[0], "@"), scope, builder, module);
				}
			}
			
			llvm::Function* calleeFunc = module->getFunction(funcname);
			if(!calleeFunc)
			{
				error("undefined function '" + call->getName() + "'", call->getLocation());
				return nullptr;
			}
			
			// Check types
			{
				if(!calleeFunc->isVarArg() && calleeFunc->getArgumentList().size() != args.size())
				{
					error("argument count mismatch, required " 
						+ std::to_string(calleeFunc->getArgumentList().size())
						+ " but given " + std::to_string(args.size()), call->getLocation());
					return nullptr;
				}
				
				auto iter = args.begin();
				size_t i = 0;
				for(auto& arg : calleeFunc->args())
				{
					if((*iter)->getType() != arg.getType())
					{
						error("argument type mismatch, expected '" 
						+ type2str(arg.getType())
						+ "' but got '" + type2str((*iter)->getType()), call->getArgs()[i]->getLocation());
					
						return nullptr;
					}
					
					i++;
					iter++;
				}
			}
			
			llvm::ArrayRef<llvm::Value*> argsRef(args);
			
			scope.exit();
			return builder.CreateCall(calleeFunc, argsRef);
		}
		
		scope.exit();
		return nullptr;
	}
	
	void generateIr(std::vector<std::shared_ptr<Expr>>& expressions, LocalScope& scope, llvm::IRBuilder<>& builder, llvm::Module* module)
	{
		for(auto& k : expressions)
		{
			generateIr(k, scope, builder, module);
		}
	}
	
	void writeLlvm(const std::string& where)
	{
		// Get a list of required librarie and
		// llvm::Linker::link them.
		preprocess();
		// dump();
		
		llvm::Module* module = new llvm::Module(where, context);
		llvm::IRBuilder<> builder(context); 
		
		LocalScope scope;
		generateIr(TopLevel, scope, builder, module);
		
		//module->dump();
				
		std::error_code error;
		llvm::raw_fd_ostream out(where, error, llvm::sys::fs::F_None);
		//WriteBitcodeToFile(module, out);
		//out.flush();

		module->print(out, nullptr, true, true);
		
		if(ErrorCount > 0)
		{
			std::cerr << "Encountered " << ErrorCount << " errors." << std::endl;
			std::exit(EXIT_FAILURE);
		}
	}

	void writeModule(const std::string& where)
	{
		std::ofstream out(where);
		if(!out)
			return;
		
		for(auto& k : TopLevel)
		{
			out << k->getDefinitionString();
		}
		
		out.close();
	}
	
	void preprocess()
	{
		for(size_t i = 0;  i < TopLevel.size(); i++)
		{
			auto& k = TopLevel[i];
			if(auto call = dynamic_cast<FunctionCall*>(k.get()))
			{
				// Handle include
				if(call->getName() == "include")
				{
					if(call->getArgs().size() != 1)
					{
						error("include called with a wrong number of parameters", call->getLocation());
						continue;
					}
					
					String* filename = dynamic_cast<String*>(call->getArgs()[0].get());
					if(!filename)
					{
						error("include requires a string constant as parameter", call->getLocation());
						continue;
					}
					
					std::string currname = SourceName;
					SourceName = filename->getValue();
					
					std::shared_ptr<Module> module = IncludeCallback(SourcePath + filename->getValue());
					
					if(!module)
					{
						SourceName = currname;
						error("could not include file '" + SourcePath + filename->getValue() + "'", SourceLocation());
						exit(1);
						break;
					}
					
					TopLevel.insert(TopLevel.begin() + i + 1, module->TopLevel.begin(), module->TopLevel.end());
					TopLevel.erase(TopLevel.begin() + i);
					i--;
					
					SourceName = currname;
					continue;
				}
			}
			else if(auto classdef = dynamic_cast<ClassDef*>(k.get()))
			{
				// Fill in members and functions
				for(auto& k : classdef->getBody())
				{
					auto function = std::dynamic_pointer_cast<Function>(k);
					if(function)
					{
						classdef->getMethods().push_back(function);
						continue;
					}
					
					auto member = std::dynamic_pointer_cast<VariableDef>(k);
					if(member)
					{
						classdef->getFields().push_back(member);
						continue;
					}
					
					error("invalid expression in class definition", k->getLocation());
				}
			}
		}
	}
	
	std::string type2str(llvm::Type* type)
	{
		std::string prefix;
		while(type->isPointerTy())
		{
			prefix += "@";
			type = type->getPointerElementType();
		}
		
		if(llvm::isa<llvm::StructType>(type))
		{
			std::string name = static_cast<llvm::StructType*>(type)->getName();
			return prefix + name;
		}
		
		if(type->isIntegerTy(32))
			return prefix + "int";
		else if(type->isIntegerTy(1))
			return prefix + "bool";
		else if(type->isIntegerTy(8))
			return prefix + "byte";
		else if(type->isFloatTy())
			return prefix + "float";
		
		return "unknown";
	}
	
	std::string getOperatorName(const std::string& op, const std::string& argL, const std::string& argR)
	{
		return op + "_" + argL + "_" + argR;
	}
	
	Function* findFunction(const std::string& name)
	{
		Function* fn;
		for(auto& k : TopLevel)
			if((fn = dynamic_cast<Function*>(k.get())) && fn->getName() == name)
				return fn;
	}
	
	llvm::FunctionType* getFunctionTypeFromName(llvm::IRBuilder<>& builder, llvm::Module* module, const std::string& name, bool vararg, llvm::ArrayRef<llvm::Type*> args = nullptr)
	{
		return llvm::FunctionType::get(getType(builder, name, module), args, vararg);
	}
	
	llvm::Type* getType(llvm::IRBuilder<>& builder, const std::string& name, llvm::Module* module = nullptr)
	{
		std::string type = name.substr(name.find_first_not_of("@"));
		unsigned int numAts = name.size() - type.size();
		
		llvm::Type* retval = nullptr;
		if(type == "void")
		{
			if(numAts == 0)
				retval = builder.getVoidTy();
			else
				retval = builder.getInt8PtrTy();
		}
		else if(type == "int")
		{
			retval = builder.getInt32Ty();
		}
		else if(type == "bool")
		{
			retval = builder.getInt1Ty();
		}
		else if(type == "float")
		{
			retval = builder.getFloatTy();
		}
		else if(type == "string")
		{
			retval = builder.getInt8PtrTy();
		}
		else if(type == "byte")
		{
			retval = builder.getInt8Ty();
		}
		else
		{
			if(module)
				retval = module->getTypeByName(type);
		}
		
		if(!retval)
		{
			//llvm::report_fatal_error("Unknown type " + name);
			return nullptr;
		}
		
		// Apply pointer depth
		for(unsigned int i = 0; i < numAts; i++)
			retval = retval->getPointerTo();
		
		return retval;
	}
	
	// Maybe cache lines somehow?
	std::string getSourceLine(const std::string& file, size_t idx)
	{
		if(idx == 0)
			return "";
		
		std::ifstream in(file);
		if(!in) return "";
		
		std::string dummy;
		for(size_t i = 0; i < idx - 1; i++)
			std::getline(in, dummy); //in.ignore(SIZE_MAX, '\n');
		
		std::string result;
		std::getline(in, result);
		in.close();
		
		return result;
	}
	
	std::string highlightSourceLine(const std::string& line, const SourceLocation& loc)
	{
		if(line.empty())
			return "";
		
		std::stringstream ss;
		// Trim leading spaces and tabs
		size_t trimOffset = 0;
		while(line[trimOffset] == ' ' || line[trimOffset] == '\t') trimOffset++;
		
		ss << "\t" << line.substr(trimOffset) << "\n\t";
		for(size_t i = 0; i < loc.getCol() - 2; i++)
			ss << " ";
		
		
		for(size_t i = 0; i < loc.getSize(); i++)
			ss << "^";
		
		ss << "\n";
		return ss.str();
	}
	
	void error(const std::string& message, const SourceLocation& loc)
	{
		std::cerr << SourceName << ":" << loc.getLine() << ":" << loc.getCol() << ": error: " << message << std::endl;
		std::cerr << highlightSourceLine(getSourceLine(SourceName, loc.getLine()), loc);
		ErrorCount++;
		//std::exit(EXIT_FAILURE);
	}
	
	void warning(const std::string& message, const SourceLocation& loc)
	{
		std::cerr << SourceName << ":" << loc.getLine() << ":" << loc.getCol() << ": warning: " << message << std::endl;
		std::cerr << highlightSourceLine(getSourceLine(SourceName, loc.getLine()), loc);
	}
};

}

// Some parse defs
typedef std::vector<std::shared_ptr<AST::Expr>> ExprList;
struct FunctionBody
{
	ExprList* Body = nullptr;
	ExprList* Args = nullptr;
	std::string Type;
	bool IsMethod = false;
	bool IsVariadic = false;
	
	~FunctionBody() { if(Body) delete Body; if(Args) delete Args; }
};
