#pragma once

#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>

static std::string type2str(llvm::Type* type)
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
	else if(type->isIntegerTy(64))
		return prefix + "int64";
	else if(type->isIntegerTy(16))
		return prefix + "short";
	else if(type->isIntegerTy(1))
		return prefix + "bool";
	else if(type->isIntegerTy(8))
		return prefix + "byte";
	else if(type->isFloatTy())
		return prefix + "float";
	else if(type->isVoidTy())
		return prefix + "void";

	return "unknown";
}

